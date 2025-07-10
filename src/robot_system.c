#include "robot_system.h"

void robot_state_equation(const double* x, const double* u, double* dx) {
    if (!x || !u || !dx) return;

    dx[X_POS] = u[INPUT_V] * sin(x[THETA_ORI]);
    dx[Y_POS] = u[INPUT_V] * cos(x[THETA_ORI]);
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

    // u(t) = 0 ~0 0 SEGS <
    // u(t) = 1 ~ 0.2π  0~ 10 SEG
    // u(t) = 1 ~-0.2π 10SEG >
    
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

shared_data* create_shared_data(const char* filename) {
    shared_data* data = malloc(sizeof(shared_data));
    if (!data) return NULL;
    
    data->state_matrix = zero_Matrix(ROBOT_NUM_STATES, 1);
    data->input_matrix = zero_Matrix(ROBOT_NUM_INPUTS, 1);
    data->output_matrix = zero_Matrix(ROBOT_NUM_OUTPUTS, 1);
    
    if (!data->state_matrix || !data->input_matrix || !data->output_matrix) {
        destroy_shared_data(data);
        return NULL;
    }
    
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
    fprintf(data->output_file, "# t\tv\tomega\txf\tyf\ttheta_f\n"); 
    
    if (pthread_mutex_init(&data->data_mutex, NULL) != 0) {
        printf("Falha no mutex init. shared data\n");
        destroy_shared_data(data);
        return NULL;
    }
    
    if (sem_init(&data->new_input_available, 0, 0) != 0 ||  // inicializa os semaforos de nova entrada e e calculo finalizado
        sem_init(&data->output_calculated, 0, 0) != 0) {
        printf("Falha inicio dos semanaforos\n");
        destroy_shared_data(data);
        return NULL;
    }
    
    data->current_time = SIM_START_TIME;
    data->simulation_running = 1;
    data->simulation_cycles = 0;
    data->sampling_cycles = 0;
    
    get_val(data->state_matrix, X_POS, 0) = 0.0;
    get_val(data->state_matrix, Y_POS, 0) = 0.0;
    get_val(data->state_matrix, THETA_ORI, 0) = 0.0;
    
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
   
    printf("Flag 2 destruindo pthreads e semaforos\n");
    pthread_mutex_destroy(&data->data_mutex);
    sem_destroy(&data->new_input_available);
    sem_destroy(&data->output_calculated);
    
    free(data);
}

void sleep_ms(int milliseconds) {
    usleep(milliseconds * 1000);
}

long get_current_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

void* simulation_task(void* arg) {
    simulation_activity* sim = (simulation_activity*)arg;
    shared_data* shared = sim->shared;
    printf("Começo da simulacoa\n");
    
    double u[ROBOT_NUM_INPUTS];
    double x[ROBOT_NUM_STATES];
    double y[ROBOT_NUM_OUTPUTS];
    double g_x[ROBOT_NUM_STATES];
    double temp[ROBOT_NUM_STATES];
    double dt = SIMULATION_DT;  
    
    long start_time_ms = get_current_time_ms();
    long next_execution = start_time_ms;
    
    while (shared->simulation_running) {
        next_execution += SIMULATION_PERIOD_MS;
        long current_time = get_current_time_ms();
        if (next_execution > current_time) {
            sleep_ms(next_execution - current_time);
        }
        
        if (!(shared->simulation_running)){
            printf("Simulacao parou 1\n");
            break;
        }

        
        sem_wait(&shared->new_input_available);
        
        if (!shared->simulation_running){
            printf("Simulacao parou 2\n");
            break;
        }
        
        // Somente leitura de ut para calculo
        pthread_mutex_lock(&shared->data_mutex);
        u[INPUT_V] = get_val(shared->input_matrix, INPUT_V, 0);
        u[INPUT_OMEGA] = get_val(shared->input_matrix, INPUT_OMEGA, 0);
        double t = shared->current_time;
        pthread_mutex_unlock(&shared->data_mutex);
        
        x[X_POS] = get_val(shared->state_matrix, X_POS, 0);
        x[Y_POS] = get_val(shared->state_matrix, Y_POS, 0);
        x[THETA_ORI] = get_val(shared->state_matrix, THETA_ORI, 0);
        
            

        double* g_x = x;
        
        double* g_u  = u;
        double* g_dx = temp;
        int g_index;

        double state_der_wrapper(){
            robot_state_equation(g_x,g_u,g_dx);
            return g_dx[g_index];
        }


        for (int i = 0; i < ROBOT_NUM_STATES; i++) {
            g_index = i;
            x[i] += simpson(state_der_wrapper, t, t + dt, 4);
        }

        get_val(shared->state_matrix, X_POS, 0) = x[X_POS];
        get_val(shared->state_matrix, Y_POS, 0) = x[Y_POS];
        get_val(shared->state_matrix, THETA_ORI, 0) = x[THETA_ORI];
        
        calculate_output(x, y);

        pthread_mutex_lock(&shared->data_mutex);
        get_val(shared->output_matrix, X_POS, 0) = y[X_POS];
        get_val(shared->output_matrix, Y_POS, 0) = y[Y_POS];
        get_val(shared->output_matrix, THETA_ORI, 0) = y[THETA_ORI];
        pthread_mutex_unlock(&shared->data_mutex);
        
        // Sinalize sem de saida
        sem_post(&shared->output_calculated);
        
        shared->simulation_cycles++;
    }
    
    printf("Simulacao acabopu cpm %ld ciclos\n", shared->simulation_cycles);
    return NULL;
}

void* input_sampling_task(void* arg) {
    simulation_activity* sim = (simulation_activity*)arg;
    shared_data* shared = sim->shared;

    
    double t = sim->start_time;
    double u[ROBOT_NUM_INPUTS];
    double y[ROBOT_NUM_OUTPUTS];
    double x[ROBOT_NUM_STATES];
    
    long start_time_ms = get_current_time_ms();
    long next_execution = start_time_ms;
    
    while (t <= sim->end_time && shared->simulation_running) {
        // força espera do tempo
        next_execution += SAMPLING_PERIOD_MS;
        long current_time = get_current_time_ms();
        if (next_execution > current_time) {
            sleep_ms(next_execution - current_time);
        }
        
        robot_input_function(t, u);
        
        // pega lock pra atualizar valores
        pthread_mutex_lock(&shared->data_mutex);
        get_val(shared->input_matrix, INPUT_V, 0) = u[INPUT_V];
        get_val(shared->input_matrix, INPUT_OMEGA, 0) = u[INPUT_OMEGA];
        shared->current_time = t;
        pthread_mutex_unlock(&shared->data_mutex);
        
        // Sinaliza nova entrada
        if (shared->sampling_cycles % 5 == 0) {
            sem_post(&shared->new_input_available);
            
            // espera saida calcular
            sem_wait(&shared->output_calculated);
        }
        
        pthread_mutex_lock(&shared->data_mutex);
        
        y[X_POS] = get_val(shared->output_matrix, X_POS, 0);
        y[Y_POS] = get_val(shared->output_matrix, Y_POS, 0);
        y[THETA_ORI] = get_val(shared->output_matrix, THETA_ORI, 0);
        
        pthread_mutex_unlock(&shared->data_mutex);
        
        if (shared->sampling_cycles % 1 == 0) {
            fprintf(shared->output_file, "%.6f\t%.6f\t%.6f\t%.6f\t%.6f\t%.6f\n",
                    t,
                    u[INPUT_V], u[INPUT_OMEGA],
                    y[X_POS], y[Y_POS], y[THETA_ORI]);
        }
        printf("Ciclo finalizado %l . Valores registrados:  %.6f\t%.6f\t%.6f\t%.6f\t%.6f\t%.6f\n",shared->sampling_cycles,
            t,
            u[INPUT_V], u[INPUT_OMEGA],
            y[X_POS], y[Y_POS], y[THETA_ORI]);
        
        t += SAMPLING_DT;
        shared->sampling_cycles++;
    }
    
    pthread_mutex_lock(&shared->data_mutex);
    shared->simulation_running = 0;
    pthread_mutex_unlock(&shared->data_mutex);
    
    sem_post(&shared->new_input_available);
    
    printf("entradas acabaram em %ld ciclos\n", shared->sampling_cycles);
    return NULL;
}

int run_multitask_simulation(const char* output_filename) {   
    shared_data* shared = create_shared_data(output_filename);
    if (!shared) {
        printf("falha na criação de dados\n");
        return -1;
    }
    
    simulation_activity sim = {
        .shared = shared,
        .start_time = SIM_START_TIME,
        .end_time = SIM_END_TIME,
        .task_period_ms = SIMULATION_PERIOD_MS
    };
    
    simulation_activity sample = {
        .shared = shared,
        .start_time = SIM_START_TIME,
        .end_time = SIM_END_TIME,
        .task_period_ms = SAMPLING_PERIOD_MS
    };
    
    pthread_t simulation_tid, sampling_tid;
    
    if (pthread_create(&simulation_tid, NULL, simulation_task, &sim) != 0) {
        printf("falha na thread de simulacao. multitask\n");
        destroy_shared_data(shared);
        return -1;
    }
    
    if (pthread_create(&sampling_tid, NULL, input_sampling_task, &sample) != 0) {
        printf("falha na thread de inputs\n");
        pthread_cancel(simulation_tid);
        destroy_shared_data(shared);
        return -1;
    }
    
    printf("começo threads\n");
    pthread_join(sampling_tid, NULL);
    pthread_join(simulation_tid, NULL);
    
    destroy_shared_data(shared);
    
    return 0;
}