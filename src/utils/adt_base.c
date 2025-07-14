#include "adt_base.h"

abase* create_adt(void *data, size_t size){
    abase* new_adt = (abase*) malloc(sizeof(abase));
    if(new_adt == NULL){
        printf("Memory Allocation Fail.\n");
        return NULL;
    }
    new_adt->data = malloc(sizeof(data));
    if(new_adt->data == NULL){
	printf("Data Allocation Fail.\n");
	free(new_adt);
	return NULL;
    }
    memcpy(new_adt->data,data,size);
    new_adt->size = size;
    return new_adt;
}

void remove_adt(abase* node){
    //printf("Removing adt base Structs \n");
    if(node){
        //printf("Main Data Deallocation\n");
        free(node->data);
        free(node);
        //printf("Success Deallocation");
    }
    //printf("Remove done\n");
}
