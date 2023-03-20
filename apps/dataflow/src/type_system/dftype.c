//
// Created by lukas on 24.01.2023.
//
#include "dftype.h"

#define PRIMITIVE_WOOF_PREFIX "primitive"

int is_primitive(const DF_VALUE* value) {
    return value->type == DF_BOOLEAN || value->type == DF_BYTE || value->type == DF_SHORT ||
           value->type == DF_INTEGER || value->type == DF_LONG || value->type == DF_UNSIGNED_BYTE ||
           value->type == DF_UNSIGNED_SHORT || value->type == DF_UNSIGNED_INTEGER || value->type == DF_UNSIGNED_LONG ||
           value->type == DF_DOUBLE || value->type == DF_DATETIME;
}

unsigned long write_value_with_handler(const char* woof, const char* handler, const DF_VALUE* value) {
    if (is_primitive(value)) {
        unsigned long sequence_number = WooFPut(woof, handler, value);
        if (WooFInvalid(sequence_number)) {
            return sequence_number;
        }
        return sequence_number;
    } else {
        if (value->type == DF_ARRAY) {
            struct df_value_array array = value->value.df_array;
        }
        // TODO
    }
}

unsigned long write_value(const char* woof, const DF_VALUE* value) {
    return write_value_with_handler(woof, NULL, value);
}

unsigned long read_value(const char* woof, const unsigned long sequence_number, DF_VALUE* value) {
    int status = WooFGet(woof, value, sequence_number);
    if (WooFInvalid(status)) {
        return status;
    }
    if (is_primitive(value)) {
        return status;
    } else {
        // TODO
        return status;
    }
}