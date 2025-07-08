#include "adt_tests.h"

void clrscr() {
    system("@cls||clear");
}

void print_menu() {
    printf("Escolha o algoritmo para teste:\n");
    printf("1. ADT das matrizes\n");
    printf("2. ADT das integrais\n");
    printf("3. ADT das Strings\n");
    printf("4. Teste automatico por arquivo\n");
    printf("0. Fechar o programa\n");
}

int read_test_file(const char* filename, TestData* tests, int max_tests) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Erro: Não foi possível abrir o arquivo %s\n", filename);
        return 0;
    }
    
    int count = 0;
    char line[1200];
    
    while (fgets(line, sizeof(line), file) && count < max_tests) {
        if (line[0] == '\n' || line[0] == '#') continue;
        
        char* token = strtok(line, "|");
        if (!token) continue;
        
        tests[count].test_type = atoi(token);
        
        token = strtok(NULL, "|");
        if (!token) continue;
        strcpy(tests[count].description, token);
        
        token = strtok(NULL, "|");
        if (!token) continue;
        strcpy(tests[count].data, token);
        
        token = strtok(NULL, "|");
        if (token) tests[count].rows = atoi(token);
        
        token = strtok(NULL, "|");
        if (token) tests[count].cols = atoi(token);
        
        token = strtok(NULL, "|");
        if (token) tests[count].scalar_value = atof(token);
        
        count++;
    }
    
    fclose(file);
    return count;
}

void test_matrix_from_file(TestData* test) {
    printf("\n=== TESTE DE MATRIZ: %s ===\n", test->description);
    
    Matrix* matrix1 = NULL;
    Matrix* matrix2 = NULL;
    
    if (strcmp(test->data, "RANDOM") == 0) {
        matrix1 = random_Matrix(test->rows, test->cols);
        matrix2 = random_Matrix(test->rows, test->cols);
        printf("Matrizes randômicas criadas %dx%d:\n", test->rows, test->cols);
    } else {
        adt_string *matrix_data = newADTS_str(test->data);
        matrix1 = create_Matrix(matrix_data, test->rows, test->cols);
        matrix2 = random_Matrix(test->rows, test->cols);
        del_adt_string(matrix_data);
        printf("Matriz criada a partir dos dados:\n");
    }
    
    if (matrix1) {
        printf("Matriz 1:\n");
        print_matrix(matrix1);
        
        if (matrix2) {
            printf("\nMatriz 2:\n");
            print_matrix(matrix2);
            
            Matrix* sum = add_matrices(matrix1, matrix2);
            if (sum) {
                printf("\nSOMA:\n");
                print_matrix(sum);
                delete_Matrix(sum);
            }
            
            Matrix* product = dot_product(matrix1, matrix2);
            if (product) {
                printf("\nPRODUTO:\n");
                print_matrix(product);
                delete_Matrix(product);
            }
        }
        
        if (test->scalar_value != 0.0) {
            Matrix* copy = random_Matrix(test->rows, test->cols);
            scalar_add_mult_matrices(copy, test->scalar_value,mult_op);
            printf("\nPRODUTO ESCALAR (%.2f):\n", test->scalar_value);
            print_matrix(copy);
            delete_Matrix(copy);
        }
        
        Matrix* transpose_copy = random_Matrix(test->rows, test->cols);
        transpose_Matrix(transpose_copy);
        printf("\nTRANSPOSTA:\n");
        print_matrix(transpose_copy);
        delete_Matrix(transpose_copy);
        
        if (test->rows == test->cols) {
            double det = determinant_Matrix(matrix1);
            printf("\nDETERMINANTE: %.6f\n", det);
        }
        
        delete_Matrix(matrix1);
    }
    
    if (matrix2) {
        delete_Matrix(matrix2);
    }
    
    printf("\n=== FIM DO TESTE ===\n\n");
}

