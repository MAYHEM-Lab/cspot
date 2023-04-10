#include "df.h"
#include "dfinterface.h"
#include "woofc.h"

#include <chrono>
#include <cmath>
#include <iostream>
#include <queue>
#include <string>
#include <unistd.h>
#include <vector>

// Helper function to calculate Euclidean distance between two points
double dist(Point& a, Point& b) {
    return sqrt(pow(a.x - b.x, 2) + pow(a.y - b.y, 2));
}

operand perform_operation(const std::vector<operand>& ops, int ns, node& n, unsigned long consumer_seq) {
    // std::cout << "Perform operation: ops = ";
    // for (auto& op : ops) std::cout << op.value << " ";
    int opcode = n.opcode;
    // std::cout << "\nopcode: " << OPCODE_STR[opcode] << std::endl;

    operand result(0, consumer_seq);
    switch (opcode) {
    case ADD:
        result.value = 0;
        for (auto& op : ops) {
            result.value += op.value;
        }
        break;
    case SUB:
        result.value = ops[0].value;
        for (size_t i = 1; i < ops.size(); i++) {
            result.value -= ops[i].value;
        }
        break;
    case MUL:
        result.value = 1;
        for (auto& op : ops) {
            result.value *= op.value;
        }
        break;
    case DIV:
        result.value = ops[0].value;
        for (size_t i = 1; i < ops.size(); i++) {
            result.value /= ops[i].value;
        }
        break;
    case SQR:
        result.value = sqrt(ops[0].value);
        break;

    case LT:
        result.value = (double)(ops[0].value < ops[1].value);
        break;

    case GT:
        result.value = (double)(ops[0].value > ops[1].value);
        break;

    case EQ:
        result.value = (double)(ops[0].value == ops[1].value);
        break;

    case ABS:
        result.value = (ops[0].value < 0 ? -ops[0].value : ops[0].value);
        break;

    case NOT:
        result.value = (ops[0].value ? 0 : 1);
        break;

    case SEL:
        // Use selector value to index alternatives
        result.value = ops[(int)ops[0].value + 1].value;
        break;

    case FILTER:
        result.value = ops[1].value;
        // if (!static_cast<bool>(ops[0].value)) {
        //     exit(0);
        // }
        break;

    case OFFSET:
        result.value = ops[1].value;
        result.seq = consumer_seq + static_cast<unsigned long>(ops[0].value);
        break;

    case KNN: {
        size_t k = static_cast<size_t>(ops[0].value);
        Point p = Point(ops[1].value, ops[2].value);
        std::string data_woof = "laminar-" + std::to_string(ns) + ".knn_data." + std::to_string(n.id);

        using distPair = std::pair<double, int>; // (distance, label)
        std::priority_queue<distPair, std::vector<distPair>, std::greater<distPair>> pq;

        // Iterate over points data and add to priority queue
        unsigned long last_seq = woof_last_seq(data_woof);
        Point p_i;
        for (unsigned long s = 0; s <= last_seq; s++) {
            woof_get(data_woof, &p_i, s);
            double d = dist(p, p_i);
            pq.push(std::make_pair(d, p_i.label));
        }

        // Pop top k elements and find most common label
        int num_pops = std::min(k, pq.size());
        int most_common_label = pq.top().second;
        int count = 1;
        pq.pop();
        for (int i = 1; i < num_pops; i++) {
            if (pq.top().second == most_common_label)
                count++;
            else
                count--;

            if (count == 0) {
                most_common_label = pq.top().second;
                count = 1;
            }
            pq.pop();
        }

        // Add result to data woof
        p.label = most_common_label;
        woof_put(data_woof, "", &p);

        result.value = most_common_label;
    } break;

    case LINREG: {
        std::string data_woof = "laminar-" + std::to_string(ns) + ".linreg_data." + std::to_string(n.id);

        Regression r;
        if (woof_last_seq(data_woof)) {
            woof_get(data_woof, &r, 0);
        }
        r.update(ops[0].value, ops[1].value);
        woof_put(data_woof, "", &r);
        result.value = r.slope;
    } break;

    default:
        result.value = 0;
        break;
    }
    return result;
}

