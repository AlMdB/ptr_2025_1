#include "robot_system.h"

void* (*task_functions[7])(void*arg) = {
    simulation_task,
    feedback_task,
    control_task,
    ref_x_task,
    ref_y_task,
    ref_gen_task,
    ui_task
};


void set_fifo_priority(pthread_t thread, int priority) {
    struct sched_param param;
    param.sched_priority = priority;
    if (pthread_setschedparam(thread, SCHED_FIFO, &param) != 0) {
        perror("Falha no sched fifo\n");
    }
}

void set_cpu_affinity(pthread_t thread, int cpu) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);
    if (pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset) != 0) {
        perror("falha ao setar affinity\n");
    }
}


void robot_state_equation(double t,double* x, double* u, double* dx) {
    if (!x || !u || !dx) return;

    dx[X_POS] = u[INPUT_V] * cos(x[THETA_ORI]);
    dx[Y_POS] = u[INPUT_V] * sin(x[THETA_ORI]);
    dx[THETA_ORI] = u[INPUT_OMEGA];
}

void calculate_output(const double* x, double* y) {
    if (!x || !y) return;

    double half_D = 0.5 * ROBOT_DIAMETER;
    double theta = x[THETA_ORI];
    
    y[X_POS] = x[X_POS] + half_D * cos(theta);
    y[Y_POS] = x[Y_POS] + half_D * sin(theta);
    y[THETA_ORI] = x[THETA_ORI];
}

void robot_input_function(double t, double* u) {
    if (!u) return;


    
    if (t < 0) {
        u[INPUT_V] = 0.0;
        u[INPUT_OMEGA] = 0.0;
    } else if (t < 10.0) {
        u[INPUT_V] = 1.0;
        u[INPUT_OMEGA] = 0.2 * M_PI;
    } else {
        u[INPUT_V] = 1.0;
        u[INPUT_OMEGA] = -0.2 * M_PI;
    }
}

void feedback_linearization(const double* x, const double* y_ref, double* u, double alpha_1, double alpha_2){
	if(!x || ! y_ref || !u )return;
	double half_D = ROBOT_RADIUS;
	double theta = x[THETA_ORI];
	double xf = x[X_POS] + half_D * cos(theta);
	double yf = x[Y_POS] + half_D * sin(theta);
	double e_x = y_ref[X_POS] - xf;
	double e_y = y_ref[Y_POS] - yf;
	u[INPUT_V] = alpha_1 * (e_x * cos(theta) + e_y * sin(theta));
	u[INPUT_OMEGA] = alpha_2 * (-e_x * sin(theta) + e_y * cos(theta)) / half_D;
}

void reference_model_x(double x_ref, double* x_bot_x){
	*x_bot_x = x_ref;
}

void reference_model_y(double t,double y_ref, double* y_bot_y){
    static double y_bot_prev = 0.0;
    double tau = 0.1;
    double dt = SIMULATION_DT;
    *y_bot_y = y_bot_prev + (dt / tau) * (y_ref - y_bot_prev);
    y_bot_prev = *y_bot_y;
}

void generate_reference(double t, double* x_ref, double* y_ref){
	*x_ref = (5.0/M_PI)*cos(0.2 * M_PI * t);
	if(t > 0  && t < 10.0) {
		*y_ref = (5.0/M_PI)* sin(0.2 * M_PI * t);
	}else {
		*y_ref = -((5.0/M_PI) * sin(0.2 * M_PI *t));
	}
}

double state_der_wrapper(double t, int index, double* x , double* u) {
	double dx[ROBOT_NUM_STATES];
	robot_state_equation(t,x,u,dx);
	return dx[index];
}



