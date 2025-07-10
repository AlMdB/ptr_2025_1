#include "robot_system.h"

int main(int argc, char* argv[]) {
    const char* output_filename = "robot_simulation.txt";
    printf("se desejar outro nome para o arquiuvo. Coloque como argumento na chamada\n");
    if (argc > 1) {
        output_filename = argv[1];
    }
    int result = run_multitask_simulation(output_filename);
    
    if (result == 0) {
        printf("Python utilizado para o plot. Use o comando:\n");
        printf("python3 plot_results.py %s\n", output_filename);
    }
    
    return result;
}