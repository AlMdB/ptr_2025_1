// robot_system.h
#ifndef ROBOT_SYSTEM_H
#define ROBOT_SYSTEM_H
#define _GNU_SOURCE 

#include "adt_matrix.h"
#include "adt_integral.h"
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <sched.h>
#include <unistd.h>
#include <time.h>
#include <string.h>


typedef struct {
    double mean;
    double variance;
    double std_dev;
    double max;
    double min;
    double jitter;
    long misses;
}thread_metrics;

#define NUM_THREADS 8


// nunca usados.r eferencia para o loop na criacao de threads
typedef enum {
    SIM_METRIC = 0,
    FEEDBACK_METRIC,
    CONTROL_METRIC,
    REF_X_METRIC,
    REF_Y_METRIC,
    REF_GEN_METRIC,
    SIM_METS_METRIC,
    LOAD_METRIC
} MetricType;


// Indices espaciais do robo(Pisicao x y centro de massa e orientacao no momento)
#define X_POS 0
#define Y_POS 1
#define THETA_ORI 2

// Indices para os vetores(de vetores) de entrada V e w (Velocidades linear e angular)
#define INPUT_V 0 
#define INPUT_OMEGA 1

#define ROBOT_NUM_STATES 3
#define ROBOT_NUM_INPUTS 2 
#define ROBOT_NUM_OUTPUTS 3

//Dioamtro do robop
#define ROBOT_DIAMETER 0.6
#define ROBOT_RADIUS 0.3

// Parametros 20 segs de teste e amostragem_periodo dos dados
#define SIM_START_TIME 0.0
#define SIM_END_TIME 20.0
#define SAMPLING_PERIOD_MS 10      

#define SIMULATION_DT 0.05 //mesmo de cima mas convertido para acelerar uso
#define SAMPLING_DT 0.01 

#define SIMULATION_PERIOD_MS 30    
#define FEEDBACK_PERIOD_MS 40
#define CONTROL_PERIOD_MS 50
#define REF_X_PERIOD_MS 50
#define REF_Y_PERIOD_MS 50
#define REF_GEN_PERIOD_MS 120

#define SIM_MET_PERIOD_MS 100

#define ALPHAS_O 3.0

//Macros pra melhorar legibilidade
#define MDATA(shared, matrix, idx) get_val((shared)->matrix, (idx), 0)
#define SET_MDATA(shared, matrix, idx, value) (get_val((shared)->matrix, (idx), 0) = (value))


typedef struct {
    // So simulacao usa
    Matrix* state_matrix;      // x(t) = [xc, yc, 0]
    
    // Simulacao e thread de leitura
    Matrix* input_matrix;      // u(t) = [v, ω]
    Matrix* output_matrix; // y(t) = [xf, yf, θ] - front point

    Matrix* y_ref;
    Matrix* y_bot;

    double alpha_1, alpha_2;

    double current_time;
    int simulation_running;

    pthread_mutex_t data_mutex;

    pthread_t tids[NUM_THREADS];

    sem_t new_input_available; 
    sem_t output_calculated;
    sem_t ref_updated;
    sem_t y_bot_updated;


    FILE* output_file;
    adt_string* filename;
    
    long simulation_cycles;
    long sampling_cycles;
    long feedback_cycles;

    long control_cycles;
    long ref_x_cycles;
    long ref_y_cycles;
    long ref_gen_cycles;
    long sim_metrics_cycles;

    double* sim_times;
    double* feedback_times;
    double* control_times;
    double* ref_x_times;
    double* ref_y_times;
    double* ref_gen_times;
    double* sim_metrics_times;
    long max_samples;

    double load_level;
    int sim_with_load;
} shared_data;

typedef struct {
    shared_data* shared;
    double start_time;
    double end_time;
    int task_period_ms;
} simulation_activity;




void robot_state_equation(double t, double* x, double* u, double* dx);
void calculate_output(const double* x, double* y);
void robot_input_function(double t, double* u);

void feedback_linearization(const double*x, const double* y_ref,double* u, double alpha_1,double alpha_2);
void reference_model_x(double x_ref, double* y_bot_x);
void reference_model_y(double t, double y_ref, double* y_bot_y);
void generate_reference(double t, double* x_ref, double* y_ref);

double state_der_wrapper(double t, int index, double* x, double* u);

void* simulation_task(void* arg);
void* input_sampling_task(void* arg);
void* feedback_task(void* arg);
void* control_task(void* arg);
void* ref_x_task(void* arg);
void* ref_y_task(void* arg);
void* ref_gen_task(void* arg);
void* sim_metrics_task(void* arg);
void* cpu_load_task(void* arg);

extern void* (*task_functions[8])(void*);

shared_data* create_shared_data(const char* filename);
void destroy_shared_data(shared_data* data);
int run_multitask_simulation(shared_data* data,const char* output_filename,int sim_loaded);

void sleep_ms(long milliseconds);
long get_current_time_ms(void);

void log_performance_metrics(shared_data* data_no_load,const char* str_load);

#endif