shared_data* create_shared_data(const char* filename) {
    shared_data* data = malloc(sizeof(shared_data));
    if (!data) return NULL;
    
    data->state_matrix = zero_Matrix(ROBOT_NUM_STATES, 1);
    data->input_matrix = zero_Matrix(ROBOT_NUM_INPUTS, 1);
    data->output_matrix = zero_Matrix(ROBOT_NUM_OUTPUTS, 1);

    data->y_ref = zero_Matrix(2,1);
    data->y_bot = zero_Matrix(2,1);

    if (!data->state_matrix ||
        !data->input_matrix ||
        !data->output_matrix||
        !data->y_ref || 
        !data->y_bot) 
        {
        destroy_shared_data(data);
        return NULL;
    }

    pthread_mutexattr_t mutex_attr;
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_setprotocol(&mutex_attr, PTHREAD_PRIO_INHERIT);
    pthread_mutex_init(&data->data_mutex, &mutex_attr);
    pthread_mutexattr_destroy(&mutex_attr);


    sem_init(&data->new_input_available, 0, 0);
    sem_init(&data->output_calculated, 0, 0);
    sem_init(&data->ref_updated, 0, 0);
    sem_init(&data->y_bot_updated, 0, 0);

    
    data->filename = newADTS_str((char*)filename);
    if (!data->filename) {
        destroy_shared_data(data);
        return NULL;
    }
    
    data->output_file = fopen(data->filename, "w");
    if (!data->output_file) {
        destroy_shared_data(data);
        return NULL;
    }
    fprintf(data->output_file, "# t\tx_ref\ty_ref\ty_bot_x\ty_bot_y\txf\tyf\ttheta\n"); 
    
    if (sem_init(&data->new_input_available, 0, 0) != 0 ||  // inicializa os semaforos de nova entrada e e calculo finalizado
        sem_init(&data->output_calculated, 0, 0) != 0 ||
        sem_init(&data->ref_updated, 0, 0) != 0 ||
        sem_init(&data->y_bot_updated, 0, 0) != 0 ){
            printf("Falha inicio dos semanaforos\n");
            destroy_shared_data(data);
            return NULL;
    }
    data->feedback_times = 0;
    data->current_time = SIM_START_TIME;
    data->simulation_running = 1;
    data->simulation_cycles = 0;
    data->sampling_cycles = 0;
    data->feedback_cycles = 0;

    
    data->alpha_1 = ALPHAS_O;
    data->alpha_2 = ALPHAS_O;
    

    data->control_cycles = 0;
    data->ref_x_cycles = 0;
    data->ref_y_cycles = 0;
    data->ref_gen_cycles = 0;
    data->ui_cycles = 0;
    data->max_samples = (long) (SIM_END_TIME / SIMULATION_DT) +1;
    data->sim_times = calloc(data->max_samples, sizeof(double));
    data->feedback_times = calloc(data->max_samples, sizeof(double));

    data->control_times = calloc(data->max_samples, sizeof(double));
    data->ref_x_times = calloc(data->max_samples, sizeof(double));
    data->ref_y_times = calloc(data->max_samples, sizeof(double));
    data->ref_gen_times = calloc(data->max_samples, sizeof(double));
    data->ui_times = calloc(data->max_samples, sizeof(double));

    SET_MDATA(data,state_matrix, X_POS, 0.0);
    SET_MDATA(data,state_matrix, Y_POS, 0.0);
    SET_MDATA(data,state_matrix, THETA_ORI, 0.0);
    
    return data;
}

void destroy_shared_data(shared_data* data) {
    printf("limpando os dados(shared)\n");
    if (!data) return;
    
    if (data->state_matrix) delete_Matrix(data->state_matrix);
    if (data->input_matrix) delete_Matrix(data->input_matrix);
    if (data->output_matrix) delete_Matrix(data->output_matrix);
    if (data->filename) del_adt_string(data->filename);
    if (data->output_file) fclose(data->output_file);

    if (data->y_ref) delete_Matrix(data->y_ref);
    if (data->y_bot) delete_Matrix(data->y_bot);
   
    printf("Flag 2 destruindo pthreads e semaforos\n");
    pthread_mutex_destroy(&data->data_mutex);
    sem_destroy(&data->new_input_available);
    sem_destroy(&data->output_calculated);
    sem_destroy(&data->ref_updated);
    sem_destroy(&data->y_bot_updated);
    free(data->sim_times);
    free(data->feedback_times);
    free(data->control_times);
    free(data->ref_x_times);
    free(data->ref_y_times);
    free(data->ref_gen_times);
    free(data->ui_times);
    
    free(data);
}

void sleep_ms(long milliseconds) {
    usleep(milliseconds * 1000);
}

long get_current_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

