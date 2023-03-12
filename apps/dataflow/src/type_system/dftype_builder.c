//
// Created by lukas on 24.01.2023.
//
#include "dftype_builder.h"

#include <malloc.h>
#include <string.h>

DF_VALUE* build_boolean(int b) {
    DF_VALUE* value = malloc(sizeof(DF_VALUE));
    value->value.df_int = b;
    value->type = DF_BOOLEAN;
    return value;
}
DF_VALUE* build_byte(char b) {
    DF_VALUE* value = malloc(sizeof(DF_VALUE));
    value->value.df_int = (int)b;
    value->type = DF_BYTE;
    return value;
}
DF_VALUE* build_short(short s) {
    DF_VALUE* value = malloc(sizeof(DF_VALUE));
    value->value.df_int = s;
    value->type = DF_SHORT;
    return value;
}
DF_VALUE* build_integer(int i) {
    DF_VALUE* value = malloc(sizeof(DF_VALUE));
    value->value.df_int = i;
    value->type = DF_INTEGER;
    return value;
}
DF_VALUE* build_long(long l) {
    DF_VALUE* value = malloc(sizeof(DF_VALUE));
    value->value.df_long = l;
    value->type = DF_LONG;
    return value;
}
DF_VALUE* build_unsigned_byte(unsigned char b) {
    DF_VALUE* value = malloc(sizeof(DF_VALUE));
    value->value.df_unsigned_int = b;
    value->type = DF_UNSIGNED_BYTE;
    return value;
}
DF_VALUE* build_unsigned_short(unsigned short s) {
    DF_VALUE* value = malloc(sizeof(DF_VALUE));
    value->value.df_unsigned_int = s;
    value->type = DF_UNSIGNED_SHORT;
    return value;
}
DF_VALUE* build_unsigned_integer(unsigned int i) {
    DF_VALUE* value = malloc(sizeof(DF_VALUE));
    value->value.df_unsigned_int = i;
    value->type = DF_UNSIGNED_INTEGER;
    return value;
}
DF_VALUE* build_unsigned_long(unsigned long l) {
    DF_VALUE* value = malloc(sizeof(DF_VALUE));
    value->value.df_unsigned_long = l;
    value->type = DF_UNSIGNED_LONG;
    return value;
}
DF_VALUE* build_double(double d) {
    DF_VALUE* value = malloc(sizeof(DF_VALUE));
    value->value.df_double = d;
    value->type = DF_DOUBLE;
    return value;
}
DF_VALUE* build_datetime(char* d) {
    return NULL;
}
DF_VALUE* build_string(const char* s) {
    DF_VALUE* value = malloc(sizeof(DF_VALUE));
    const size_t string_size = strlen(s);

    DF_STORAGE_SYSTEM storage_system;
    uuid_generate(storage_system.id);

    DF_VALUE_ARRAY array = {
        .storage_system = storage_system,
        .type = DF_UNSIGNED_BYTE,
        .size = string_size,
        .value = s,
    };
    value->value.df_array = array;
    value->type = DF_STRING;
    return value;
}

DF_VALUE* build_array(DF_TYPE type, void* array, size_t size) {
    DF_VALUE* value = malloc(sizeof(DF_VALUE));

    DF_STORAGE_SYSTEM storage_system;
    uuid_generate(storage_system.id);

    DF_VALUE_ARRAY value_array = {
        .storage_system = storage_system,
        .type = type,
        .size = size,
        .value = array,
    };
    value->value.df_array = value_array;
    value->type = DF_ARRAY;
    return value;
}
DF_VALUE* build_list(DF_TYPE type, void* l) {
    return NULL;
}
DF_VALUE* build_stream(DF_TYPE type, void* s) {
    return NULL;
}
DF_VALUE* build_object() {
    return NULL;
}

void destroy_df_value(DF_VALUE* value) {
    free(value);
}
