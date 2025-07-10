// robot_system.h
#ifndef ROBOT_SYSTEM_H
#define ROBOT_SYSTEM_H

#include "adt_matrix.h"
#include "adt_string.h"
#include "adt_integral.h"
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>

// Indices espaciais do robo(Pisicao x y centro de massa e orientacao no momento)
#define X_POS 0
#define Y_POS 1
#define THETA_ORI 2

// Indices para os vetores(de vetores) de entrada V e w (Velocidades linear e angular)
#define INPUT_V 0 
#define INPUT_OMEGA 1


// Dimensoes do sistema (Dado pelo lab2) como ***VETORES***
#define ROBOT_NUM_STATES 3
#define ROBOT_NUM_INPUTS 2 
#define ROBOT_NUM_OUTPUTS 3

//Dioamtro do robop
#define ROBOT_DIAMETER 0.30 // em metros

// Parametros 20 segs de teste e amostragem_periodo dos dados
#define SIM_START_TIME 0.0
#define SIM_END_TIME 20.0
#define SIMULATION_PERIOD_MS 50    
#define SAMPLING_PERIOD_MS 10      

#define SIMULATION_DT 0.05 //mesmo de cima mas convertido para acelerar uso
#define SAMPLING_DT 0.01 

typedef struct {
    // So simulacao usa
    Matrix* state_matrix;      // x(t) = [xc, yc, 0]
    
    // Simulacao e thread de leitura
    Matrix* input_matrix;      // u(t) = [v, ω]
    Matrix* output_matrix; // y(t) = [xf, yf, θ] - front point
    
    double current_time;
    int simulation_running;

    pthread_mutex_t data_mutex;
    sem_t new_input_available; 
    sem_t output_calculated;
    
    FILE* output_file;
    adt_string* filename;
    
    long simulation_cycles;
    long sampling_cycles;
    
} shared_data;

typedef struct {
    shared_data* shared;
    double start_time;
    double end_time;
    int task_period_ms;
} simulation_activity;


void robot_state_equation(const double* x, const double* u, double* dx);
void calculate_output(const double* x, double* y);
void robot_input_function(double t, double* u);

double state_der_wrapper(double tau);
void* simulation_task(void* arg);
void* input_sampling_task(void* arg);

shared_data* create_shared_data(const char* filename);
void destroy_shared_data(shared_data* data);
int run_multitask_simulation(const char* output_filename);

void sleep_ms(int milliseconds);
long get_current_time_ms(void);

#endif