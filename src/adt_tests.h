#ifndef ADT_TESTS_H
#define ADT_TESTS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "adt_matrix.h"
#include "adt_integral.h"
#include "adt_string.h"

typedef struct {
    int test_type;
    char data[1000];
    int rows, cols;
    double scalar_value;
    char description[200];
} TestData;

// Utility functions
void clrscr(void);
void print_menu(void);

int read_test_file(const char* filename, TestData* tests, int max_tests);
void test_matrix_from_file(TestData* test);
void test_string_from_file(TestData* test);
void test_integral_from_file(TestData* test);
void run_file_tests(const char* filename);

void auto_matrix_test(void);
void auto_string_test(void);
int auto_test_integral_function(void);

int testing_matrix_function(void);
int testing_string_function(void);


double quadratic_test(double x);

#endif