void* simulation_task(void* arg) {
    simulation_activity* sim = (simulation_activity*)arg;
    shared_data* data = sim->shared;
    printf("Começo da simulacoa\n");
    
    double u[ROBOT_NUM_INPUTS];
    double x[ROBOT_NUM_STATES];
    double y[ROBOT_NUM_OUTPUTS];
    double dx[ROBOT_NUM_STATES];
    double dt = SIMULATION_DT;  
    
    long start_time_ms = get_current_time_ms();
    long next_execution = start_time_ms;
    
    while (data->simulation_running) {
        long task_start = get_current_time_ms();
        next_execution += SIMULATION_PERIOD_MS;
        long current_time = get_current_time_ms();
        if (next_execution > current_time) {
            sleep_ms(next_execution - current_time);
        }

        sem_wait(&data->new_input_available);
        
        if (!(data->simulation_running)){
            printf("Simulacao parou 1\n");
            break;
        }

        // Somente leitura de ut para calculo
        pthread_mutex_lock(&data->data_mutex);
        u[INPUT_V] = MDATA(data,input_matrix, INPUT_V);
        u[INPUT_OMEGA] = MDATA(data,input_matrix, INPUT_OMEGA);
        x[X_POS] = MDATA(data,state_matrix, X_POS);
        x[Y_POS] = MDATA(data,state_matrix, Y_POS);
        x[THETA_ORI] = MDATA(data,state_matrix,THETA_ORI);
        double t = data->current_time;
        pthread_mutex_unlock(&data->data_mutex);
        
        for (int i = 0; i < ROBOT_NUM_STATES; i++) {
            x[i] += simpson((fun)state_der_wrapper, t, t + dt, 4,i,x,u);
        }




        pthread_mutex_lock(&data->data_mutex);
        SET_MDATA(data,state_matrix, X_POS, x[X_POS]);
        SET_MDATA(data,state_matrix, Y_POS,x[Y_POS]);
        SET_MDATA(data,state_matrix, THETA_ORI,x[THETA_ORI]);
        
        calculate_output(x, y);

        SET_MDATA(data,output_matrix, X_POS,y[X_POS]);
        SET_MDATA(data,output_matrix, Y_POS,y[Y_POS]);
        SET_MDATA(data,output_matrix, THETA_ORI,y[THETA_ORI]);
        pthread_mutex_unlock(&data->data_mutex);
        
        // Sinalize sem de saida
        sem_post(&data->output_calculated);
    
        data->sim_times[data->simulation_cycles] = (get_current_time_ms() - task_start) / 1000.0;
        data->simulation_cycles++;
    }
    
    printf("Simulacao acabopu cpm %ld ciclos\n", data->simulation_cycles);
    return NULL;
}


void* feedback_task(void* arg) {
    shared_data* data = (shared_data*)arg;
    double x[ROBOT_NUM_STATES], y[ROBOT_NUM_OUTPUTS], u[ROBOT_NUM_INPUTS];
    long start_time_ms = get_current_time_ms();
    long next_execution = start_time_ms;

    while (data->simulation_running) {
        long task_start = get_current_time_ms();
        next_execution += FEEDBACK_PERIOD_MS;
        long current_time = get_current_time_ms();
        if (next_execution > current_time) {
            sleep_ms(next_execution - current_time);
        }

        sem_wait(&data->output_calculated);
        if (!data->simulation_running) break;

        pthread_mutex_lock(&data->data_mutex);
        x[X_POS] = MDATA(data, state_matrix, X_POS);
        x[Y_POS] = MDATA(data, state_matrix, Y_POS);
        x[THETA_ORI] = MDATA(data, state_matrix, THETA_ORI);
        y[X_POS] = MDATA(data, output_matrix, X_POS);
        y[Y_POS] = MDATA(data, output_matrix, Y_POS);
        y[THETA_ORI] = MDATA(data, output_matrix, THETA_ORI);
        pthread_mutex_unlock(&data->data_mutex);

        feedback_linearization(x, y, u,data->alpha_1,data->alpha_2);

        pthread_mutex_lock(&data->data_mutex);
        SET_MDATA(data, input_matrix, INPUT_V, u[INPUT_V]);
        SET_MDATA(data, input_matrix, INPUT_OMEGA, u[INPUT_OMEGA]);
        pthread_mutex_unlock(&data->data_mutex);

        sem_post(&data->new_input_available);
        data->feedback_times[data->feedback_cycles] = (get_current_time_ms() - task_start) / 1000.0;
        data->feedback_cycles++;
    }
    return NULL;
}


