#include "dfinterface.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uuid/uuid.h>

uuid_t run_uuid;

void init(const char* woof, int size) {
    uuid_generate(run_uuid);
    int err;
    char op_woof[4096] = "";
    char prog_woof[4096] = "";

    // [WOOF].dfprogram
    strncpy(prog_woof, woof, sizeof(prog_woof) - 1);
    strncat(prog_woof, ".dfprogram", sizeof(prog_woof) - strlen(prog_woof) - 1);

    // [WOOF].dfoperand
    strncpy(op_woof, woof, sizeof(prog_woof) - 1);
    strncat(op_woof, ".dfoperand", sizeof(op_woof) - strlen(op_woof) - 1);

    WooFInit(); /* attach local namespace for create*/

    err = WooFCreate(prog_woof, sizeof(DF_NODE), size);
    if (err < 0) {
        fprintf(stderr, "ERROR -- create of %s failed\n", prog_woof);
        exit(1);
    }

    err = WooFCreate(op_woof, sizeof(DF_OPERAND), size);
    if (err < 0) {
        fprintf(stderr, "ERROR -- create of %s failed\n", prog_woof);
        exit(1);
    }
}

void add_operand(const char* woof, const char* destination_node_id, int destination_node_port, double value) {
    unsigned long seqno;
    char op_woof[1024] = "";

    DF_OPERAND operand = {
        .value = value,
        .destination_node_id = "UNSET",
        .destination_port = destination_node_port,
        .prog_woof = "UNSET",
    };

    strncpy(operand.destination_node_id, destination_node_id, sizeof(operand.destination_node_id) - 1);
    strncat(operand.destination_node_id,
            "@",
            sizeof(operand.destination_node_id) - strlen(operand.destination_node_id) - 1);
    char uuid_string[37];
    uuid_unparse(run_uuid, uuid_string);
    strncat(operand.destination_node_id,
            uuid_string,
            sizeof(operand.destination_node_id) - strlen(operand.destination_node_id) - strlen(uuid_string) - 1);

    // [WOOF].dfprogram
    strncpy(operand.prog_woof, woof, sizeof(operand.prog_woof) - 1);
    strncat(operand.prog_woof, ".dfprogram", sizeof(operand.prog_woof) - strlen(operand.prog_woof) - 1);

    // [WOOF].dfoperand
    strncpy(op_woof, woof, sizeof(op_woof) - 1);
    strncat(op_woof, ".dfoperand", sizeof(op_woof) - strlen(op_woof) - 1);

    WooFInit(); /* local only for now */

    seqno = WooFPut(op_woof, "dfhandler", &operand);

    if (WooFInvalid(seqno)) {
        fprintf(stderr, "ERROR -- put to %s of operand with dest %s failed\n", op_woof, operand.destination_node_id);
        exit(1);
    }
}

void add_node(const char* woof,
              const char* node_id,
              int input_count,
              int opcode,
              const char* destination_node_id,
              int destination_node_port) {
    unsigned long seqno;
    char prog_woof[4096] = "";

    DF_NODE node = {.node_id = "UNSET",
                    .opcode = opcode,
                    .total_values_count = input_count,
                    .value_exist = {0},
                    .values = {0},
                    .destination_node_id = "UNSET",
                    .destination_port = destination_node_port,
                    .claim_input_port = -1,
                    .claim_input_value = 0.0,
                    .state = WAITING};

    char uuid_string[37];
    uuid_unparse(run_uuid, uuid_string);

    strncpy(node.node_id, node_id, sizeof(node.node_id) - 1);
    strncat(node.node_id, "@", sizeof(node.node_id) - strlen(node.node_id) - 1);
    strncat(node.node_id, uuid_string, sizeof(node.node_id) - strlen(node.node_id) - strlen(uuid_string) - 1);

    strncpy(node.destination_node_id, destination_node_id, sizeof(node.destination_node_id) - 1);
    strncat(node.destination_node_id, "@", sizeof(node.destination_node_id) - strlen(node.destination_node_id) - 1);
    strncat(node.destination_node_id,
            uuid_string,
            sizeof(node.destination_node_id) - strlen(node.destination_node_id) - strlen(uuid_string) - 1);

    // [WOOF].dfprogram
    strncpy(prog_woof, woof, sizeof(prog_woof) - 1);
    strncat(prog_woof, ".dfprogram", sizeof(prog_woof) - strlen(prog_woof) - 1);

    WooFInit(); /* local only for now */

    seqno = WooFPut(prog_woof, "dfnodehandler", &node);

    if (WooFInvalid(seqno)) {
        fprintf(stderr, "ERROR -- put to %s of node %s, opcode %d failed\n", prog_woof, node.node_id, node.opcode);
        exit(1);
    }
}

void reset_nodes() {
    uuid_generate(run_uuid);
}

int found_result(const char* woof, double result_value) {
    char op_woof[1024] = "";
    strncpy(op_woof, woof, sizeof(op_woof) - 1);
    strncat(op_woof, ".dfoperand", sizeof(op_woof) - strlen(op_woof) - 1);

    DF_OPERAND* operand = malloc(sizeof(DF_OPERAND));
    unsigned long latest_sequence_number = WooFGetLatestSeqno(op_woof);
    for (unsigned long sequence_number = latest_sequence_number; sequence_number > 0; sequence_number--) {
        WooFGet(op_woof, operand, sequence_number);
        if (fabs(operand->value - result_value) < 0.1) {
            return 1;
        }
    }
    return 0;
}