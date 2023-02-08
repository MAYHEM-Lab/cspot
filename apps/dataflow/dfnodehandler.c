#include "df.h"
#include "dflogger.h"
#include "woofc.h"

#include <string.h>

#define OPERAND_WOOF "test.dfoperand"

int dfnodehandler(WOOF* wf, unsigned long node_sequence_number, void* ptr) {
    DF_NODE* created_node = ptr;

    log_info("[woof: %s] NODE HANDLER EXECUTE of node_id:\"%s\" fired from sequence number:\"%lu\"",
             WoofGetFileName(wf),
             created_node->node_id,
             node_sequence_number);

    DF_NODE pre_node_creation_claim_node;
    for (unsigned long pre_node_creation_claim_sequence_number = node_sequence_number;
         !WooFInvalid(pre_node_creation_claim_sequence_number) && pre_node_creation_claim_sequence_number > 0;
         pre_node_creation_claim_sequence_number--) {
        int get_result =
            WooFGet(WooFGetFileName(wf), &pre_node_creation_claim_node, pre_node_creation_claim_sequence_number);
        if (get_result < 0) {
            log_error("[woof: %s] Could not get node with sequence_number:\"%lu\"",
                      WooFGetFileName(wf),
                      pre_node_creation_claim_sequence_number);
            return 1;
        }

        if (strcmp(pre_node_creation_claim_node.node_id, created_node->node_id) != 0) {
            continue;
        }

        // if CLAIM is found, it slipped through. Resend the CLAIM
        // everything else should be ignorable
        if (pre_node_creation_claim_node.state == CLAIM) {
            log_info("[woof: %s] NODE FOUND CLAIM during NODE CREATE at sequence_number:\"%lu\" for NODE "
                     "node_id:\"%s\" with input port:\"%d\" with value:\"%f\"",
                     WooFGetFileName(wf),
                     pre_node_creation_claim_sequence_number,
                     pre_node_creation_claim_node.node_id,
                     pre_node_creation_claim_node.claim_input_port,
                     pre_node_creation_claim_node.claim_input_value);

            // resend CLAIM as operator retry
            DF_OPERAND resend_operand;
            memset(&resend_operand, 0, sizeof(resend_operand));
            resend_operand.value = pre_node_creation_claim_node.claim_input_value;
            strcpy(resend_operand.destination_node_id, pre_node_creation_claim_node.node_id);
            resend_operand.destination_port = pre_node_creation_claim_node.claim_input_port;
            strcpy(resend_operand.prog_woof, WooFGetFileName(wf));

            unsigned long done_operand_sequence_number;
            done_operand_sequence_number = WooFPut(OPERAND_WOOF, "dfhandler", &resend_operand);

            // check for successful put operation
            if (WooFInvalid(done_operand_sequence_number)) {
                // TODO
                log_error("[woof: %s] Could not create resend OPERAND at woof %s", WooFGetFileName(wf), OPERAND_WOOF);
                return 1;
            } else {
                log_debug("[woof: %s] NODE CREATE OPERAND RESEND at woof:\"%s\" node_id:\"%s\" to destination "
                          "node_id:\"%s\" for input port:\"%d\" with "
                          "value:\"%f\" written to sequence number:\"%lu\"",
                          WooFGetFileName(wf),
                          OPERAND_WOOF,
                          pre_node_creation_claim_node.node_id,
                          pre_node_creation_claim_node.node_id,
                          pre_node_creation_claim_node.claim_input_port,
                          pre_node_creation_claim_node.claim_input_value,
                          done_operand_sequence_number);
            }
        }
    }

    log_info("[woof: %s] NODE HANDLER EXIT after finished NODE CREATION cycle for NODE node_id:\"%s\"",
             WoofGetFileName(wf),
             created_node->node_id);
    return 0;
}