void* control_task(void* arg) {
    shared_data* data = (shared_data*)arg;
    long start_time_ms = get_current_time_ms();
    long next_execution = start_time_ms;

    while (data->simulation_running) {
        long task_start = get_current_time_ms();
        next_execution += CONTROL_PERIOD_MS;
        long current_time = get_current_time_ms();
        if (next_execution > current_time) {
            sleep_ms(next_execution - current_time);
        }

        sem_wait(&data->ref_updated);
        if (!data->simulation_running) break;

        pthread_mutex_lock(&data->data_mutex);
        SET_MDATA(data, y_ref, X_POS, MDATA(data, y_bot, X_POS));
        SET_MDATA(data, y_ref, Y_POS, MDATA(data, y_bot, Y_POS));
        pthread_mutex_unlock(&data->data_mutex);

        sem_post(&data->output_calculated);
        data->control_times[data->control_cycles] = (get_current_time_ms() - task_start) / 1000.0;
        data->control_cycles++;
    }
    return NULL;
}

void* ref_x_task(void* arg) {
    shared_data* data = (shared_data*)arg;
    long start_time_ms = get_current_time_ms();
    long next_execution = start_time_ms;

    while (data->simulation_running) {
        long task_start = get_current_time_ms();
        next_execution += REF_X_PERIOD_MS;
        long current_time = get_current_time_ms();
        if (next_execution > current_time) {
            sleep_ms(next_execution - current_time);
        }

        sem_wait(&data->y_bot_updated);
        if (!data->simulation_running) break;

        pthread_mutex_lock(&data->data_mutex);
        double x_ref = MDATA(data, y_ref, X_POS);
        double t = data->current_time;
        double x_bot;
        reference_model_x(x_ref, &x_bot);
        SET_MDATA(data, y_bot, X_POS, x_bot);
        pthread_mutex_unlock(&data->data_mutex);

        sem_post(&data->ref_updated);
        data->ref_x_times[data->ref_x_cycles] = (get_current_time_ms() - task_start) / 1000.0;
        data->ref_x_cycles++;
    }
    return NULL;
}

void* ref_y_task(void* arg) {
    shared_data* data = (shared_data*)arg;
    long start_time_ms = get_current_time_ms();
    long next_execution = start_time_ms;

    while (data->simulation_running) {
        long task_start = get_current_time_ms();
        next_execution += REF_Y_PERIOD_MS;
        long current_time = get_current_time_ms();
        if (next_execution > current_time) {
            sleep_ms(next_execution - current_time);
        }

        sem_wait(&data->y_bot_updated);
        if (!data->simulation_running) break;

        pthread_mutex_lock(&data->data_mutex);
        double y_ref = MDATA(data, y_ref, Y_POS);
        double t = data->current_time;
        double y_bot;
        reference_model_y(t, y_ref, &y_bot);
        SET_MDATA(data, y_bot, Y_POS, y_bot);
        pthread_mutex_unlock(&data->data_mutex);

        sem_post(&data->ref_updated);
        data->ref_y_times[data->ref_y_cycles] = (get_current_time_ms() - task_start) / 1000.0;
        data->ref_y_cycles++;
    }
    return NULL;
}

void* ref_gen_task(void* arg) {
    shared_data* data = (shared_data*)arg;
    long start_time_ms = get_current_time_ms();
    long next_execution = start_time_ms;

    while (data->simulation_running) {
        long task_start = get_current_time_ms();
        next_execution += REF_GEN_PERIOD_MS;
        long current_time = get_current_time_ms();
        if (next_execution > current_time) {
            sleep_ms(next_execution - current_time);
        }

        pthread_mutex_lock(&data->data_mutex);
        double t = data->current_time;
        double x_ref, y_ref;
        generate_reference(t, &x_ref, &y_ref);
        SET_MDATA(data, y_ref, X_POS, x_ref);
        SET_MDATA(data, y_ref, Y_POS, y_ref);
        pthread_mutex_unlock(&data->data_mutex);

        sem_post(&data->y_bot_updated);
        data->ref_gen_times[data->ref_gen_cycles] = (get_current_time_ms() - task_start) / 1000.0;
        data->ref_gen_cycles++;
    }
    return NULL;
}

