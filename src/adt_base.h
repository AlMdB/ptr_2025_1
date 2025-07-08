#ifndef ADT_BASE_H
#define ADT_BASE_H


#include <stdlib.h>

#define abase_data(adt) ((adt)->data) 
#define abase_size(adt)  ((adt)->size)


typedef struct adt_base{
    void* data;
    size_t size;
}abase;

abase* create_adt(void *data, size_t size);
void remove_adt(abase* node);


#endif