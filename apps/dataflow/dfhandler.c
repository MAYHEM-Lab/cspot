#include "df.h"
#include "dflogger.h"
#include "woofc.h"

#include <malloc.h>
#include <string.h>

// #define DEBUG

unsigned int get_received_values_count(DFNODE* node);

int df_fire_node(WOOF* wf, const char* prog, DFNODE* node, double result) {
    // retire this node
    node->state = DONE;
    unsigned long done_node_sequence_number = WooFPut(prog, NULL, node);
    if (WooFInvalid(done_node_sequence_number)) {
        // TODO
        log_error("[woof: %s] Could not finish node ", prog);
        return 1;
    } else {
        log_info("[woof: %s] FINISH NODE node_id:\"%d\" to destination node_id:\"%d\" for input port:\"%d\" with "
                 "value:\"%f\"",
                 prog,
                 node->node_id,
                 node->destination_node_id,
                 node->destination_port,
                 result);
    }

    // transmit result to next node
    DFOPERAND result_operand;
    memset(&result_operand, 0, sizeof(result_operand));
    result_operand.value = result;
    result_operand.destination_node_id = node->destination_node_id;
    result_operand.destination_port = node->destination_port;
    strcpy(result_operand.prog_woof, prog);

    // if the destination node id is 0, this is a final result. Don't fire the handler in this case
    unsigned long done_operand_sequence_number;
    if (node->destination_node_id == 0) {
        done_operand_sequence_number = WooFPut(WooFGetFileName(wf), NULL, &result_operand);
        log_info("FINAL RESULT: %f", result);
    } else {
        done_operand_sequence_number = WooFPut(WooFGetFileName(wf), "dfhandler", &result_operand);
    }

    // check for successful put operation
    if (WooFInvalid(done_operand_sequence_number)) {
        // TODO
        log_error("[woof: %s] Could not create result OPERAND at woof %s", prog, WooFGetFileName(wf));
        return 1;
    } else {
        log_debug("[woof: %s] CREATE OPERAND at woof:\"%s\" node_id:\"%d\" to destination node_id:\"%d\" for input "
                  "port:\"%d\" with "
                  "value:\"%f\"",
                  prog,
                  WooFGetFileName(wf),
                  node->node_id,
                  node->destination_node_id,
                  node->destination_port,
                  result);
    }
    return 0;
}

