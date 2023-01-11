#ifndef DF_H
#define DF_H

#include "dfoperation.h"
#include "dftype.h"

enum df_node_state_enum
{
    WAITING,
    CLAIM,
    PARTIAL,
    DONE
};
typedef enum df_node_state_enum DF_NODE_STATE;

struct df_node_stc {
    DF_NODE_STATE state;        // the state of the node
    int node_id;                // which node is it
    DF_OPERATION operation;     // this op code
    int total_values_count;     // count of total values node has to receive
    int received_values_count;  // count of values received by node
    DF_VALUE values[10];        // array of input values, mem alloc at runtime (only for partials)
    int destination_node_id;    // next node address
    int destination_port;       // next node port
    DF_VALUE claim_input_value; // single value used in claims received by operand
    int claim_input_port;       // the port of the single value in claims received by operand
};
typedef struct df_node_stc DF_NODE;

struct df_operand_stc {
    DF_VALUE value;          // operand value
    int destination_node_id; // dest node address
    int destination_port;    // dest node's port address
    char prog_woof[1024];    // name of woof holding the program
};
typedef struct df_operand_stc DF_OPERAND;

#endif
