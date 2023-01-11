//
// Created by lukas on 09.01.2023.
//
#ifndef CSPOT_DFTYPE_H
#define CSPOT_DFTYPE_H

union df_types_union {
    unsigned int df_unsigned_int;
    unsigned long df_unsigned_long;
    int df_int;
    long df_long;
    double df_double;
    char* df_string;
    void* df_object;
};
typedef union df_types_union DF_TYPE_VALUE;

enum df_types_enum
{
    DF_UNKNOWN,
    DF_BOOLEAN,
    DF_BYTE,
    DF_SHORT,
    DF_INTEGER,
    DF_LONG,
    DF_UNSIGNED_BYTE,
    DF_UNSIGNED_SHORT,
    DF_UNSIGNED_INTEGER,
    DF_UNSIGNED_LONG,
    DF_DOUBLE,
    DF_STRING,
    DF_DATETIME,
    // TODO
};
typedef enum df_types_enum DF_TYPE;

struct df_types_struct {
    DF_TYPE_VALUE value;
    DF_TYPE type;
};
typedef struct df_types_struct DF_VALUE;

#endif // CSPOT_DFTYPE_H