int dfhandler(WOOF* wf, unsigned long operand_sequence_number, void* ptr) {
    DFOPERAND* operand = (DFOPERAND*)ptr;
    const char* prog = operand->prog_woof;

    log_info("[woof: %s] HANDLER EXECUTE of node_id:\"%d\" for input port:\"%d\" with value:\"%f\"",
             prog,
             operand->destination_node_id,
             operand->destination_port,
             operand->value);

    // create a CLAIM node at the end of the prog woof to mark the log with the arrival of the operand
    DFNODE claim_node;
    memset(&claim_node, 0, sizeof(claim_node));
    claim_node.state = CLAIM;
    claim_node.node_id = operand->destination_node_id;
    memset(claim_node.value_exist, 0, VALUE_SIZE);
    claim_node.value_exist[claim_node.claim_input_port] = 1;
    memset(claim_node.values, 0, VALUE_SIZE * sizeof(double));
    claim_node.claim_input_port = operand->destination_port;
    claim_node.claim_input_value = operand->value;
    unsigned long claim_sequence_number = WooFPut(prog, NULL, &claim_node);
    if (WooFInvalid(claim_sequence_number)) {
        log_error("[woof: %s] Could not create CLAIM NODE node_id:\"%d\" for input port:\"%d\" with value:\"%f\"",
                  prog,
                  claim_node.node_id,
                  claim_node.claim_input_port,
                  claim_node.claim_input_value);
        return 1;
    } else {
        log_info("[woof: %s] CREATE CLAIM NODE node_id:\"%d\" for input port:\"%d\" with value:\"%f\"",
                 prog,
                 claim_node.node_id,
                 claim_node.claim_input_port,
                 claim_node.claim_input_value);
    }

    log_trace("[woof: %s] CURRENT STACK:", prog);
    for (unsigned long sequence_index = claim_sequence_number; !WooFInvalid(sequence_index); sequence_index--) {
        DFNODE debug_node;
        WooFGet(prog, &debug_node, sequence_index);

        log_trace("[woof: %s] --- Sequence Number \"%lu\" ---", prog, sequence_index);
        log_trace("[woof: %s] \tnode id \"%d\" with operation \"%d\"", prog, debug_node.node_id);
        log_trace("[woof: %s] \tdestination node id \"%d\" at port \"%d\"",
                  prog,
                  debug_node.destination_node_id,
                  debug_node.destination_port);
        log_trace("[woof: %s] \tstate \"%d\"", prog, debug_node.state);
        log_trace("[woof: %s] \t(input port \"%d\" with value \"%f\"",
                  prog,
                  debug_node.claim_input_port,
                  debug_node.claim_input_value);
        log_trace("[woof: %s] \treceived \"%d\" of \"%d\" values",
                  prog,
                  get_received_values_count(&debug_node),
                  debug_node.total_values_count);

        char* debug_values_string = values_as_string(debug_node.values, debug_node.total_values_count);
        log_trace("[woof: %s] \tvalues [%s]", prog, debug_values_string);
        free(debug_values_string);
        log_trace("[woof: %s] -------------------------------", prog);
    }

    /*
     * claim_sequence_number is the sequence number in the log of the CLAIM
     * for this handler.  For this handler, this defines the tail
     * of the log.  Search backwards from this point (SCAN 1)
     *
     * if we find another CLAIM or PARTIAL with an earlier sequence number
     * and the same destination_node_id, but we don't find a WAITING, then there is
     * another handler in progress that will find this CLAIM and fire
     * the node and this handler should just exit (SCAN 1 exit case)
     *
     * if we find a WAITING for this destination_node_id, then we fire the node (SCAN 1 fire case)
     *
     * then we create a partial and we then check to make sure another claim did not come
     * in before we commited the partial to the log (SCAN 2)
     *
     * if we find CLAIMS in Scan 2 we add their PARTIALS and we start SCAN 2B from the last PARTIAL
     * backwards till the PREVIOUS PARTIAL (SCAN 2B)
     * SCAN 2B is repeated until no other CLAIM is found.
     *
     * now the handler checks the CLAIMS which are AFTER the latest PARTIAL, until the end of the WOOF
     * it fires PARTIAL for all CLAIMS found. (SCAN 3)
     *
     * After scan 3 the handler appends the PARTIAL_DONE node, so that a new handler can start firing the coming CLAIMS.
     *
     * After every claim is processed the number of input elements are checked
     * if it is n then the node is fired and DONE.
     *
     * main log scan loop (SCAN 1)
     *
     * must start with the sequence_index for my CLAIM since it will be a future
     * if another handler completes a partial afterwards.
     */
    unsigned long partial_sequence_number = -1;
    DFNODE node;
    unsigned long sequence_index;
    for (sequence_index = claim_sequence_number - 1; !WooFInvalid(sequence_index) && sequence_index > 0;
         sequence_index--) {
        int get_result = WooFGet(prog, &node, sequence_index);
        if (get_result < 0) {
            log_error("[woof: %s] Could not get node with sequence_number:\"%lu\"", prog, sequence_index);
            return 1;
        }

        // TODO
        log_trace("INITIAL TEST: ");
        log_trace("\tclaim_node_no: %d   (val=%f)", claim_node.node_id, claim_node.claim_input_value);
        log_trace("\tnode_no: %d", node.node_id);
        log_trace("\tclaim_port_no: %d, ", claim_node.claim_input_port);
        log_trace("\tstate: %d, ", node.state);
        log_trace("claim_node_no : %d, claim_node_port : %d claim_node_state %d curr_node_no: %d curr_node_state: "
                  "%d curr_node_seq_no: %ld",
                  claim_node.node_id,
                  claim_node.claim_input_port,
                  claim_node.state,
                  node.node_id,
                  node.state,
                  sequence_index);

        if (node.node_id != claim_node.node_id) {
            continue;
        }

        // if we find a CLAIM or PARTIAL node with a lower sequence number,
        // then exit as that CLAIM or PARTIAL is in progress and will pick up this CLAIM as a future
        if (node.state == CLAIM || node.state == PARTIAL) {
            if (node.state == CLAIM) {
                log_info("[woof: %s] HANDLER EXIT found CLAIM during SCAN-1 at sequence_number:\"%lu\" for NODE "
                         "node_id:\"%d\" with input port:\"%d\" with value:\"%f\" (searcher is input port:\"%d\" with "
                         "value:\"%f\")",
                         prog,
                         sequence_index,
                         node.node_id,
                         node.claim_input_port,
                         node.claim_input_value,
                         claim_node.claim_input_port,
                         claim_node.claim_input_value);
            } else {
                char* old_partial_values_string = values_as_string(node.values, node.total_values_count);
                log_info(
                    "[woof: %s] HANDLER EXIT found PARTIAL during SCAN-1 at sequence_number:\"%lu\" for NODE "
                    "node_id:\"%d\" with \"%u\"/\"%u\" values:[%s] (searcher is input port:\"%d\" with value:\"%f\")",
                    prog,
                    sequence_index,
                    node.node_id,
                    get_received_values_count(&node),
                    node.total_values_count,
                    old_partial_values_string,
                    claim_node.claim_input_port,
                    claim_node.claim_input_value);
                free(old_partial_values_string);
            }
            return 0;
        }
        // we didn't see a younger CLAIM/PARTIAL before, we are going to encounter WAITING
        // we take info from that node, add the new-found info
        else if (node.state == WAITING) {
            char* waiting_values_string = values_as_string(node.values, node.total_values_count);
            log_info("[woof: %s] FOUND WAITING during SCAN-1 at sequence_number:\"%lu\" for NODE "
                     "node_id:\"%d\" with \"%u\"/\"%u\" values:[%s] (searcher is input port:\"%d\" with value:\"%f\")",
                     prog,
                     sequence_index,
                     node.node_id,
                     get_received_values_count(&node),
                     node.total_values_count,
                     waiting_values_string,
                     claim_node.claim_input_port,
                     claim_node.claim_input_value);
            free(waiting_values_string);

            node.value_exist[claim_node.claim_input_port] = 1;
            node.values[claim_node.claim_input_port] = claim_node.claim_input_value;

            // if all inputs are received fire the node.
            if (get_received_values_count(&node) == node.total_values_count) {
                double result = DFOperation(node.opcode, node.values, node.total_values_count);

                char* all_inputs_values_string = values_as_string(node.values, node.total_values_count);
                log_info("[woof: %s] ALL INPUTS RECEIVED during SCAN-1 at sequence_number:\"%lu\" for NODE "
                         "node_id:\"%d\" with \"%u\"/\"%u\" values:[%s] and result:\"%f\" (last input was input "
                         "port:\"%d\" with value:\"%f\")",
                         prog,
                         sequence_index,
                         node.node_id,
                         get_received_values_count(&node),
                         node.total_values_count,
                         all_inputs_values_string,
                         result,
                         claim_node.claim_input_port,
                         claim_node.claim_input_value);
                free(all_inputs_values_string);

                return df_fire_node(wf, prog, &node, result);
            }


            // if not all inputs are received then create a partial and start another check
            // backwards from that partial to this claim to search for other claims in the history.
            node.state = PARTIAL;
            partial_sequence_number = WooFPut(prog, NULL, &node);
            if (WooFInvalid(partial_sequence_number)) {
                // TODO
                log_error("[woof: %s] Could not create partial", prog);
                return 1;
            } else {
                char* partial_values_string = values_as_string(node.values, node.total_values_count);
                log_info(
                    "[woof: %s] CREATE PARTIAL after found WAITING during SCAN-1 at sequence_number:\"%lu\" for NODE "
                    "node_id:\"%d\" with \"%u\"/\"%u\" values:[%s] (searcher is input port:\"%d\" with value:\"%f\")",
                    prog,
                    sequence_index,
                    node.node_id,
                    get_received_values_count(&node),
                    node.total_values_count,
                    partial_values_string,
                    claim_node.claim_input_port,
                    claim_node.claim_input_value);
                free(partial_values_string);
            }

            // done with SCAN-1 when we process one WAITING node
            break;
        }

        // log scan continue
        log_trace("DECREMENTING (%lu, %lu)", sequence_index, claim_sequence_number);
    }
    if (sequence_index == 0) {
        log_info("[woof: %s] HANDLER EXIT found no WAITING for NODE (searcher is input port:\"%d\" with value:\"%f\")",
                 prog,
                 node.node_id,
                 claim_node.claim_input_port,
                 claim_node.claim_input_value);
        return 0;
    }

    /*
     * now search for a future that may have come in while we were creating the partial
     *
     * note that the future will be represented by a CLAIM that is younger than this partial, but
     * older than the CLAIM for this handler. The partial pre_claim_sequence_number is
     * partial_sequence_number and the CLAIM for this handler is claim_sequence_number
     *
     * start at the pre_claim_sequence_number immediately before this partial
     *
     * start a while loop for checking repeated CLAIMS until either no CLAIM is found
     *
     * the outer while loop is for part 2B i.e. new PARTIALS
     * arising out of this handler, have to keep backward checking
     * repeatedly, until no backward CLAIM is left. for each case,
     * loop starts from the newest partial to partial which caused it.
     */

    // to check if any new partial is fired; i.e. new backward CLAIM was found
    DFNODE fnode;
    unsigned long old_partial_sequence_number = -1;
    unsigned long new_claim_sequence_number;
    while (!WooFInvalid(partial_sequence_number)) {
        old_partial_sequence_number = partial_sequence_number;
        new_claim_sequence_number = partial_sequence_number - 1;
        partial_sequence_number = -1;

        // walk back from the partial to the CLAIM (SCAN 2A) just once
        // walk back form new partial to old partial (SCAN 2B) until no backward claim for this handler
        while (new_claim_sequence_number > claim_sequence_number) {
            int get_result = WooFGet(prog, &fnode, new_claim_sequence_number);
            if (get_result < 0) {
                log_error(
                    "[woof: %s] Could not get node with sequence_number:\"%lu\"", prog, new_claim_sequence_number);
                return 1;
            }

            if (fnode.node_id != node.node_id) {
                new_claim_sequence_number--; // continue future scan
                continue;
            }

            // is this another claim for the node that is younger than the partial? If so, fire or partial fire it
            if (fnode.state == CLAIM) {
                log_info("[woof: %s] FOUND CLAIM during SCAN-2 at sequence_number:\"%lu\" for NODE "
                         "node_id:\"%d\" with input port:\"%d\" with value:\"%f\" (searcher is input port:\"%d\" with "
                         "value:\"%f\")",
                         prog,
                         new_claim_sequence_number,
                         fnode.node_id,
                         fnode.claim_input_port,
                         fnode.claim_input_value,
                         claim_node.claim_input_port,
                         claim_node.claim_input_value);

                node.value_exist[fnode.claim_input_port] = 1;
                node.values[fnode.claim_input_port] = fnode.claim_input_value;


                // if all inputs are received then fire the node
                if (get_received_values_count(&node) == node.total_values_count) {
                    double result = DFOperation(node.opcode, node.values, node.total_values_count);

                    char* all_inputs_values_string = values_as_string(node.values, node.total_values_count);
                    log_info("[woof: %s] ALL INPUTS RECEIVED during SCAN-2 at sequence_number:\"%lu\" for NODE "
                             "node_id:\"%d\" with \"%u\"/\"%u\" values:[%s] and result:\"%f\" (last input was input "
                             "port:\"%d\" with value:\"%f\")",
                             prog,
                             new_claim_sequence_number,
                             node.node_id,
                             get_received_values_count(&node),
                             node.total_values_count,
                             all_inputs_values_string,
                             result,
                             claim_node.claim_input_port,
                             claim_node.claim_input_value);
                    free(all_inputs_values_string);

                    return df_fire_node(wf, prog, &node, result);
                }


                // create partial if all inputs are not received
                // does one more round of checking as not all inputs are received.
                // this is done by the updated partial_sequence_number
                partial_sequence_number = WooFPut(prog, NULL, &node);
                if (WooFInvalid(partial_sequence_number)) {
                    // TODO
                    log_error("[woof: %s] Could not create partial", prog);
                    return 1;
                } else {
                    char* partial_values_string = values_as_string(node.values, node.total_values_count);
                    log_info(
                        "[woof: %s] CREATE PARTIAL after found CLAIM during SCAN-2 at sequence_number:\"%lu\" for NODE "
                        "node_id:\"%d\" with \"%u\"/\"%u\" values:[%s] (searcher is input port:\"%d\" with "
                        "value:\"%f\")",
                        prog,
                        partial_sequence_number,
                        node.node_id,
                        get_received_values_count(&node),
                        node.total_values_count,
                        partial_values_string,
                        claim_node.claim_input_port,
                        claim_node.claim_input_value);
                    free(partial_values_string);
                }
            }
            new_claim_sequence_number--; // continue future scan
        }
        // reduce the scan space by limiting it to the first PARTIAL as the lowest node
        claim_sequence_number = old_partial_sequence_number;
    }

    // all claims are in the backlog are processed now. check for new claims till the end of the list.
    // start from 'the checking PARTIAL' till end of list. add a WAITING at the end of the list. (SCAN 3)
    for (unsigned long future_claim_sequence_number = old_partial_sequence_number + 1;
         !WooFInvalid(old_partial_sequence_number) && future_claim_sequence_number <= WooFGetLatestSeqno(prog);
         future_claim_sequence_number++) {
        int get_result = WooFGet(prog, &fnode, future_claim_sequence_number);
        if (get_result < 0) {
            log_error("[woof: %s] Could not get node with sequence_number:\"%lu\"", prog, future_claim_sequence_number);
            return 1;
        }

        if (fnode.node_id != node.node_id) {
            continue;
        }

        // if CLAIM is found, create a partial
        // if PARTIAL is found, it must be created by the claims found in this loop so ignore them
        if (fnode.state == CLAIM) {
            log_info("[woof: %s] FOUND CLAIM during SCAN-3 at sequence_number:\"%lu\" for NODE "
                     "node_id:\"%d\" with input port:\"%d\" with value:\"%f\" (searcher is input port:\"%d\" with "
                     "value:\"%f\")",
                     prog,
                     future_claim_sequence_number,
                     fnode.node_id,
                     fnode.claim_input_port,
                     fnode.claim_input_value,
                     claim_node.claim_input_port,
                     claim_node.claim_input_value);

            node.value_exist[fnode.claim_input_port] = 1;
            node.values[fnode.claim_input_port] = fnode.claim_input_value;


            // if all inputs are received then fire the node.
            if (get_received_values_count(&node) == node.total_values_count) {
                double result = DFOperation(node.opcode, node.values, node.total_values_count);

                char* all_inputs_values_string = values_as_string(node.values, node.total_values_count);
                log_info("[woof: %s] ALL INPUTS RECEIVED during SCAN-3 at sequence_number:\"%lu\" for NODE "
                         "node_id:\"%d\" with \"%u\"/\"%u\" values:[%s] and result:\"%f\" (last input was input "
                         "port:\"%d\" with value:\"%f\")",
                         prog,
                         future_claim_sequence_number,
                         node.node_id,
                         get_received_values_count(&node),
                         node.total_values_count,
                         all_inputs_values_string,
                         result,
                         claim_node.claim_input_port,
                         claim_node.claim_input_value);
                free(all_inputs_values_string);

                return df_fire_node(wf, prog, &node, result);
            }


            // create partial if not all inputs are received
            partial_sequence_number = WooFPut(prog, NULL, &node);
            if (WooFInvalid(partial_sequence_number)) {
                log_error("[woof: %s] Could not create partial", prog);
                return 1;
            } else {
                char* partial_values_string = values_as_string(node.values, node.total_values_count);
                log_info(
                    "[woof: %s] CREATE PARTIAL after found CLAIM during SCAN-3 at sequence_number:\"%lu\" for NODE "
                    "node_id:\"%d\" with \"%u\"/\"%u\" values:[%s] (searcher is input port:\"%d\" with "
                    "value:\"%f\")",
                    prog,
                    partial_sequence_number,
                    node.node_id,
                    get_received_values_count(&node),
                    node.total_values_count,
                    partial_values_string,
                    claim_node.claim_input_port,
                    claim_node.claim_input_value);
                free(partial_values_string);
            }
        }
    }


    // end of partial creation by all possible ways on top
    // need better solution here as there might be a (very rare case)
    // where a CLAIM might be added in between the loop ending and this woofput
    node.state = WAITING;
    unsigned long waiting_sequence_number = WooFPut(prog, NULL, &node);
    if (WooFInvalid(waiting_sequence_number)) {
        // TODO
        log_error("[woof: %s] Could not create waiting", prog);
        return 1;
    } else {
        char* waiting_values_string = values_as_string(node.values, node.total_values_count);
        log_info("[woof: %s] CREATE WAITING after end of SCANS at sequence_number:\"%lu\" for NODE "
                 "node_id:\"%d\" with \"%u\"/\"%u\" values:[%s] (searcher is input port:\"%d\" with "
                 "value:\"%f\")",
                 prog,
                 waiting_sequence_number,
                 node.node_id,
                 get_received_values_count(&node),
                 node.total_values_count,
                 waiting_values_string,
                 claim_node.claim_input_port,
                 claim_node.claim_input_value);
        free(waiting_values_string);
    }


    DFNODE pre_waiting_claim_node;
    for (unsigned long pre_waiting_claim_sequence_number = partial_sequence_number;
         pre_waiting_claim_sequence_number < waiting_sequence_number;
         pre_waiting_claim_sequence_number++) {
        int get_result = WooFGet(prog, &pre_waiting_claim_node, pre_waiting_claim_sequence_number);
        if (get_result < 0) {
            log_error(
                "[woof: %s] Could not get node with sequence_number:\"%lu\"", prog, pre_waiting_claim_sequence_number);
            return 1;
        }

        if (pre_waiting_claim_node.node_id != node.node_id) {
            continue;
        }

        // if CLAIM is found, it slipped through. Resend the CLAIM
        // everything else should be ignorable
        if (pre_waiting_claim_node.state == CLAIM) {
            log_info("[woof: %s] FOUND CLAIM during SCAN-4 at sequence_number:\"%lu\" for NODE "
                     "node_id:\"%d\" with input port:\"%d\" with value:\"%f\" (searcher is input port:\"%d\" with "
                     "value:\"%f\")",
                     prog,
                     pre_waiting_claim_sequence_number,
                     pre_waiting_claim_node.node_id,
                     pre_waiting_claim_node.claim_input_port,
                     pre_waiting_claim_node.claim_input_value,
                     claim_node.claim_input_port,
                     claim_node.claim_input_value);

            // resend CLAIM as operator retry
            DFOPERAND resend_operand;
            memset(&resend_operand, 0, sizeof(resend_operand));
            resend_operand.value = pre_waiting_claim_node.claim_input_value;
            resend_operand.destination_node_id = pre_waiting_claim_node.node_id;
            resend_operand.destination_port = pre_waiting_claim_node.claim_input_port;
            strcpy(resend_operand.prog_woof, prog);

            unsigned long done_operand_sequence_number;
            done_operand_sequence_number = WooFPut(WooFGetFileName(wf), "dfhandler", &resend_operand);

            // check for successful put operation
            if (WooFInvalid(done_operand_sequence_number)) {
                // TODO
                log_error("[woof: %s] Could not create resend OPERAND at woof %s", prog, WooFGetFileName(wf));
                return 1;
            } else {
                log_debug("[woof: %s] CREATE OPERAND RESEND at woof:\"%s\" node_id:\"%d\" to destination "
                          "node_id:\"%d\" for input port:\"%d\" with "
                          "value:\"%f\"",
                          prog,
                          WooFGetFileName(wf),
                          pre_waiting_claim_node.node_id,
                          pre_waiting_claim_node.node_id,
                          pre_waiting_claim_node.claim_input_port,
                          pre_waiting_claim_node.claim_input_value);
            }
        }
    }

    // one running handler finishes every possible claim in its lifetime adds a WAITING
    return 0;
}

unsigned int get_received_values_count(DFNODE* node) {
    unsigned int received_values_count = 0;
    for (int i = 0; i < VALUE_SIZE; ++i) {
        received_values_count += (*node).value_exist[i];
    }
    return received_values_count;
}
