#include "../df.h"
#include "../dflogger.h"
#include "../woofc.h"

#include <malloc.h>

unsigned int get_received_values_count(DF_NODE* node);

int main() {

    WooFInit();
    printf("STACK DEBUG\n");
    char* prog = "test.dfprogram";

    log_debug("[woof: %s] CURRENT STACK:", prog);
    unsigned long latest_sequence_number = WooFGetLatestSeqno(prog);
    for (unsigned long sequence_index = 1; sequence_index <= latest_sequence_number; sequence_index++) {
        DF_NODE debug_node;

        WooFGet(prog, &debug_node, sequence_index);

        log_debug("[woof: %s] --- Sequence Number \"%lu\" ---", prog, sequence_index);
        log_debug("[woof: %s] \tnode_id \"%s\" with operation \"%d\"", prog, debug_node.node_id);
        log_debug("[woof: %s] \tdestination node_id \"%s\" at port \"%d\"",
                  prog,
                  debug_node.destination_node_id,
                  debug_node.destination_port);
        log_debug("[woof: %s] \tstate \"%d\"", prog, debug_node.state);
        log_debug("[woof: %s] \t(input port \"%d\" with value \"%f\")",
                  prog,
                  debug_node.claim_input_port,
                  debug_node.claim_input_value);
        log_debug("[woof: %s] \treceived \"%d\" of \"%d\" values",
                  prog,
                  get_received_values_count(&debug_node),
                  debug_node.total_values_count);

        char* debug_values_string = values_as_string(debug_node.values, debug_node.total_values_count);
        log_debug("[woof: %s] \tvalues [%s]", prog, debug_values_string);
        free(debug_values_string);
        log_debug("[woof: %s] -------------------------------", prog);
    }
}

unsigned int get_received_values_count(DF_NODE* node) {
    unsigned int received_values_count = 0;
    for (int i = 0; i < VALUE_SIZE; ++i) {
        received_values_count += (*node).value_exist[i];
    }
    return received_values_count;
}
