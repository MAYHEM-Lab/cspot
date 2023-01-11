//
// Created by lukas on 09.01.2023.
//
#ifndef CSPOT_DFOPERATION_H
#define CSPOT_DFOPERATION_H

#include "dftype.h"

/**
 * TYPE - COMMAND_TYPE - COMMAND
 * XXX - XXX - XXX
 */
enum df_operations
{
    DF_SLOPPY_LOGIC_NOT = 1001001,
    DF_SLOPPY_LOGIC_AND = 1001002,
    DF_SLOPPY_LOGIC_OR = 1001003,
    DF_SLOPPY_LOGIC_XOR = 1001004,
    DF_SLOPPY_ARITH_ADDITION = 1002001,
    DF_SLOPPY_ARITH_SUBTRACTION = 1002002,
    DF_SLOPPY_ARITH_MULTIPLICATION = 1002003,
    DF_SLOPPY_ARITH_DIVISION = 1002004,
    DF_SLOPPY_ARITH_SQRT = 1002005,

    DF_BOOLEAN_LOGIC_NOT = 2001001,
    DF_BOOLEAN_LOGIC_AND = 2001002,
    DF_BOOLEAN_LOGIC_OR = 2001003,
    DF_BOOLEAN_LOGIC_XOR = 2001004,

    DF_BYTE_LOGIC_NOT = 3001001,
    DF_BYTE_LOGIC_AND = 3001002,
    DF_BYTE_LOGIC_OR = 3001003,
    DF_BYTE_LOGIC_XOR = 3001004,
    DF_BYTE_ARITH_ADDITION = 3002001,
    DF_BYTE_ARITH_SUBTRACTION = 3002002,
    DF_BYTE_ARITH_MULTIPLICATION = 3002003,
    DF_BYTE_ARITH_DIVISION = 3002004,
    DF_BYTE_ARITH_SQRT = 3002005,

    DF_SHORT_LOGIC_NOT = 4001001,
    DF_SHORT_LOGIC_AND = 4001002,
    DF_SHORT_LOGIC_OR = 4001003,
    DF_SHORT_LOGIC_XOR = 4001004,
    DF_SHORT_ARITH_ADDITION = 4002001,
    DF_SHORT_ARITH_SUBTRACTION = 4002002,
    DF_SHORT_ARITH_MULTIPLICATION = 4002003,
    DF_SHORT_ARITH_DIVISION = 4002004,
    DF_SHORT_ARITH_SQRT = 4002005,

    DF_INT_LOGIC_NOT = 5001001,
    DF_INT_LOGIC_AND = 5001002,
    DF_INT_LOGIC_OR = 5001003,
    DF_INT_LOGIC_XOR = 5001004,
    DF_INT_ARITH_ADDITION = 5002001,
    DF_INT_ARITH_SUBTRACTION = 5002002,
    DF_INT_ARITH_MULTIPLICATION = 5002003,
    DF_INT_ARITH_DIVISION = 5002004,
    DF_INT_ARITH_SQRT = 5002005,

    DF_LONG_LOGIC_NOT = 6001001,
    DF_LONG_LOGIC_AND = 6001002,
    DF_LONG_LOGIC_OR = 6001003,
    DF_LONG_LOGIC_XOR = 6001004,
    DF_LONG_ARITH_ADDITION = 6002001,
    DF_LONG_ARITH_SUBTRACTION = 6002002,
    DF_LONG_ARITH_MULTIPLICATION = 6002003,
    DF_LONG_ARITH_DIVISION = 6002004,
    DF_LONG_ARITH_SQRT = 6002005,

    DF_UNSIGNED_BYTE_LOGIC_NOT = 7001001,
    DF_UNSIGNED_BYTE_LOGIC_AND = 7001002,
    DF_UNSIGNED_BYTE_LOGIC_OR = 7001003,
    DF_UNSIGNED_BYTE_LOGIC_XOR = 7001004,
    DF_UNSIGNED_BYTE_ARITH_ADDITION = 7002001,
    DF_UNSIGNED_BYTE_ARITH_SUBTRACTION = 7002002,
    DF_UNSIGNED_BYTE_ARITH_MULTIPLICATION = 7002003,
    DF_UNSIGNED_BYTE_ARITH_DIVISION = 7002004,
    DF_UNSIGNED_BYTE_ARITH_SQRT = 7002005,

    DF_UNSIGNED_SHORT_LOGIC_NOT = 8001001,
    DF_UNSIGNED_SHORT_LOGIC_AND = 8001002,
    DF_UNSIGNED_SHORT_LOGIC_OR = 8001003,
    DF_UNSIGNED_SHORT_LOGIC_XOR = 8001004,
    DF_UNSIGNED_SHORT_ARITH_ADDITION = 8002001,
    DF_UNSIGNED_SHORT_ARITH_SUBTRACTION = 8002002,
    DF_UNSIGNED_SHORT_ARITH_MULTIPLICATION = 8002003,
    DF_UNSIGNED_SHORT_ARITH_DIVISION = 8002004,
    DF_UNSIGNED_SHORT_ARITH_SQRT = 8002005,

    DF_UNSIGNED_INT_LOGIC_NOT = 9001001,
    DF_UNSIGNED_INT_LOGIC_AND = 9001002,
    DF_UNSIGNED_INT_LOGIC_OR = 9001003,
    DF_UNSIGNED_INT_LOGIC_XOR = 9001004,
    DF_UNSIGNED_INT_ARITH_ADDITION = 9002001,
    DF_UNSIGNED_INT_ARITH_SUBTRACTION = 9002002,
    DF_UNSIGNED_INT_ARITH_MULTIPLICATION = 9002003,
    DF_UNSIGNED_INT_ARITH_DIVISION = 9002004,
    DF_UNSIGNED_INT_ARITH_SQRT = 9002005,

    DF_UNSIGNED_LONG_LOGIC_NOT = 10001001,
    DF_UNSIGNED_LONG_LOGIC_AND = 10001002,
    DF_UNSIGNED_LONG_LOGIC_OR = 10001003,
    DF_UNSIGNED_LONG_LOGIC_XOR = 10001004,
    DF_UNSIGNED_LONG_ARITH_ADDITION = 10002001,
    DF_UNSIGNED_LONG_ARITH_SUBTRACTION = 10002002,
    DF_UNSIGNED_LONG_ARITH_MULTIPLICATION = 10002003,
    DF_UNSIGNED_LONG_ARITH_DIVISION = 10002004,
    DF_UNSIGNED_LONG_ARITH_SQRT = 10002005,

    DF_DOUBLE_ARITH_ADDITION = 11002001,
    DF_DOUBLE_ARITH_SUBTRACTION = 11002002,
    DF_DOUBLE_ARITH_MULTIPLICATION = 11002003,
    DF_DOUBLE_ARITH_DIVISION = 11002004,
    DF_DOUBLE_ARITH_SQRT = 11002005,
};
typedef enum df_operations DF_OPERATION;

int df_operation(DF_OPERATION operation, const DF_VALUE values[], unsigned int value_count, DF_VALUE* return_value);

#endif // CSPOT_DFOPERATION_H