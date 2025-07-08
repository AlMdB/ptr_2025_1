#ifndef _DSTRING_H
#define _DSTRING_H

#include "adt_base.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>




typedef struct adt_string{
    abase* base;
}adt_string;


#define adt_string_size(aString) ((aString)->base->size)
#define $(aString) ((char*)(aString->base->data))
#define $$(aString) ((char*)((adt_string*)(aString))->base->data)


#define new_adt_string _Generic((I), \
    char*  : new_adt_string_str, \
    char   : new_adt_string_char, \
    int    : new_adt_string_int,   \
    long   : new_adt_string_long,  \
    float  : new_adt_string_float,  \
    double : new_adt_string_double  \
    )


adt_string *adt_string_empty();
void append_char(adt_string* str, char c);
adt_string *newADTS_str(char *str);
adt_string *newADTS_char(char c);
adt_string *newADTS_int(int i);    
adt_string *newADTS_long(long l);
adt_string *newADTS_float(float f);
adt_string *newADTS_double(double d);

void del_adt_string(adt_string *pt);

adt_string* concat_adt_string(adt_string* s1, ...);

#endif