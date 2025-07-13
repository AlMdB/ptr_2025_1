#include "robot_system.h"

int main() {
    const char* bot_filename = "robot_simulation.txt";
    const char* sim_filename = "simulation_performance.txt";
    shared_data* sim_data = create_shared_data(bot_filename);
    int result = run_multitask_simulation(sim_data,sim_filename);
    
    if (result == 0) {
        printf("Python utilizado para o plot. Use o comando:\n");
        printf("python3 plot_results.py %s ou %s\n", bot_filename,sim_filename);
    }
    
    return result;
}