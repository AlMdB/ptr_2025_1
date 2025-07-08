#include <stdio.h>
#include <stdlib.h>
#include "adt_matrix.h"
#include "adt_integral.h"
#include "adt_string.h"
#include "adt_tests.h"


int main() {
    int choice = 7;
    int inside_loop_function = 1;

    while (choice != 0) {
        print_menu();
        printf("Digite sua escolha: ");
        scanf("%d", &choice);
        clrscr();

        switch (choice) {
            case 1:
                inside_loop_function = 1;
                while (inside_loop_function) {
                    inside_loop_function = testing_matrix_function();
                }
                break;
            case 2:
                inside_loop_function = 1;
                while (inside_loop_function) {
                    inside_loop_function = auto_test_integral_function();
                }
                break;
            case 3:
                inside_loop_function = 1;
                while (inside_loop_function) {
                    inside_loop_function = testing_string_function();
                }
                break;
            case 4:
                run_file_tests("text_data.txt");
                break;
            case 0:
                printf("SAIR DO PROGRAMA\n");
                break;
            default:
                printf("ESCOLHA INVÁLIDA\n");
                break;
        }
    }

    return 0;
}