extern "C" int subscription_event_handler(WOOF* wf, unsigned long seqno, void* ptr) {
    
    // auto start = std::chrono::high_resolution_clock::now();
    // std::cout << "SUBSCRIPTION EVENT HANDLER STARTED" << std::endl;

    int err;

    // Get name of this woof
    std::string woof_name(WoofGetFileName(wf));

    // Extract id
    unsigned long id = get_id_from_woof_path(woof_name);

    // Extract namespace
    int ns = get_ns_from_woof_path(woof_name);

    // Get subscription_event
    subscription_event* subevent = static_cast<subscription_event*>(ptr);

    // Get current execution iteration
    execution_iteration_lock exec_iter_lk;
    std::string consumer_ptr_woof = generate_woof_path(SUBSCRIPTION_POINTER_WOOF_TYPE, ns, id);
    err = woof_get(consumer_ptr_woof, &exec_iter_lk, 0);
    if (err < 0) {
        std::cout << "Error reading woof (s1): " << consumer_ptr_woof << std::endl;
        return err;
    }

    unsigned long consumer_seq = exec_iter_lk.iter;

    DEBUG_PRINT("Starting subscription handler");

    // Only proceed if this event is relevant to the current execution iteration
    if (subevent->seq > consumer_seq) {
        DEBUG_PRINT("event seq: " << subevent->seq << ", consumer seq: " << consumer_seq);
        DEBUG_PRINT("Event is not for current seq, exiting");
        return 0;
    }

    // Only proceed if this event is not already being handled
    if (exec_iter_lk.lock) {
        DEBUG_PRINT("Another handler already locked, exiting");
    }

    // Look up subscriptions to determine required number of operands
    // TODO: Replace woofmap with single entry read-only woof for better performance
    std::string submap = generate_woof_path(SUBSCRIPTION_MAP_WOOF_TYPE, ns);
    std::string subdata = generate_woof_path(SUBSCRIPTION_DATA_WOOF_TYPE, ns);
    unsigned long start_idx, end_idx;
    unsigned long last_seq = woof_last_seq(submap);

    err = woof_get(submap, &start_idx, id);
    if (err < 0) {
        std::cout << "Error reading submap woof (s1): " << submap << std::endl;
        return err;
    }

    if (id == last_seq) {
        end_idx = woof_last_seq(subdata) + 1;
    } else {
        err = woof_get(submap, &end_idx, id + 1);
        if (err < 0) {
            std::cout << "Error reading submap woof (s2): " << submap << std::endl;
            return err;
        }
    }

    int num_ops = static_cast<int>(end_idx - start_idx);

    // Scan through subscription outputs and collect operands

    std::vector<operand> op_values(num_ops);
    subscription sub;
    operand op(0);
    std::string output_woof;
    for (unsigned long i = start_idx; i < end_idx; i++) {
#ifdef DEBUG
        std::string param_num = "(param #" + std::to_string(i - start_idx) + ") ";
#endif
        // Get last used output and seqno for this port
        cached_output last_output;
        std::string last_used_sub_pos_woof = generate_woof_path(SUBSCRIPTION_POS_WOOF_TYPE, ns, id, -1, i - start_idx);
        if (woof_last_seq(last_used_sub_pos_woof) == 0) {
            // On first read, check if output woof is empty
            if (woof_last_seq(output_woof) == 0) {
                DEBUG_PRINT(param_num << "No outputs yet, exiting");
                return 0;
            }
        } else {
            // std::cout << "Loading last_used_subscription_positions" << std::endl;
            woof_get(last_used_sub_pos_woof, &last_output, 0);
        }

        // std::cout << "[" << woof_name << "] " << "[" << consumer_seq << "] last_output_seq: " << last_output.op.seq
        // << ", consumer_seq: " << consumer_seq << std::endl;
        if (last_output.op.seq == consumer_seq) {
            // Operand for this seq has already been found and cached
            // Retrieve from cache and proceed
            // std::cout << "already retrieved input #" << i - start_idx << ", proceeding" << std::endl;
            op_values[i - start_idx] = last_output.op;
            continue;
        } else if (last_output.op.seq > consumer_seq) {
            // Another handler has already cached an operand from a later
            // execution iteration, thus this execution iteration has already
            // been completed, exit
            DEBUG_PRINT(param_num << "Cached operand exceeds current excecution iteration, exiting");
            return 0;
        }

        // std::cout << "subscription port: " << i - start_idx << std::endl;
        // Get subscription id
        err = woof_get(subdata, &sub, i);
        if (err < 0) {
            std::cout << "Error reading subdata woof: " << subdata << std::endl;
            return err;
        }

        // Get relevant operand from subscription output (if it exists)
        output_woof = generate_woof_path(OUTPUT_WOOF_TYPE, sub.ns, sub.id);

        // Scan from last used output until finding current seq
        unsigned long idx = last_output.seq;
        unsigned long last_idx = woof_last_seq(output_woof);

        if (idx >= last_idx) {
            DEBUG_PRINT(param_num << "No new outputs to check: cached_idx=" << idx << ", last_seq=" << last_idx);
            return 0;
        }

        // Increment sequence number (idx) until finding current execution iteration
        do {
            idx++;
            err = woof_get(output_woof, &op, idx);
            if (err < 0) {
                std::cout << "Error reading output woof: " << output_woof << std::endl;
                return err;
            }
        } while (op.seq < consumer_seq && idx < last_idx);

#ifdef DEBUG
        if (op.seq != consumer_seq) {
            DEBUG_PRINT(param_num << "ERROR: UNEXPECTED BEHAVIOR (SKIPPED EXECUTION ITER)");
            std::vector<operand> v;
            for (int j = 1; j <= idx; j++) {
                operand tmp;
                woof_get(output_woof, &tmp, j);
                v.push_back(tmp);
            }

            std::cout << "op.seq: " << op.seq << ", consumer_seq: " << consumer_seq << std::endl;

            for (auto& o : v) {
                std::cout << output_woof << " @ " << o.seq << ": " << o.value << std::endl;
            }
        }
#endif

        // Write latest idx back to `last used subscription position` woof
        // std::cout << "Writing back: " << "op = " << op.value << ", seq=" << idx << std::endl;
        if (op.seq <= consumer_seq) {
            DEBUG_PRINT(param_num << "CACHING OUTPUT: seq=" << op.seq << ", idx=" << idx);
            last_output = cached_output(op, idx);
            woof_put(last_used_sub_pos_woof, "", &last_output);
        }

        if (op.seq == consumer_seq) {
            // Relevant operand found, save and continue
            // std::cout << "[" << woof_name<< "] getting op" << std::endl;
            // std::cout << "[" << woof_name<< "] consumer_seq: "
            // << consumer_seq << ", woof: " << output_woof << ", op: "
            // << op.value << std::endl;
            op_values[i - start_idx] = op;
        } else {
            // At least one input is not ready --> exit handler
            // std::cout << "idx: " << idx << ", consumer_seq: " << consumer_seq
            // << ", op.seq: " << op.seq << std::endl;
            DEBUG_PRINT(param_num << "Not all operands are present, exiting");
            return 0;
        }
    }

    /* Fire node */

    DEBUG_PRINT("Firing node");

    // TODO: Factor out into function [
    // Get current execution iteration
    err = woof_get(consumer_ptr_woof, &exec_iter_lk, 0);
    if (err < 0) {
        std::cout << "Error reading woof (s2): " << consumer_ptr_woof << std::endl;
        return err;
    }

    // Only proceed if this event is relevant to the current execution iteration
    if (subevent->seq > exec_iter_lk.iter) {
        DEBUG_PRINT("event seq: " << subevent->seq << ", consumer seq: " << exec_iter_lk.iter);
        DEBUG_PRINT("Event is not for current seq, exiting");
        return 0;
    }

    // Only proceed if this event is not already being handled
    if (exec_iter_lk.lock) {
        DEBUG_PRINT("Another handler already locked, exiting");
    }
    // TODO: Factor out into function ]

    // TODO: Factor out into function [
    // Test-and-set to take lock for this execution iteration
    exec_iter_lk.lock = true;
    unsigned long lk_seq = woof_put(consumer_ptr_woof, "", &exec_iter_lk);
    execution_iteration_lock prev_exec_iter_lk;
    woof_get(consumer_ptr_woof, &prev_exec_iter_lk, lk_seq - 1);
    if (prev_exec_iter_lk.iter > exec_iter_lk.iter) {
        DEBUG_PRINT("ERROR: UNEXPECTED BEHAVIOR (LOCKS OUT OF ORDER)");

        DEBUG_PRINT("\tprev: " << prev_exec_iter_lk.iter << ", " << prev_exec_iter_lk.lock);
        DEBUG_PRINT("\tthis: " << exec_iter_lk.iter << ", " << exec_iter_lk.lock);

        // Put previous entry back and trigger subscription handler manually (if not locked)
        woof_put(consumer_ptr_woof, "", &prev_exec_iter_lk);
        if (!prev_exec_iter_lk.lock) {
            subevent->seq++;
            subscription_event_handler(wf, seqno + 1, subevent);
        }

        return 0;
    }
    if (prev_exec_iter_lk.lock) {
        DEBUG_PRINT("Failed to acquire lock, exiting");
    } else {
        DEBUG_PRINT("Lock acquired");
    }
    // TODO: Factor out into function ]

    // Get opcode
    node n;
    std::string nodes_woof = generate_woof_path(NODES_WOOF_TYPE, ns);
    err = woof_get(nodes_woof, &n, id);
    if (err < 0) {
        std::cout << "Error reading nodes woof: " << nodes_woof << std::endl;
        return err;
    }
    // std::cout << "[" << woof_name<< "] get node" << std::endl;

    operand result = perform_operation(op_values, ns, n, consumer_seq);
    // std::cout << "[" << woof_name<< "] result = " << result.value << std::endl;

    // Do not write result if it already exists
    output_woof = generate_woof_path(OUTPUT_WOOF_TYPE, ns, id);

    operand last_result;
    woof_get(output_woof, &last_result, 0);
    if (n.opcode != OFFSET) {
        // Fix for offset: the following code prevents duplicates with most
        // nodes, but renders the offset node useless. For now, this feature
        // will be disabled for offset nodes. TODO: account for offset node's
        // `offset` to check for duplicates.
        if (last_result.seq >= consumer_seq) {
            DEBUG_PRINT("Operation already performed, exiting");
            // // Relinquish lock and increment execution iteration
            // exec_iter_lk.lock = false;
            // exec_iter_lk.iter++;
            // woof_put(consumer_ptr_woof, "", &exec_iter_lk);

            // // Call handler for next iter in case all operands were received before this function finished
            // if (woof_last_seq(woof_name) > seqno) {
            //     subevent->seq++;
            //     subscription_event_handler(wf, seqno + 1, subevent);
            // }

            return 0;
        }
    }

    // Write result (unless FILTER should omit result)
    if (n.opcode != FILTER || op_values[0].value) {
        woof_put(output_woof, OUTPUT_HANDLER, &result);
        DEBUG_PRINT("Wrote result #" << consumer_seq);
    }

    /*
    // // linreg_multinode
    if (id == 8 && woof_name == "laminar-1.subscription_events.8") {

    // // linreg_uninode
    // if (id == 1 && woof_name == "laminar-1.subscription_events.1") {

        std::cout << "start: "
                << std::chrono::duration_cast<std::chrono::nanoseconds>(
                        start.time_since_epoch())
                        .count()
                << "ns" << std::endl;
    }
    */
   
    // std::cout << "SUBSCRIPTION EVENT HANDLER DONE" << std::endl;

    // Relinquish lock and increment execution iteration
    exec_iter_lk.lock = false;
    exec_iter_lk.iter++;
    woof_put(consumer_ptr_woof, "", &exec_iter_lk);

    // Call handler for next iter in case all operands were received before this function finished
    // if (woof_last_seq(woof_name) > seqno) {
    //     subevent->seq++;
    //     subscription_event_handler(wf, seqno + 1, subevent);
    // }
    subevent->seq++;
    subscription_event_handler(wf, seqno + 1, subevent);

    return 0;
}
