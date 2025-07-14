#include "adt_matrix.h"

double sub_op(double num1, double num2){
    return num1 - num2;
}

double add_op(double num1,double num2){
    return num1 + num2;
};

double mult_op(double num1, double num2){
    return num1*num2;
}

Matrix* scalar_add_mult_matrices(Matrix*mtx, double val, op_func op){
    Matrix* new = zero_Matrix(mtx->rows,mtx->cols);
    if(new == NULL) {
        printf("Falha de memoria. Operacao scalar_add_mult\n");
        return NULL;
    }
    for(int row = 0 ; row < mtx->rows; row++){
        for(int col = 0; col < mtx->cols; col++){
            get_val(new,row,col) = op(get_val(mtx,row,col),val);  
        }
    }
    return new;
}


Matrix* create_Matrix(adt_string* input, unsigned int rows,unsigned int cols) {

    double** data = malloc(sizeof(double*) * rows);

    for (int row_i = 0; row_i < rows; row_i++) {
        data[row_i] = malloc(cols * sizeof(double));
        memset(data[row_i], 0, cols * sizeof(double));
    }

    char* inputStr = (char*)input->base->data; 
    char* token = strtok(inputStr, " ");


    for (int row_i = 0; row_i < rows; row_i++) {
        for (int col_j = 0; col_j < cols; col_j++) {
            if (token == NULL) {
                printf("\nQuantidade insuficiente de dados para criar matriz\n");
                for (int k = 0; k < row_i; k++) free(data[k]); 
                free(data);
                return NULL;
            }
            data[row_i][col_j] = atof(token);
            token = strtok(NULL, " ");
        }
    }

    abase* newBase = create_adt(data, sizeof(double**) * rows);
    free(data);
    
    Matrix* newMatrix = malloc(sizeof(Matrix));
    newMatrix->data = newBase;
    newMatrix->rows = rows;
    newMatrix->cols = cols;
    
    return newMatrix;
}


Matrix* random_Matrix(int rows,int cols){
    srand(time(NULL));
    double **data = malloc(sizeof(double*)*rows*cols);
    for (int row_i = 0; row_i < rows; row_i++){
        data[row_i] = malloc(cols* sizeof(double));
        for(int col_i = 0; col_i < cols; col_i++){
            double annex = (rand()%999)+1;
            data[row_i][col_i] = annex;
            printf("%lf %lf \n",annex,data[row_i][col_i]);
        }
    }
    abase* newBase = create_adt(data,sizeof(double**)*rows);
    free(data);
    Matrix* new = malloc(sizeof(Matrix));
    new->data = newBase;
    new->rows = rows;
    new->cols = cols;
    return new;
}

Matrix* zero_Matrix(unsigned int rows,unsigned int cols){
    double **data = malloc(sizeof(double*)*rows*cols);
    for (unsigned int i = 0; i < rows; i++){
        data[i] = malloc(cols* sizeof(double));
        for(unsigned int j = 0; j < cols; j++){
            data[i][j] = 0;
        }
    }
    abase* newBase = create_adt(data,sizeof(double**)*rows);
    free(data);
    Matrix* new = malloc(sizeof(Matrix));
    new->data = newBase;
    new->rows = rows;
    new->cols = cols;
    return new;
}


Matrix* add_matrices(const Matrix* mtx1, const Matrix* mtx2) {
    if (mtx1->rows != mtx2->rows || mtx1->cols != mtx2->cols) {
        printf("\noperação invalida. matrizes de tamanho diferente\n");
        return NULL;
    }

    Matrix* result = zero_Matrix(mtx1->rows,mtx1->cols);

    for (int i = 0; i < mtx1->rows; i++) {
        for (int j = 0; j < mtx1->cols; j++) {
            get_val(result,i,j) = add_op(get_val(mtx1,i,j),get_val(mtx2,i,j));  
            //((double**)result->data->data)[i][j] = ((double**)mtx1->data->data)[i][j] + ((double**)mtx2->data->data)[i][j];
        }
    }

    return result;
}


void transpose_Matrix(Matrix* mtx) {
    for (int i = 0; i < mtx->rows; i++) {
        for (int j = i + 1; j < mtx->cols; j++) {
            double temp = get_val(mtx,i,j);
            get_val(mtx,i,j) = get_val(mtx,j,i);
            get_val(mtx,i,j) = temp;
        }
    }
}


