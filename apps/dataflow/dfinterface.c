#include "dfinterface.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void init(char* woof, int size) {
    int err;
    char op_woof[4096] = "";
    char prog_woof[4096] = "";

    // [WOOF].dfprogram
    strncpy(prog_woof, woof, sizeof(prog_woof));
    strncat(prog_woof, ".dfprogram", sizeof(prog_woof) - strlen(prog_woof) - 1);

    // [WOOF].dfoperand
    strncpy(op_woof, woof, sizeof(op_woof));
    strncat(op_woof, ".dfoperand", sizeof(op_woof) - strlen(op_woof) - 1);

    WooFInit(); /* attach local namespace for create*/

    err = WooFCreate(prog_woof, sizeof(DFNODE), size);
    if (err < 0) {
        fprintf(stderr, "ERROR -- create of %s failed\n", prog_woof);
        exit(1);
    }

    err = WooFCreate(op_woof, sizeof(DFOPERAND), size);
    if (err < 0) {
        fprintf(stderr, "ERROR -- create of %s failed\n", prog_woof);
        exit(1);
    }
}

void add_operand(char* woof, int destination_node_id, int destination_node_port, double value) {
    unsigned long seqno;
    char op_woof[1024] = "";

    DFOPERAND operand = {
        .value = value,
        .destination_node_id = destination_node_id,
        .destination_port = destination_node_port,
        .prog_woof = "",
    };

    // [WOOF].dfprogram
    strncpy(operand.prog_woof, woof, sizeof(operand.prog_woof));
    strncat(operand.prog_woof, ".dfprogram", sizeof(operand.prog_woof) - strlen(operand.prog_woof) - 1);

    // [WOOF].dfoperand
    strncpy(op_woof, woof, sizeof(op_woof));
    strncat(op_woof, ".dfoperand", sizeof(op_woof) - strlen(op_woof) - 1);

    WooFInit(); /* local only for now */

    seqno = WooFPut(op_woof, "dfhandler", &operand);

    if (WooFInvalid(seqno)) {
        fprintf(stderr, "ERROR -- put to %s of operand with dest %d failed\n", op_woof, operand.destination_node_id);
        exit(1);
    }
}

void add_node(
    char* woof, int node_id, int input_count, int opcode, int destination_node_id, int destination_node_port) {
    unsigned long seqno;
    char prog_woof[4096] = "";

    DFNODE node = {.node_id = node_id,
                   .opcode = opcode,
                   .total_values_count = input_count,
                   .value_exist = {0},
                   .values = {0},
                   .destination_node_id = destination_node_id,
                   .destination_port = destination_node_port,
                   .claim_input_port = -1,
                   .claim_input_value = 0.0,
                   .state = WAITING};

    // [WOOF].dfprogram
    strncpy(prog_woof, woof, sizeof(prog_woof));
    strncat(prog_woof, ".dfprogram", sizeof(prog_woof) - strlen(prog_woof) - 1);

    WooFInit(); /* local only for now */

    seqno = WooFPut(prog_woof, "dfnodehandler", &node);

    if (WooFInvalid(seqno)) {
        fprintf(stderr, "ERROR -- put to %s of node %d, opcode %d failed\n", prog_woof, node.node_id, node.opcode);
        exit(1);
    }
}

void* get_result(char* woof, int id) {
    DFNODE* node = (DFNODE*)malloc(sizeof(DFNODE));
    WooFGet(woof, node, id);
    return node->values;
}