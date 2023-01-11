#include "dfoperation.h"

#include <math.h>

int df_logic_operation(const DF_OPERATION arithmetic_operation,
                       const DF_VALUE values[],
                       const unsigned int value_count,
                       DF_VALUE* return_value) {
    // TODO
    return 0;
}

int df_arithmetic_operation(const DF_OPERATION arithmetic_operation,
                            const DF_VALUE values[],
                            const unsigned int value_count,
                            DF_VALUE* return_value) {
    double result;
    switch (arithmetic_operation) {
    case DF_DOUBLE_ARITH_ADDITION:
        result = 0;
        for (unsigned int i = 0; i < value_count; i++) {
            result += values[i].value.df_double;
        }
        break;
    case DF_DOUBLE_ARITH_SUBTRACTION:
        result = 0;
        for (unsigned int i = 0; i < value_count; i++) {
            result -= values[i].value.df_double;
        }
        break;
    case DF_DOUBLE_ARITH_MULTIPLICATION:
        result = 1;
        for (unsigned int i = 0; i < value_count; i++) {
            result *= values[i].value.df_double;
        }
        break;
    case DF_DOUBLE_ARITH_DIVISION:
        result = values[0].value.df_double;
        for (unsigned int i = 1; i < value_count; i++) {
            result /= values[i].value.df_double;
        }
        break;
    case DF_DOUBLE_ARITH_SQRT:
        result = sqrt(values[0].value.df_double);
        break;
    default:
        return 1;
    }
    return_value->type = DF_DOUBLE;
    return_value->value.df_double = result;
    return 0;
}


int df_operation(const DF_OPERATION operation,
                 const DF_VALUE values[],
                 const unsigned int value_count,
                 DF_VALUE* return_value) {
    // TODO
    switch (operation) {
    case DF_SLOPPY_LOGIC_NOT:
        return df_logic_operation(operation, values, value_count, return_value);
    case DF_DOUBLE_ARITH_ADDITION:
    case DF_DOUBLE_ARITH_SUBTRACTION:
    case DF_DOUBLE_ARITH_MULTIPLICATION:
    case DF_DOUBLE_ARITH_DIVISION:
    case DF_DOUBLE_ARITH_SQRT:
        return df_arithmetic_operation(operation, values, value_count, return_value);
    default:
        return 1;
    }
}