Matrix* dot_product(const Matrix* mtx1, const Matrix* mtx2) {
    if (mtx1->cols != mtx2->rows) {
        printf("\ntamanho erado para multiplicacao.\n");
        return NULL;
    }

    Matrix* result = zero_Matrix(mtx1->rows,mtx1->cols);

    for (int i = 0; i < mtx1->rows; i++) {
        for (int j = 0; j < mtx2->cols; j++) {
            double sum = 0.0;
            for (int k = 0; k < mtx1->cols; k++) { 
                sum += get_val(mtx1,i,k) * get_val(mtx2,k,j); 
            }
            get_val(result,i,j) = sum;
        }
    }

    return result;
}

Matrix* negate_matrix(const Matrix* mtx) {
    Matrix* result = zero_Matrix(mtx->rows,mtx->cols);

    for (int i = 0; i < mtx->rows; i++) {
        for (int j = 0; j < mtx->cols; j++) {
            get_val(mtx,i,j) = -get_val(mtx,i,j);
        }
    }

    return result;
}

void print_matrix(const Matrix* mtx) {
    for (int i = 0; i < mtx->rows; i++) {
        for (int j = 0; j < mtx->cols; j++) {
            printf(" %.2lf ", get_val(mtx,i,j));
        }
        printf("\n");
    }
    printf("\n");
}

Matrix* identity_matrix(unsigned int rows, unsigned int cols){
    Matrix* identity = zero_Matrix(rows,cols);
    for (int i = 0; i < rows; i++) {
        get_val(identity,i,i) = 1.0;
    }
    return identity;
}

Matrix* inverse_Matrix(const Matrix* mtx) {
    if (mtx->rows != mtx->cols) {
        printf("\nTamanho errado para inverter.\n");
        return NULL;
    }
    Matrix* identity = identity_matrix(mtx->rows,mtx->cols);
    Matrix* copy = zero_Matrix(mtx->rows,mtx->cols);
    for (int i = 0; i < mtx->rows; i++) {
        for (int j = 0; j < mtx->cols; j++) {
            get_val(copy,i,j) = get_val(mtx,i,j);
        }
    }

    for (int i = 0; i < mtx->rows; i++) {
        double pivot = get_val(copy,i,i);

        if (fabs(pivot) < 1e-10) {
            printf("\n nao inverti vel.\n");
            delete_Matrix(copy);
            delete_Matrix(identity);
            return NULL;
        }

        for (int j = 0; j < mtx->cols; j++) {
            get_val(copy,i,j) /= pivot;
            get_val(identity,i,j) /= pivot;
        }

        for (int k = 0; k < mtx->rows; k++) {
            if (k != i) {
                double factor = get_val(copy,k,i);
                for (int j = 0; j < mtx->cols; j++) {
                    get_val(copy,k,j) -= get_val(copy,i,j);
                    get_val(identity,k,j) -= get_val(identity,i,j);
                }
            }
        }
    }
    delete_Matrix(copy);
    return identity;
}

double determinant_Matrix(const Matrix* mtx) {
    if (mtx->rows != mtx->cols) {
        printf("\nTamanhos incorretos para determinante.\n");
        return NAN;
    }
    Matrix* copy = zero_Matrix(mtx->rows,mtx->cols);
    for (int i = 0; i < mtx->rows; i++) {
        for (int j = 0; j < mtx->cols; j++) {
            get_val(copy,i,j) = get_val(mtx,i,j);
        }
    }
    double det = 1.0;
    for (int i = 0; i < mtx->rows; i++) {
        double pivot = get_val(copy,i,i);
        if (fabs(pivot) < 1e-10) {
            det = 0.0;
            break;
        }
        for (int k = i + 1; k < mtx->rows; k++) {
            double factor = get_val(copy,k,i) / pivot;
            for (int j = i; j < mtx->cols; j++) {
                get_val(copy,k,j) -= factor * get_val(copy,i,j);
            }
        }
        det *= pivot;
    }
    delete_Matrix(copy);
    return det;
}

void delete_Matrix(Matrix* mtx) {
    //printf("Deletando Matriz\n");
    if(!mtx) return;
    for (int i = 0; i < mtx->rows; i++) {
        //printf("\nDeletando valores correspondentes a linhas %d\n",i);
        free(((double**)mtx->data->data)[i]);
    }
    printf("Deleta adt_base da matriz\n");
    remove_adt(mtx->data);
    free(mtx);
    printf("Fim do delete matrix\n");
}

