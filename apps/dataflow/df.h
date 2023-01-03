#ifndef DF_H
#define DF_H

// opcodes

#define ADD (1)
#define SUB (2)
#define MUL (3)
#define DIV (4)
#define SQR (5)

#define WAITING (0)
#define CLAIM (1)
#define PARTIAL (2)
#define DONE (3)

struct df_node_stc {
    int node_no;       // which node is it
    int opcode;        // this op code
    int total_val_ct;  // count of total values node has to receive
    int recvd_val_ct;  // count of values received by node
    double ip_value;   // single value used in claims received by operand
    int ip_port;       // the port of the single value in claims received by operand
    double values[10]; // array of input values, mem alloc at runtime (only for partials)
    int dst_no;        // next node address
    int dst_port;      // next node port
    int state;         // the state of the node
};

typedef struct df_node_stc DFNODE;

struct df_operand_stc {
    double value;         // operand value
    int dst_no;           // dest node address
    int dst_port;         // dest node's port address
    char prog_woof[1024]; // name of woof holding the program
};

typedef struct df_operand_stc DFOPERAND;

extern double DFOperation(int opcode, double* values, int size);

#endif
