#include "robot_system.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <string.h>


void* (*task_functions[8])(void*arg) = {
    simulation_task,
    feedback_task,
    control_task,
    ref_x_task,
    ref_y_task,
    ref_gen_task,
    sim_metrics_task,
    cpu_load_task
};

void* cpu_load_task(void* arg) {
    shared_data* data = (shared_data*)arg;
    double load_level = data->load_level;
    while (data->simulation_running) {
        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);
        long duration_us = (long)(load_level * 10000); 
        while (1) {
            double x = 0.0;
            for (int i = 0; i < 100000; i++) {
                x += i; // op inutil/ usando cpu
            }
            clock_gettime(CLOCK_MONOTONIC, &end);
            long elapsed_us = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000;
            if (elapsed_us >= duration_us) break;
        }
        usleep(10000 - duration_us);
    }
    return NULL;
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

    pthread_mutex_init(&data->data_mutex, NULL);
    
    data->filename = newADTS_str((char*)filename);
    if (!data->filename) {
        destroy_shared_data(data);
        return NULL;
    }
    
    data->output_file = fopen($(data->filename), "w");
    if (!data->output_file) {
        destroy_shared_data(data);
        return NULL;
    }
    fprintf(data->output_file, "t,x_ref,y_ref,y_bot_x,y_bot_y,xf,yf,theta\n"); 
    
    if (sem_init(&data->new_input_available, 0, 0) != 0 ||
        sem_init(&data->output_calculated, 0, 0) != 0 ||
        sem_init(&data->ref_updated, 0, 0) != 0 ||
        sem_init(&data->y_bot_updated, 0, 0) != 0 ){
            printf("Falha inicio dos semanaforos\n");
            destroy_shared_data(data);
            return NULL;
    }
    
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
    data->sim_metrics_cycles = 0;
    data->max_samples = (long) ((SIM_END_TIME - SIM_START_TIME) / SIMULATION_DT) + 1;
    data->sim_times = calloc(data->max_samples, sizeof(double));
    data->feedback_times = calloc(data->max_samples, sizeof(double));
    data->control_times = calloc(data->max_samples, sizeof(double));
    data->ref_x_times = calloc(data->max_samples, sizeof(double));
    data->ref_y_times = calloc(data->max_samples, sizeof(double));
    data->ref_gen_times = calloc(data->max_samples, sizeof(double));
    data->sim_metrics_times = calloc(data->max_samples, sizeof(double));
    data->load_level = 0.0;

    SET_MDATA(data,state_matrix, X_POS, 0.0);
    SET_MDATA(data,state_matrix, Y_POS, 0.0);
    SET_MDATA(data,state_matrix, THETA_ORI, 0.0);
    
    SET_MDATA(data,input_matrix, INPUT_V, 0.0);
    SET_MDATA(data,input_matrix, INPUT_OMEGA, 0.0);
    
    SET_MDATA(data,y_ref, X_POS, 0.0);
    SET_MDATA(data,y_ref, Y_POS, 0.0);
    SET_MDATA(data,y_bot, X_POS, 0.0);
    SET_MDATA(data,y_bot, Y_POS, 0.0);
    
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
    
    if (data->sim_times) free(data->sim_times);
    if (data->feedback_times) free(data->feedback_times);
    if (data->control_times) free(data->control_times);
    if (data->ref_x_times) free(data->ref_x_times);
    if (data->ref_y_times) free(data->ref_y_times);
    if (data->ref_gen_times) free(data->ref_gen_times);
    if (data->sim_metrics_times) free(data->sim_metrics_times);
    printf("Dando free\n");
    
    free(data);
    printf("sucess\n");
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
    printf("Começo da simulacao\n");
    
    double u[ROBOT_NUM_INPUTS];
    double x[ROBOT_NUM_STATES];
    double y[ROBOT_NUM_OUTPUTS];
    double dx[ROBOT_NUM_STATES];
    double dt = SIMULATION_DT;  
    
    long start_time_ms = get_current_time_ms();
    long next_execution = start_time_ms;
    
    while (data->simulation_running && data->current_time <= SIM_END_TIME) {
        long task_start = get_current_time_ms();
        next_execution += SIMULATION_PERIOD_MS;
        long current_time = get_current_time_ms();
        if (next_execution > current_time) {
            sleep_ms(next_execution - current_time);
        }

        sem_wait(&data->new_input_available);
        
        if (!data->simulation_running || data->current_time > SIM_END_TIME){
            break;
        }

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
        
        data->current_time += dt;
        pthread_mutex_unlock(&data->data_mutex);
        
        sem_post(&data->output_calculated);
    
        if (data->simulation_cycles < data->max_samples) {
            data->sim_times[data->simulation_cycles] = (get_current_time_ms() - task_start) / 1000.0;
        }
        data->simulation_cycles++;
    }
    
    pthread_mutex_lock(&data->data_mutex);
    data->simulation_running = 0;
    pthread_mutex_unlock(&data->data_mutex);
    
    sem_post(&data->new_input_available);
    sem_post(&data->output_calculated);
    sem_post(&data->ref_updated);
    sem_post(&data->ref_updated);
    sem_post(&data->y_bot_updated);
    sem_post(&data->y_bot_updated);
    
    printf("Simulacao acabou com %ld ciclos\n", data->simulation_cycles);
    return NULL;
}