void* ui_task(void* arg) {
    shared_data* data = (shared_data*)arg;
    char buffer[1024];
    int buffer_pos = 0;
    long start_time_ms = get_current_time_ms();
    long next_execution = start_time_ms;

    while (data->simulation_running) {
        long task_start = get_current_time_ms();
        next_execution += UI_PERIOD_MS;
        long current_time = get_current_time_ms();
        if (next_execution > current_time) {
            sleep_ms(next_execution - current_time);
        }

        pthread_mutex_lock(&data->data_mutex);
        buffer_pos += snprintf(buffer + buffer_pos, sizeof(buffer) - buffer_pos,
                               "%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n",
                               data->current_time,
                               MDATA(data, y_ref, X_POS), MDATA(data, y_ref, Y_POS),
                               MDATA(data, y_bot, X_POS), MDATA(data, y_bot, Y_POS),
                               MDATA(data, output_matrix, X_POS), MDATA(data, output_matrix, Y_POS));
        if (buffer_pos > 900) {
            fwrite(buffer, 1, buffer_pos, data->output_file);
            fflush(data->output_file);
            buffer_pos = 0;
        }
        pthread_mutex_unlock(&data->data_mutex);

        data->ui_times[data->ui_cycles] = (get_current_time_ms() - task_start) / 1000.0;
        data->ui_cycles++;
    }

    pthread_mutex_lock(&data->data_mutex);
    fwrite(buffer, 1, buffer_pos, data->output_file);
    fflush(data->output_file);
    pthread_mutex_unlock(&data->data_mutex);
    return NULL;
}

void log_performance_metrics(shared_data* data) {
    FILE* file = fopen("data/performance_metrics.txt", "w");
    if (!file) {
        perror("Failed to open performance_metrics.txt");
        return;
    }

    thread_metrics metrics[NUM_THREADS] = {0};
    double* times[] = {
        data->sim_times,
        data->feedback_times,
        data->control_times,
        data->ref_x_times,
        data->ref_y_times,
        data->ref_gen_times,
        data->ui_times
    };
    long cycles[] = {
        data->simulation_cycles,
        data->feedback_cycles,
        data->control_cycles,
        data->ref_x_cycles,
        data->ref_y_cycles,
        data->ref_gen_cycles,
        data->ui_cycles
    };
    double periods[] = {
        SIMULATION_PERIOD_MS, 
        FEEDBACK_PERIOD_MS,
        CONTROL_PERIOD_MS,
        REF_X_PERIOD_MS,
        REF_Y_PERIOD_MS, 
        REF_GEN_PERIOD_MS,
        UI_PERIOD_MS
    };

    for(int i = 0; i < sizeof(periods)/sizeof(double); i++){
        periods[i] /= 1000.0;

    }
    const char* thread_names[] = {
        "Simulation",
        "Feedback",
        "Control",
        "Ref_X",
        "Ref_Y",
        "Ref_Gen",
        "UI"
    };

    for (int i = 0; i < NUM_THREADS; i++) {
        if (cycles[i] == 0) continue;
        double sum = 0, sum_sq_dif = 0;
        for (long j = 0; j < cycles[i]; j++) {
            metrics[i].min = times[i][0];
            metrics[i].mean += times[i][j];
            for (long j = 0; j < cycles[i]; j++) {
            double t = times[i][j];
                sum += t;
                if (t > metrics[i].max) metrics[i].max = t;
                if (t < metrics[i].min) metrics[i].min = t;
                if (t > periods[i]) metrics[i].misses++;
            }
            metrics[i].mean = sum / cycles[i];
            for (long j = 0; j < cycles[i]; j++) {
                double diff = times[i][j] - metrics[i].mean;
                sum_sq_dif += diff * diff;
        }
        metrics[i].variance = sum_sq_dif / cycles[i];
        metrics[i].std_dev = sqrt(metrics[i].variance);
        metrics[i].jitter = metrics[i].max - metrics[i].min;
        }
        metrics[i].mean /= cycles[i];
    }

    fprintf(file, "%-7s %-7s %-7s %-7s %-7s %-7s %-7s\n", "Thread", "Media (s)", "Desvio (s)","Max (s)","Min (s)","Jitter(s)", "Misses");
    fprintf(file, "------------------------------------------------------------\n");
    for (int i = 0; i < NUM_THREADS; i++) {
        fprintf(file, "%-7s %-7s %-7s %-7s %-7s %-7s %-7s\n",
                thread_names[i],
                metrics[i].mean,
                metrics[i].std_dev,
                metrics[i].max,
                metrics[i].min,
                metrics[i].jitter,
                metrics[i].misses);
    }

    fclose(file);
}

