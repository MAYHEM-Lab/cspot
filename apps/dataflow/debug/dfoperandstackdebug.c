#include "../df.h"
#include "../dflogger.h"
#include "../woofc.h"

#include <malloc.h>

unsigned int get_received_values_count(DF_NODE* node);

int main() {

    WooFInit();
    printf("OPERAND STACK DEBUG\n");
    char* prog = "test.dfoperand";

    log_debug("[woof: %s] CURRENT STACK:", prog);
    unsigned long latest_sequence_number = WooFGetLatestSeqno(prog);
    for (unsigned long sequence_index = 1; sequence_index <= latest_sequence_number; sequence_index++) {
        DF_OPERAND debug_operand;

        WooFGet(prog, &debug_operand, sequence_index);

        log_debug("[woof: %s] ------------ Sequence Number \"%lu\" ------------", prog, sequence_index);
        log_debug("[woof: %s] \tdestination node_id \"%s\" at port \"%d\"",
                  prog,
                  debug_operand.destination_node_id,
                  debug_operand.destination_port);
        log_debug("[woof: %s] \tvalue [%f]", prog, debug_operand.value);
        log_debug("[woof: %s] \tprogram woof [%s]", prog, debug_operand.prog_woof);
        log_debug("[woof: %s] -------------------------------------------------", prog);
    }
}
