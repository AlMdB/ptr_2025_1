#include "adt_string.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>


adt_string *adt_string_empty(){
    return newADTS_str(NULL);
}

void append_char(adt_string* str, char c) {
    char char_str[2] = {c, '\0'};
    adt_string* char_adt_str = newADTS_str(char_str);
    adt_string* concat_result = concat_adt_string(str, char_adt_str, NULL);
    del_adt_string(str);
    del_adt_string(char_adt_str);
    *str = *concat_result;
    free(concat_result);
}


adt_string *newADTS_str(char *str) {
    size_t len = (str != NULL) ? strlen(str) : 0; 
    adt_string *new_str = malloc(sizeof(adt_string));
    if (new_str) {
        new_str->base = malloc(sizeof(abase));  
        if (new_str->base) {
            new_str->base->size = len * sizeof(char);
            new_str->base->data = malloc(len + 1);  
            if (new_str->base->data) {
                if (str != NULL) {
                    sprintf((char *)new_str->base->data, "%s", str);
                } else {
                    $(new_str)[0] = '\0';  
                }
                return new_str; 
            } else {
                free(new_str->base);
                free(new_str);
                new_str = NULL;
            }
        } else {
            free(new_str);
            new_str = NULL;
        }
    }
    return new_str;
}




adt_string *newADTS_char(char str_c) {
    adt_string *new_str = malloc(sizeof(adt_string));

    if (new_str) {
        new_str->base = malloc(sizeof(abase)); 
        if (new_str->base) {
            new_str->base->size = 2*sizeof(char);  
            new_str->base->data = malloc(new_str->base->size);  
            if (new_str->base->data) {
                ((char *)new_str->base->data)[0] = str_c;
                ((char *)new_str->base->data)[1] = '\0';
            } else {
                free(new_str->base);
                free(new_str);
                new_str = NULL;
            }
        } else {
            free(new_str);
            new_str = NULL;
        }
    }

    return new_str;
}

adt_string *newADTS_int(int str_i) {
    adt_string *new_str = malloc(sizeof(adt_string));

    if (new_str) {
        new_str->base = malloc(sizeof(abase));
        if (new_str->base) {
            new_str->base->size = 4*sizeof(int);  
            new_str->base->data = malloc(new_str->base->size);
            if (new_str->base->data) {
                sprintf((char *)new_str->base->data, "%d", str_i);
            } else {
                free(new_str->base);
                free(new_str);
                new_str = NULL;
            }
        } else {
            free(new_str);
            new_str = NULL;
        }
    }

    return new_str;
}

adt_string *newADTS_long(long str_l) {
    adt_string *new_str = malloc(sizeof(adt_string));

    if (new_str) {
        new_str->base = malloc(sizeof(abase));  
        if (new_str->base) {
            new_str->base->size = 4*sizeof(long);  
            new_str->base->data = malloc(new_str->base->size);  
            if (new_str->base->data) {
                sprintf((char *)new_str->base->data, "%ld", str_l);
            } else {
                free(new_str->base);
                free(new_str);
                new_str = NULL;
            }
        } else {
            free(new_str);
            new_str = NULL;
        }
    }

    return new_str;
}

adt_string *newADTS_float(float str_f) {
    adt_string *new_str = malloc(sizeof(adt_string));

    if (new_str) {
        new_str->base = malloc(sizeof(abase)); 
        if (new_str->base) {
            new_str->base->size = 4*sizeof(float);  
            new_str->base->data = malloc(new_str->base->size);
            if (new_str->base->data) {
                sprintf((char *)new_str->base->data, "%f", str_f);
            } else {
                free(new_str->base);
                free(new_str);
                new_str = NULL;
            }
        } else {
            free(new_str);
            new_str = NULL;
        }
    }

    return new_str;
}

adt_string *newADTS_double(double str_d) {
    adt_string *new_str = malloc(sizeof(adt_string));

    if (new_str) {
        new_str->base = malloc(sizeof(abase));
        if (new_str->base) {
            new_str->base->size = 4*sizeof(double); 
            new_str->base->data = malloc(new_str->base->size);
            if (new_str->base->data) {
                sprintf((char *)new_str->base->data, "%lf", str_d);
            } else {
                free(new_str->base);
                free(new_str);
                new_str = NULL;
            }
        } else {
            free(new_str);
            new_str = NULL;
        }
    }

    return new_str;
}

void del_adt_string(adt_string *ptr_str) {
    if (ptr_str) {
        remove_adt(ptr_str->base);
        free(ptr_str);
    }
}



adt_string* concat_adt_string(adt_string* s1, ...) {
    va_list args_list;
    char concat[999] = "";
    adt_string *new_str = malloc(sizeof(adt_string));
    va_start(args_list, s1);
    printf("entrando no loop de concat\n");
    while(s1 != NULL){
        printf("Primeira concatencao\n");
        printf("%s",$(s1));
        strcat(concat,$(s1));
        printf("Passou do concat\n");
        s1 = va_arg(args_list,adt_string*);
        printf("Concatencao de concat %s\n",concat);
    }
    va_end(args_list);
    printf("Conmcat final %s\n",concat);
    new_str->base = malloc(sizeof(abase));
    new_str->base->data = malloc(sizeof(char*)*strlen(concat)+1);
    sprintf($(new_str), "%s", concat);
    adt_string_size(new_str) = strlen(concat);
    return new_str;
}