void test_string_from_file(TestData* test) {
    printf("\n=== TESTE DE STRING: %s ===\n", test->description);
    
    adt_string* adt_s1 = NULL;
    
    int string_type = (int)test->scalar_value;
    
    switch (string_type) {
        case 1:
            adt_s1 = newADTS_str(test->data);
            printf("String criada: %s\n", $(adt_s1));
            break;
        case 2:
            if (strlen(test->data) > 0) {
                adt_s1 = newADTS_char(test->data[0]);
                printf("Char criado: %s\n", $(adt_s1));
            }
            break;
        case 3:
            adt_s1 = newADTS_float(atof(test->data));
            printf("Float criado: %s\n", $(adt_s1));
            break;
        case 4:
            adt_s1 = newADTS_double(atof(test->data));
            printf("Double criado: %s\n", $(adt_s1));
            break;
        case 5:
            adt_s1 = newADTS_long(atol(test->data));
            printf("Long criado: %s\n", $(adt_s1));
            break;
        case 6:
            adt_s1 = newADTS_int(atoi(test->data));
            printf("Int criado: %s\n", $(adt_s1));
            break;
        default:
            printf("Tipo de string inválido\n");
            return;
    }
    
    if (adt_s1) {
        adt_string* test_str = newADTS_str(" [CONCATENADO]");
        adt_string* concat_result = concat_adt_string(adt_s1, test_str, NULL);
        
        printf("Resultado da concatenação: %s\n", $(concat_result));
        
        del_adt_string(adt_s1);
        del_adt_string(test_str);
        del_adt_string(concat_result);
    }
    
    printf("=== FIM DO TESTE ===\n\n");
}

void test_integral_from_file(TestData* test) {
    printf("\n=== TESTE DE INTEGRAL: %s ===\n", test->description);
    printf("Dados: %s\n", test->data);
    printf("Valor escalar: %.6f\n", test->scalar_value);
    printf("=== FIM DO TESTE ===\n\n");
}

void run_file_tests(const char* filename) {
    TestData tests[100];
    int test_count = read_test_file(filename, tests, 100);
    
    if (test_count == 0) {
        printf("Nenhum teste foi carregado do arquivo.\n");
        return;
    }
    
    printf("Carregados %d testes do arquivo %s\n\n", test_count, filename);
    
    for (int i = 0; i < test_count; i++) {
        printf("Executando teste %d/%d...\n", i + 1, test_count);
        
        switch (tests[i].test_type) {
            case 1:
                test_matrix_from_file(&tests[i]);
                break;
            case 2:
                test_integral_from_file(&tests[i]);
                break;
            case 3:
                test_string_from_file(&tests[i]);
                break;
            default:
                printf("Tipo de teste desconhecido: %d\n", tests[i].test_type);
                break;
        }
        
        printf("Pressione Enter para continuar...");
        getchar();
    }
}

void auto_matrix_test(){
    Matrix* matrix1 = random_Matrix(3, 3);
    Matrix* matrix2 = random_Matrix(3, 3);

    printf("Matrix 1:\n");
    print_matrix(matrix1);
    printf("\nMatrix 2:\n");
    print_matrix(matrix2);

    Matrix* sum = add_matrices(matrix1, matrix2);
    printf("\nSOMA:\n");
    print_matrix(sum);

    Matrix* product = dot_product(matrix1, matrix2);
    printf("\nPRODUTO:\n");
    print_matrix(product);

    scalar_add_mult_matrices(matrix1, 2.0,mult_op);
    printf("\nPRODUTO ESCALAR NA MATRIX 1:\n");
    print_matrix(matrix1);

    scalar_add_mult_matrices(matrix2, 3.0,add_op);
    printf("\nADIÇÃO ESCALAR NA MATRIX 2:\n");
    print_matrix(matrix2);

    transpose_Matrix(matrix1);
    printf("\nTRANSPOSTA 1:\n");
    print_matrix(matrix1);

    Matrix* identity = inverse_Matrix(matrix1);
    printf("\nINVERSA 1:\n");
    print_matrix(identity);

    double determinant = determinant_Matrix(matrix1);
    printf("\nDETERMINANTE DA PRIMEIRA: %lf\n", determinant);

    delete_Matrix(matrix1);
    delete_Matrix(matrix2);
    delete_Matrix(sum);
    delete_Matrix(product);
    delete_Matrix(identity);
}

void auto_string_test(){
    adt_string *adt_s1 = newADTS_str("Somando com um float");
    printf("%s\n",$(adt_s1));
    adt_string *adt_s2 = newADTS_float(2.1674);
    printf("%s\n",$(adt_s2));
    adt_string *concat_st = concat_adt_string(adt_s1,adt_s2,NULL);
    printf("%s\n",$(concat_st));
    printf("A concatenação da string : %s com o float %s é igual a : %s\n",$(adt_s1),$(adt_s2),$(concat_st));
    del_adt_string(adt_s1);
    del_adt_string(adt_s2);
    del_adt_string(concat_st);
}