void* feedback_task(void* arg) {
    shared_data* data = (shared_data*)arg;
    double x[ROBOT_NUM_STATES], y_ref[ROBOT_NUM_OUTPUTS], u[ROBOT_NUM_INPUTS];
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
        y_ref[X_POS] = MDATA(data, y_ref, X_POS);
        y_ref[Y_POS] = MDATA(data, y_ref, Y_POS);
        pthread_mutex_unlock(&data->data_mutex);

        feedback_linearization(x, y_ref, u,data->alpha_1,data->alpha_2);

        pthread_mutex_lock(&data->data_mutex);
        SET_MDATA(data, input_matrix, INPUT_V, u[INPUT_V]);
        SET_MDATA(data, input_matrix, INPUT_OMEGA, u[INPUT_OMEGA]);
        pthread_mutex_unlock(&data->data_mutex);

        sem_post(&data->new_input_available);
        
        if (data->feedback_cycles < data->max_samples) {
            data->feedback_times[data->feedback_cycles] = (get_current_time_ms() - task_start) / 1000.0;
        }
        data->feedback_cycles++;
    }
    
    sem_post(&data->new_input_available);
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
        sem_wait(&data->ref_updated);
        if (!data->simulation_running) break;

        pthread_mutex_lock(&data->data_mutex);
        SET_MDATA(data, y_ref, X_POS, MDATA(data, y_bot, X_POS));
        SET_MDATA(data, y_ref, Y_POS, MDATA(data, y_bot, Y_POS));
        pthread_mutex_unlock(&data->data_mutex);

        sem_post(&data->output_calculated);
        
        if (data->control_cycles < data->max_samples) {
            data->control_times[data->control_cycles] = (get_current_time_ms() - task_start) / 1000.0;
        }
        data->control_cycles++;
    }
    
    sem_post(&data->output_calculated);
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
        
        if (data->ref_x_cycles < data->max_samples) {
            data->ref_x_times[data->ref_x_cycles] = (get_current_time_ms() - task_start) / 1000.0;
        }
        data->ref_x_cycles++;
    }
    
    sem_post(&data->ref_updated);
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
        
        if (data->ref_y_cycles < data->max_samples) {
            data->ref_y_times[data->ref_y_cycles] = (get_current_time_ms() - task_start) / 1000.0;
        }
        data->ref_y_cycles++;
    }
    
    sem_post(&data->ref_updated);
    return NULL;
}

void* ref_gen_task(void* arg) {
    shared_data* data = (shared_data*)arg;
    long start_time_ms = get_current_time_ms();
    long next_execution = start_time_ms;
    
    sem_post(&data->y_bot_updated);
    sem_post(&data->y_bot_updated);
    sem_post(&data->output_calculated);
    sem_post(&data->new_input_available);

    while (data->simulation_running) {
        long task_start = get_current_time_ms();
        next_execution += REF_GEN_PERIOD_MS;
        long current_time = get_current_time_ms();
        if (next_execution > current_time) {
            sleep_ms(next_execution - current_time);
        }

        pthread_mutex_lock(&data->data_mutex);
        if (!data->simulation_running) {
            pthread_mutex_unlock(&data->data_mutex);
            break;
        }
        double t = data->current_time;
        double x_ref, y_ref;
        generate_reference(t, &x_ref, &y_ref);
        SET_MDATA(data, y_ref, X_POS, x_ref);
        SET_MDATA(data, y_ref, Y_POS, y_ref);
        pthread_mutex_unlock(&data->data_mutex);

        sem_post(&data->y_bot_updated);
        sem_post(&data->y_bot_updated);
        if (data->ref_gen_cycles < data->max_samples) {
            data->ref_gen_times[data->ref_gen_cycles] = (get_current_time_ms() - task_start) / 1000.0;
        }
        data->ref_gen_cycles++;
    }
    
    sem_post(&data->y_bot_updated);
    sem_post(&data->y_bot_updated);
    return NULL;
}

void* sim_metrics_task(void* arg) {
    shared_data* data = (shared_data*)arg;
    char buffer[3000];
    int buffer_pos = 0;
    long start_time_ms = get_current_time_ms();
    long next_execution = start_time_ms;

    while (data->simulation_running) {
        long task_start = get_current_time_ms();
        next_execution += SIM_MET_PERIOD_MS;
        long current_time = get_current_time_ms();
        if (next_execution > current_time) {
            sleep_ms(next_execution - current_time);
        }

        pthread_mutex_lock(&data->data_mutex);
        if (!data->simulation_running) {
            pthread_mutex_unlock(&data->data_mutex);
            break;
        }
        buffer_pos += snprintf(buffer + buffer_pos, sizeof(buffer) - buffer_pos,
                               "%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f,%.7f\n",
                               data->current_time,
                               MDATA(data, y_ref, X_POS), MDATA(data, y_ref, Y_POS),
                               MDATA(data, y_bot, X_POS), MDATA(data, y_bot, Y_POS),
                               MDATA(data, output_matrix, X_POS), MDATA(data, output_matrix, Y_POS),
                               MDATA(data, output_matrix, THETA_ORI));
        pthread_mutex_unlock(&data->data_mutex);
        
        if (buffer_pos > 2000) {
            fwrite(buffer, 1, buffer_pos, data->output_file);
            fflush(data->output_file);
            buffer_pos = 0;
        }

        if (data->sim_metrics_cycles < data->max_samples) {
            data->sim_metrics_times[data->sim_metrics_cycles] = (get_current_time_ms() - task_start) / 1000.0;
        }
        data->sim_metrics_cycles++;
    }

    if (buffer_pos > 0) {
        fwrite(buffer, 1, buffer_pos, data->output_file);
        fflush(data->output_file);
    }
    return NULL;
}

