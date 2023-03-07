//
// Created by lukas on 24.01.2023.
//

#ifndef CSPOT_DFTYPE_BUILDER_H
#define CSPOT_DFTYPE_BUILDER_H

#include "dftype.h"


DF_VALUE* build_boolean(int b);
DF_VALUE* build_byte(char b);
DF_VALUE* build_short(short s);
DF_VALUE* build_integer(int i);
DF_VALUE* build_long(long l);
DF_VALUE* build_unsigned_byte(unsigned char b);
DF_VALUE* build_unsigned_short(unsigned short s);
DF_VALUE* build_unsigned_integer(unsigned int i);
DF_VALUE* build_unsigned_long(unsigned long l);
DF_VALUE* build_double(double d);
DF_VALUE* build_string(char* s);
DF_VALUE* build_datetime(char* d);

DF_VALUE* build_array(DF_TYPE type, void* array, size_t size);
DF_VALUE* build_list(DF_TYPE type, void* l);
DF_VALUE* build_stream(DF_TYPE type, void* s);
DF_VALUE* build_object();


void destroy_df_value(DF_VALUE* value);

#endif // CSPOT_DFTYPE_BUILDER_H
