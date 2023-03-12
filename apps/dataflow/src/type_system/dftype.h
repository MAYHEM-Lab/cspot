//
// Created by lukas on 09.01.2023.
//
#ifndef CSPOT_DFTYPE_H
#define CSPOT_DFTYPE_H

#include <uuid/uuid.h>

enum df_types_enum
{
    DF_UNKNOWN,

    // Simple Types
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
    DF_DATETIME,

    // Compound Types
    DF_STRING,
    DF_ARRAY,
    DF_LIST,
    DF_STREAM,
    DF_DATA_OBJECT,
    // TODO
};
typedef enum df_types_enum DF_TYPE;


struct df_storage_system {
    uuid_t id;
};
typedef struct df_storage_system DF_STORAGE_SYSTEM;


/* **************** ARRAY **************** */
struct df_value_array {
    struct df_storage_system storage_system;
    size_t size;
    enum df_types_enum type;
    const void* value;
};
typedef struct df_value_array DF_VALUE_ARRAY;


/* **************** RECORD **************** */
struct df_record_element {
    char identifier[21];
    enum df_types_enum type;
    struct df_types_struct* value;
};
typedef struct df_record_element DF_RECORD_ELEMENTS;

struct df_value_record {
    struct df_storage_system storage_system;
    struct df_record_element element;
};
typedef struct df_value_record DF_VALUE_RECORD;


/* **************** NODE VALUE **************** */
union df_values_union {
    unsigned int df_unsigned_int;
    unsigned long df_unsigned_long;
    int df_int;
    long df_long;
    double df_double;
    struct df_value_array df_array;
    struct df_value_record df_object;
};
typedef union df_values_union DF_TYPE_VALUE;

struct df_types_struct {
    union df_values_union value;
    enum df_types_enum type;
};
typedef struct df_types_struct DF_VALUE;


int write_value(const DF_VALUE* value);
int read_value(const DF_VALUE* value);


#endif // CSPOT_DFTYPE_H