void* input_sampling_task(void* arg) {
    simulation_activity* sim = (simulation_activity*)arg;
    shared_data* data = sim->shared;

    
    double t = sim->start_time;
    double u[ROBOT_NUM_INPUTS];
    double y[ROBOT_NUM_OUTPUTS];
    double x[ROBOT_NUM_STATES];
    
    long start_time_ms = get_current_time_ms();
    long next_execution = start_time_ms;
    
    while (t <= sim->end_time && data->simulation_running) {
        // força espera do tempo
        next_execution += SAMPLING_PERIOD_MS;
        long current_time = get_current_time_ms();
        if (next_execution > current_time) {
            sleep_ms(next_execution - current_time);
        }
        
        robot_input_function(t, u);
        
        // pega lock pra atualizar valores
        pthread_mutex_lock(&data->data_mutex);
        get_val(data->input_matrix, INPUT_V, 0) = u[INPUT_V];
        get_val(data->input_matrix, INPUT_OMEGA, 0) = u[INPUT_OMEGA];
        data->current_time = t;
        pthread_mutex_unlock(&data->data_mutex);
        
        // Sinaliza nova entrada
        if (data->sampling_cycles % 5 == 0) {
            sem_post(&data->new_input_available);
            
            // espera saida calcular
            sem_wait(&data->output_calculated);
        }
        
        pthread_mutex_lock(&data->data_mutex);
        
        y[X_POS] = get_val(data->output_matrix, X_POS, 0);
        y[Y_POS] = get_val(data->output_matrix, Y_POS, 0);
        y[THETA_ORI] = get_val(data->output_matrix, THETA_ORI, 0);
        
        pthread_mutex_unlock(&data->data_mutex);
        
        if (data->sampling_cycles % 1 == 0) {
            fprintf(data->output_file, "%.6f\t%.6f\t%.6f\t%.6f\t%.6f\t%.6f\n",
                    t,
                    u[INPUT_V], u[INPUT_OMEGA],
                    y[X_POS], y[Y_POS], y[THETA_ORI]);
        }
        printf("Ciclo finalizado %l . Valores registrados:  %.6f\t%.6f\t%.6f\t%.6f\t%.6f\t%.6f\n",data->sampling_cycles,
            t,
            u[INPUT_V], u[INPUT_OMEGA],
            y[X_POS], y[Y_POS], y[THETA_ORI]);
        
        t += SAMPLING_DT;
        data->sampling_cycles++;
    }
    
    pthread_mutex_lock(&data->data_mutex);
    data->simulation_running = 0;
    pthread_mutex_unlock(&data->data_mutex);
    
    sem_post(&data->new_input_available);
    
    printf("entradas acabaram em %ld ciclos\n", data->sampling_cycles);
    return NULL;
}

int run_multitask_simulation(shared_data* data,const char* output_filename) {   
    data->output_file = fopen(output_filename,"w");
    if (!data->output_file) {
        printf("falha na criação de dados\n");
        return -1;
    }
    pthread_t tids[7] = {};
    int prio_values[7] = {90,80,70,70,70,60,65};

    

    simulation_activity sim = {data};
    

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);

    int result;

    for (int index = 0; index < 7;index++){
        if(index == 0){
            result = pthread_create(&tids[index], &attr, task_functions[index], &sim);
            //set_fifo_priority(&tids[index], 1);
            
        }else{
            result = pthread_create(&tids[index],&attr, task_functions[index], data);
            //set_fifo_priority(&tids[index], 80);
        }
        if (result != 0) {
            printf("Failed to cerated thread %d: %s\n",index,strerror(result));
            continue;
        }

        set_cpu_affinity(&tids[index], index % 4);
    }
    set_fifo_priority(&tids[index], prio_values[index]);
    printf("começo threads\n");
    for(int index = 0 ; index < 7; index++){
        pthread_join(tids[index], NULL);
    }

    


    log_performance_metrics(data);    
    destroy_shared_data(data);
    
    return 0;
}

