#ifndef DF_H
#define DF_H

#define VALUE_SIZE 10
#define ID_SIZE 64

// opcodes

#define ADD (1)
#define SUB (2)
#define MUL (3)
#define DIV (4)
#define SQR (5)

enum df_node_state_enum
{
    WAITING,
    CLAIM,
    PARTIAL,
    DONE
};
typedef enum df_node_state_enum DF_NODE_STATE;

struct df_node_stc {
    DF_NODE_STATE state;                   // the state of the node
    char node_id[ID_SIZE];                 // which node is it
    int opcode;                            // this op code
    unsigned int total_values_count;       // count of total values node has to receive
    unsigned char value_exist[VALUE_SIZE]; // count of values received by node
    double values[VALUE_SIZE];             // array of input values, mem alloc at runtime (only for partials)
    char destination_node_id[ID_SIZE];     // next node address
    int destination_port;                  // next node port
    double claim_input_value;              // single value used in claims received by operand
    int claim_input_port;                  // the port of the single value in claims received by operand
};
typedef struct df_node_stc DF_NODE;

struct df_operand_stc {
    double value;                      // operand value
    char destination_node_id[ID_SIZE]; // dest node address
    int destination_port;              // dest node's port address
    char prog_woof[1024];              // name of woof holding the program
};
typedef struct df_operand_stc DF_OPERAND;

extern double DFOperation(int opcode, double* values, unsigned int size);

#endif
