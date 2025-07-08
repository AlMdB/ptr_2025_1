#ifndef ADT_MATRIX_H
#define ADT_MATRIX_H

#include <time.h>
#include "adt_string.h"
#include "adt_base.h"
#include <stdio.h>
#include <math.h>
#include <string.h>

#define sub_matrices(mt1,mtx2) add_matrix(mtx1,negate_matrix(mtx2))
#define get_val(mtx,row,col) ((double**) abase_data((mtx)->data))[(row)][(col)]

typedef struct Matrix{
    abase* data;
    unsigned int rows, cols;
}Matrix;

typedef double (*op_func)(double,double);

double sub_op(double,double);

double add_op(double,double);

double mult_op(double,double);

Matrix* scalar_add_mult_matrices(Matrix*mtx1, double val, op_func);

Matrix* zero_Matrix(unsigned int rows, unsigned int cols);

Matrix* create_Matrix(adt_string* input,unsigned int rows, unsigned int cols);

Matrix* random_Matrix(int rows,int cols);

Matrix* add_matrices(const Matrix* mtx1, const Matrix* mtx2);

Matrix* dot_product(const Matrix* mtx1, const Matrix* mtx2);

Matrix* negate_matrix(const Matrix* mtx);

void transpose_Matrix(Matrix* mtx);

Matrix* inverse_Matrix(const Matrix* mtx);

double determinant_Matrix(const Matrix* mtx);

void delete_Matrix(Matrix* mtx);

void print_matrix(const Matrix* mtx);

Matrix* identity_matrix(unsigned int rows, unsigned int cols);

//Matrix* adt_matrix_empty();

#endif