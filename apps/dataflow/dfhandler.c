#include "df.h"
#include "dflogger.h"
#include "woofc.h"

#include <malloc.h>
#include <stdio.h>
#include <string.h>

// #define DEBUG

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
                 node->node_no,
                 node->dst_no,
                 node->dst_port,
                 result);
    }

    // transmit result to next node
    DFOPERAND result_operand;
    memset(&result_operand, 0, sizeof(result_operand));
    result_operand.value = result;
    result_operand.dst_no = node->dst_no;
    result_operand.dst_port = node->dst_port;
    strcpy(result_operand.prog_woof, prog);

    // if the destination node id is 0, this is a final result. Don't fire the handler in this case
    unsigned long done_operand_sequence_number;
    if (node->dst_no == 0) {
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
        log_debug("[woof: %s] CREATE OPERAND at woof:\"%s\" to destination node_id:\"%d\" for input port:\"%d\" with "
                  "value:\"%f\"",
                  prog,
                  WooFGetFileName(wf),
                  node->node_no,
                  node->dst_no,
                  node->dst_port,
                  result);
        return 0;
    }
}

int dfhandler(WOOF* wf, unsigned long operand_sequence_number, void* ptr) {
    DFOPERAND* operand = (DFOPERAND*)ptr;
    const char* prog = operand->prog_woof;

    log_info("[woof: %s] HANDLER EXECUTE of node_id:\"%d\" for input port:\"%d\" with value:\"%f\"",
             prog,
             operand->dst_no,
             operand->dst_port,
             operand->value);

    // create a CLAIM node at the end of the prog woof to mark the log with the arrival of the operand
    DFNODE claim_node;
    memset(&claim_node, 0, sizeof(claim_node));
    claim_node.state = CLAIM;
    claim_node.node_no = operand->dst_no;
    claim_node.recvd_val_ct = 1;
    // claim_node.values = {0};
    claim_node.ip_port = operand->dst_port;
    claim_node.ip_value = operand->value;
    unsigned long claim_sequence_number = WooFPut(prog, NULL, &claim_node);
    if (WooFInvalid(claim_sequence_number)) {
        log_error("[woof: %s] Could not create CLAIM NODE node_id:\"%d\" for input port:\"%d\" with value:\"%f\"",
                  prog,
                  claim_node.node_no,
                  claim_node.ip_port,
                  claim_node.ip_value);
        return 1;
    } else {
        log_info("[woof: %s] CREATE CLAIM NODE node_id:\"%d\" for input port:\"%d\" with value:\"%f\"",
                 prog,
                 claim_node.node_no,
                 claim_node.ip_port,
                 claim_node.ip_value);
    }

    log_trace("[woof: %s] CURRENT STACK:", prog);
    for (unsigned long sequence_index = claim_sequence_number; !WooFInvalid(sequence_index); sequence_index--) {
        DFNODE debug_node;
        WooFGet(prog, &debug_node, sequence_index);

        log_trace("[woof: %s] --- Sequence Number \"%lu\" ---", prog, sequence_index);
        log_trace("[woof: %s] \tnode id \"%d\" with operation \"%d\"", prog, debug_node.node_no);
        log_trace(
            "[woof: %s] \tdestination node id \"%d\" at port \"%d\"", prog, debug_node.dst_no, debug_node.dst_port);
        log_trace("[woof: %s] \tstate \"%d\"", prog, debug_node.state);
        log_trace("[woof: %s] \t(input port \"%d\" with value \"%f\"", prog, debug_node.ip_port, debug_node.ip_value);
        log_trace(
            "[woof: %s] \treceived \"%d\" of \"%d\" values", prog, debug_node.recvd_val_ct, debug_node.total_val_ct);

        char* debug_values_string = values_as_string(debug_node.values, debug_node.total_val_ct);
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
     * and the same dst_no, but we don't find a WAITING, then there is
     * another handler in progress that will find this CLAIM and fire
     * the node and this handler should just exit (SCAN 1 exit case)
     *
     * if we find a WAITING for this dst_no, then we fire the node (SCAN 1 fire case)
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
    unsigned long d_seqno = -1;
    DFNODE node;
    for (unsigned long sequence_index = claim_sequence_number - 1; !WooFInvalid(sequence_index); sequence_index--) {
        int get_result = WooFGet(prog, &node, sequence_index);
        if (get_result < 0) {
            log_error("[woof: %s] Could not get node with sequence_number:\"%lu\"", prog, sequence_index);
            return 1;
        }

        //TODO
        log_trace("INITIAL TEST: ");
        log_trace("\tclaim_node_no: %d   (val=%f)", claim_node.node_no, claim_node.ip_value);
        log_trace("\tnode_no: %d", node.node_no);
        log_trace("\tclaim_port_no: %d, ", claim_node.ip_port);
        log_trace("\tstate: %d, ", node.state);
        log_trace("claim_node_no : %d, claim_node_port : %d claim_node_state %d curr_node_no: %d curr_node_state: "
                  "%d curr_node_seq_no: %ld",
                  claim_node.node_no,
                  claim_node.ip_port,
                  claim_node.state,
                  node.node_no,
                  node.state,
                  sequence_index);

        if (node.node_no != claim_node.node_no) {
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
                         node.node_no,
                         node.ip_port,
                         node.ip_value,
                         claim_node.ip_port,
                         claim_node.ip_value);
            } else {
                char* old_partial_values_string = values_as_string(node.values, node.total_val_ct);
                log_info(
                    "[woof: %s] HANDLER EXIT found PARTIAL during SCAN-1 at sequence_number:\"%lu\" for NODE "
                    "node_id:\"%d\" with \"%u\"/\"%u\" values:[%s] (searcher is input port:\"%d\" with value:\"%f\")",
                    prog,
                    sequence_index,
                    node.node_no,
                    node.recvd_val_ct,
                    node.total_val_ct,
                    old_partial_values_string,
                    claim_node.ip_port,
                    claim_node.ip_value);
                free(old_partial_values_string);
            }
            return 0;
        }
        // we didn't see a younger CLAIM/PARTIAL before, we are going to encounter WAITING
        // we take info from that node, add the new-found info
        else if (node.state == WAITING) {
            char* waiting_values_string = values_as_string(node.values, node.total_val_ct);
            log_info(
                "[woof: %s] FOUND WAITING during SCAN-1 at sequence_number:\"%lu\" for NODE "
                "node_id:\"%d\" with \"%u\"/\"%u\" values:[%s] (searcher is input port:\"%d\" with value:\"%f\")",
                prog,
                sequence_index,
                node.node_no,
                node.recvd_val_ct,
                node.total_val_ct,
                waiting_values_string,
                claim_node.ip_port,
                claim_node.ip_value);
            free(waiting_values_string);

            node.recvd_val_ct += 1;
            node.values[claim_node.ip_port] = claim_node.ip_value;

            // if all inputs are received fire the node.
            if (node.recvd_val_ct == node.total_val_ct) {
                double result = DFOperation(node.opcode, node.values, node.total_val_ct);

                char* all_inputs_values_string = values_as_string(node.values, node.total_val_ct);
                log_info("[woof: %s] ALL INPUTS RECEIVED during SCAN-1 at sequence_number:\"%lu\" for NODE "
                         "node_id:\"%d\" with \"%u\"/\"%u\" values:[%s] and result:\"%f\" (last input was input "
                         "port:\"%d\" with value:\"%f\")",
                         prog,
                         sequence_index,
                         node.node_no,
                         node.recvd_val_ct,
                         node.total_val_ct,
                         all_inputs_values_string,
                         result,
                         claim_node.ip_port,
                         claim_node.ip_value);
                free(all_inputs_values_string);

                return df_fire_node(wf, prog, &node, result);
            }


            // if not all inputs are received then create a partial and start another check
            // backwards from that partial to this claim to search for other claims in the history.
            node.state = PARTIAL;
            d_seqno = WooFPut(prog, NULL, &node);
            if (WooFInvalid(d_seqno)) {
                // TODO
                log_error("[woof: %s] Could not create partial", prog);
                return 1;
            } else {
                char* partial_values_string = values_as_string(node.values, node.total_val_ct);
                log_info(
                    "[woof: %s] CREATE PARTIAL after found WAITING during SCAN-1 at sequence_number:\"%lu\" for NODE "
                    "node_id:\"%d\" with \"%u\"/\"%u\" values:[%s] (searcher is input port:\"%d\" with value:\"%f\")",
                    prog,
                    sequence_index,
                    node.node_no,
                    node.recvd_val_ct,
                    node.total_val_ct,
                    partial_values_string,
                    claim_node.ip_port,
                    claim_node.ip_value);
                free(partial_values_string);
            }

            // done with SCAN 1 when we process one WAITING node
            break;
        }

        // log scan continue
        log_trace("DECREMENTING (%lu, %lu)", sequence_index, claim_sequence_number);
    }

    /*
     * now search for a future that may have come in while we were creating the partial
     *
     * note that the future will be represented by a CLAIM that is younger than this partial, but
     * older than the CLAIM for this handler. The partial pre_claim_sequence_number is
     * d_seqno and the CLAIM for this handler is claim_sequence_number
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
    unsigned long p_seqno = -1;
    unsigned long f_seqno;
    while (!WooFInvalid(d_seqno)) {
        p_seqno = d_seqno;
        f_seqno = d_seqno - 1;
        d_seqno = -1;

        // walk back from the partial to the CLAIM (SCAN 2A) just once
        // walk back form new partial to old partial (SCAN 2B) until no backward claim for this handler
        while (f_seqno > claim_sequence_number) {
            int get_result = WooFGet(prog, &fnode, f_seqno);
            if (get_result < 0) {
                fprintf(stderr, "dfhandler ERROR -- couldn't get %lu from %s on future\n", f_seqno, prog);
                return (1);
            }

            // is this another claim for the node that is younger than the partial? If so, fire or partial fire it
            if ((fnode.node_no == node.node_no) && (fnode.state == CLAIM)) {
                log_info("SCAN 2 found CLAIM prog: %s, node_no: %d ip_val %f ip_port %d",
                         prog,
                         node.node_no,
                         fnode.ip_value,
                         fnode.ip_port);
                node.recvd_val_ct += 1;
                node.values[fnode.dst_port] = fnode.ip_value;


                // if all inputs are received then fire the node
                if (node.recvd_val_ct == node.total_val_ct) {
                    double result = DFOperation(node.opcode, node.values, node.total_val_ct);
                    log_info(
                        "SCAN 2 ALL INPUTS RECEIVED: prog: %s, node_no: %d result: %f", prog, node.node_no, result);

                    return df_fire_node(wf, prog, &node, result);
                }


                // create partial if all inputs are not received
                // does one more round of checking as not all inputs are received.
                // this is done by the updated d_seqno
                d_seqno = WooFPut(prog, NULL, &node);
                if (WooFInvalid(d_seqno)) {
                    fprintf(stderr, "dfhandler: failed to put partial to %s\n", prog);
                    return (1);
                }
                log_info("SCAN 2 CREATE PARTIAL prog: %s, node_no: %d ip_port %d ip_val %f recvd %d total %d",
                         prog,
                         node.node_no,
                         fnode.ip_port,
                         fnode.ip_value,
                         node.recvd_val_ct,
                         node.total_val_ct);
            }
            f_seqno--; // continue future scan
        }
        // reduce the scan space by limiting it to the first PARTIAL as the lowest node
        claim_sequence_number = p_seqno;
    }

    // all claims are in the backlog are processed now. check for new claims till the end of the list.
    // start from 'the checking PARTIAL' till end of list. add a WAITING at the end of the list. (SCAN 3)
    f_seqno = p_seqno + 1;
    while (!WooFInvalid(p_seqno) && f_seqno <= WooFGetLatestSeqno(prog)) {
        int get_result = WooFGet(prog, &fnode, f_seqno);
        if (get_result < 0) {
            fprintf(stderr, "dfhandler ERROR -- couldn't get %lu from %s on future\n", f_seqno, prog);
            return (1);
        }

        // if CLAIM is found, create a partial
        // if PARTIAL is found, it must be created by the claims found in this loop so ignore them
        if ((fnode.node_no == node.node_no) && (fnode.state == CLAIM)) {
            log_info("SCAN 3 found CLAIM prog: %s, node_no: %d ip_val %f ip_port %d",
                     prog,
                     node.node_no,
                     fnode.ip_value,
                     fnode.ip_port);

            node.recvd_val_ct += 1;
            node.values[fnode.dst_port] = fnode.ip_value;


            // if all inputs are received then fire the node.
            if (node.recvd_val_ct == node.total_val_ct) {
                double result = DFOperation(node.opcode, node.values, node.total_val_ct);
                log_info("SCAN 3 ALL INPUTS RECEIVED: prog: %s, node_no: %d result: %f", prog, node.node_no, result);

                return df_fire_node(wf, prog, &node, result);
            }


            // create partial if not all inputs are received
            d_seqno = WooFPut(prog, NULL, &node);
            if (WooFInvalid(d_seqno)) {
                fprintf(stderr, "dfhandler: failed to put partial to %s\n", prog);
                return (1);
            }
            log_info("SCAN 3 CREATE PARTIAL prog: %s, node_no: %d ip_port %d ip_val %f recvd %d total %d",
                     prog,
                     node.node_no,
                     fnode.ip_port,
                     fnode.ip_value,
                     node.recvd_val_ct,
                     node.total_val_ct);
        }
        f_seqno++;
    }


    // end of partial creation by all possible ways on top
    // need better solution here as there might be a (very rare case)
    // where a CLAIM might be added in between the loop ending and this woofput
    node.state = WAITING;
    d_seqno = WooFPut(prog, NULL, &node);
    if (WooFInvalid(d_seqno)) {
        fprintf(stderr, "dfhandler: failed to put partial to %s\n", prog);
        return (1);
    }

    log_info("SCAN 3 CREATE WAITING prog: %s, node_no: %d recvd %d total %d",
             prog,
             node.node_no,
             node.recvd_val_ct,
             node.total_val_ct);

    // one running partial finishes every possible claim in its lifetime adds a PARTIAL_DONE
    return (0);
}