int testing_matrix_function() {
    int rows, cols;
    printf("Input as linhas e colunas da matriz: ");
    scanf("%d %d", &rows, &cols);

    int choice = 0;

    printf("\n1. Para criar a própria matriz\n");
    printf("2. Para criar a matriz randômica\n");
    printf("3. Test Automático\n");

    printf("Escolha: ");
    scanf("%d", &choice);

    Matrix *matrix;
    switch (choice) {
        case 1:
            printf("Digite os valores da matriz separados por espaço: ");
            char entrada[1000];
            scanf("%s", entrada); 
            adt_string *entrada_matrix = newADTS_str(entrada);
            matrix = create_Matrix(entrada_matrix, rows, cols);
            printf("Matriz criada:\n");
            print_matrix(matrix);
            del_adt_string(entrada_matrix);
            break;
        case 2:
            matrix = random_Matrix(rows, cols);
            printf("Matriz randômica criada:\n");
            print_matrix(matrix);
            break;
        case 3:
            auto_matrix_test();
            return 0;
        default:
            printf("Escolha inválida\n");
            return 0;
    }
    
    if (matrix != NULL) {
        delete_Matrix(matrix);
    }
    return 0; 
}

int testing_string_function() {
    printf("Função de teste para Strings\n");
    int loop1 = 1;
    while (loop1) {
        printf("Qual tipo de string deseja testar?\n");
        printf("1. String\n");
        printf("2. Char\n");
        printf("3. Float\n");
        printf("4. Double\n");
        printf("5. Long\n");
        printf("6. Int\n");
        printf("7. Teste automatico das strings\n");
        printf("0. Sair\n");

        int choice;
        printf("Escolha: ");
        scanf("%d", &choice);

        adt_string* adt_s1;
        switch (choice) {
            case 1:
                printf("\nEscreva uma String\n");
                char test_string[1000];
                scanf("%s", test_string);
                adt_s1 = newADTS_str(test_string);
                printf("String: %s\n", $(adt_s1));
                del_adt_string(adt_s1);
                break;
            case 2:
                printf("\nEscreva um char\n");
                char test_char;
                scanf(" %c", &test_char);
                adt_s1 = newADTS_char(test_char);
                printf("Char: %s\n", $(adt_s1));
                del_adt_string(adt_s1);
                break;
            case 3:
                printf("\nEscreva um numero float\n");
                float test_fl;
                scanf("%f", &test_fl);
                adt_s1 = newADTS_float(test_fl);
                printf("Float: %s\n", $(adt_s1));
                del_adt_string(adt_s1);
                break;
            case 4:
                printf("\nEscreva um numero double\n");
                double test_double;
                scanf("%lf", &test_double);
                adt_s1 = newADTS_double(test_double);
                printf("Double: %s\n", $(adt_s1));
                del_adt_string(adt_s1);
                break;
            case 5:
                printf("\nEscreva um numero long\n");
                long test_long;
                scanf("%ld", &test_long);
                adt_s1 = newADTS_long(test_long);
                printf("Long: %s\n", $(adt_s1));
                del_adt_string(adt_s1);
                break;
            case 6:
                printf("\nEscreva um numero int\n");
                int test_int;
                scanf("%d", &test_int);
                adt_s1 = newADTS_int(test_int);
                printf("Int: %s\n", $(adt_s1));
                del_adt_string(adt_s1);
                break;
            case 7:
                auto_string_test();
                break;
            case 0:
                loop1 = 0;
                break;
            default:
                printf("Escolha inválida\n");
                break;
        }
    }

    return 0;
}


int auto_test_integral_function() {
    double a = 0.0;
    double b = 7.0; 
    int n = 1000;
    double exact = 1.0 / 3.0;

    double riemann_result = riemann_sum(quadratic_test, a, b,n);
    double mid_result = mid_point(quadratic_test, a, b,n);
    double trap_result = trapez(quadratic_test, a, b,n);
    double simp_result = simpson(quadratic_test, a, b,n);
    double gauss_result = quad_gauss(quadratic_test, a, b);
    
    printf("Soma Riemman: %.3f | erro : %.3e\n",riemann_result, fabs(riemann_result - exact));
    printf("Ponto Medio: %.3f | erro : %.3e\n",mid_result, fabs(mid_result - exact));
    printf("Regra Trapeizodal: %.3f | erro : %.3e\n",trap_result, fabs(trap_result - exact));
    printf("Regra de simpson: %.3f | erro : %.3e\n",simp_result, fabs(simp_result - exact));
    printf("Quadratura Gaussiana: %.3f | erro : %.3e\n",gauss_result, fabs(gauss_result - exact));
    return 0; 
}
