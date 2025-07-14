#include "robot_system.h"


int main() {
    FILE* file = fopen("performance_metrics.txt", "w");
    if (file) fclose(file);

    printf("Simulacao sem carga comecando\n");
    shared_data* data_no_load = create_shared_data("output_no_txt.csv");
    run_multitask_simulation(data_no_load, "output_no_load.txt", 0);
    printf("Simulacao com carga comecando\n");

    shared_data* data_loaded = create_shared_data("output_loaded.csv");
    run_multitask_simulation(data_loaded, "output_loaded.txt", 1);
    printf("fim das operacoes use o comando python para ver mais sobre os dados\n");

    return 0;
}