void log_performance_metrics(shared_data* data, const char* str_load) {
    FILE* file = fopen("performance_metrics.txt", "a");

    fprintf(file, "%s\n", str_load);
    fprintf(file, "Thread  ###  Media(s)  ###  Desvio(s)  ###  Max(s)  ###  Min(s)  ###  Jitter(s)  ###  Misses\n");
    fprintf(file, "---------------------------------------------------------------------------\n");

    thread_metrics metrics[NUM_THREADS] = {0};
    double* times[] = {
        data->sim_times,
        data->feedback_times,
        data->control_times,
        data->ref_x_times,
        data->ref_y_times,
        data->ref_gen_times,
        data->sim_metrics_times
    };
    long cycles[] = {
        data->simulation_cycles,
        data->feedback_cycles,
        data->control_cycles,
        data->ref_x_cycles,
        data->ref_y_cycles,
        data->ref_gen_cycles,
        data->sim_metrics_cycles
    };
    double periods[] = {
        SIMULATION_PERIOD_MS / 1000.0, 
        FEEDBACK_PERIOD_MS / 1000.0,
        CONTROL_PERIOD_MS / 1000.0,
        REF_X_PERIOD_MS / 1000.0,
        REF_Y_PERIOD_MS / 1000.0, 
        REF_GEN_PERIOD_MS / 1000.0,
        SIM_MET_PERIOD_MS / 1000.0
    };

    const char* thread_names[] = {
        "Simulation",
        "Feedback",
        "Control",
        "Ref_X",
        "Ref_Y",
        "Ref_Gen",
        "Simulation_Metrics"
    };

    for (int i = 0; i < NUM_THREADS -1; i++) {
        if (cycles[i] == 0) continue;
        long effective_cycles = (cycles[i] > data->max_samples ? data->max_samples : cycles[i]);
        
        metrics[i].min = times[i][0];
        metrics[i].max = times[i][0];
        double sum = 0;
        
        for (long j = 0; j < effective_cycles; j++) {
            double t = times[i][j];
            sum += t;
            if (t > metrics[i].max) metrics[i].max = t;
            if (t < metrics[i].min) metrics[i].min = t;
            if (t > periods[i]) metrics[i].misses++;
        }
        metrics[i].mean = sum / effective_cycles;
        
        double sum_sq_diff = 0;
        for (long j = 0; j < effective_cycles; j++) {
            double diff = times[i][j] - metrics[i].mean;
            sum_sq_diff += diff * diff;
        }
        metrics[i].variance = sum_sq_diff / effective_cycles;
        metrics[i].std_dev = sqrt(metrics[i].variance);
        metrics[i].jitter = metrics[i].max - metrics[i].min;
        
        fprintf(file, "%-10s %-10.7f %-10.7f %-10.7f %-10.7f %-10.7f %-7ld\n",
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

int run_multitask_simulation(shared_data* data,const char* output_filename,int sim_loaded) {   
    simulation_activity sim = {data, SIM_START_TIME, SIM_END_TIME};
    
    int result;
    int num_threads = NUM_THREADS;
    for (int index = 0; index < 7; index++){
        if(index == 0){
            result = pthread_create(&data->tids[index], NULL, task_functions[index], &sim);
        } else {
            result = pthread_create(&data->tids[index],NULL, task_functions[index], data);
        }
        if (result != 0) {
            printf("Falha na thread %d\n",index);
            continue;
        }
    }

    if (sim_loaded) {
        printf("Comeco sim com carga\n");
        data->load_level = 0.5;
        result = pthread_create(&data->tids[7], NULL, task_functions[7], data);
        if (result != 0) printf("Falha na sim com carga\n");
        else num_threads++;
    }
    
    printf("Threads started\n");
    
    for(int index = 0 ; index < num_threads; index++){
        if(sim_loaded && index == 7){
            break;
        } 
        pthread_join(data->tids[index], NULL);
        printf("Thread iniciada %d\n",index);
    }
    if(sim_loaded) pthread_join(data->tids[7], NULL);
    const char* str_load= sim_loaded ? "Com Carga" : "Sem Carga:";
    printf("Fazendo logs dos arquivos de %s\n",str_load);
    log_performance_metrics(data, str_load);
    printf("Destruindo data\n");
    destroy_shared_data(data);
    
    return 0;
}