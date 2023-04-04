#include "df.h"
#include "dfinterface.h"
#include "woofc.h"

#include <chrono>
#include <ctime>
#include <deque>
#include <fstream>
#include <iostream>
#include <math.h>
#include <thread>

extern "C" int output_handler(WOOF* wf, unsigned long seqno, void* ptr) {
    // std::cout << "OUTPUT HANDLER STARTED " <<  WoofGetFileName(wf) << std::endl;

    int err;
    operand* result = static_cast<operand*>(ptr);
    unsigned long consumer_seq = result->seq;
    std::deque<subscription_event> event_buffer;

    // std::cout << "wf: " << WoofGetFileName(wf) << std::endl;
    // std::cout << "seqno: " << seqno << std::endl;
    // std::cout << "operand: " << result->value << std::endl;

    // Get name of this woof
    std::string woof_name(WoofGetFileName(wf));
    // std::cout << woof_name << " @ " << seqno << std::endl;

    DEBUG_PRINT("Starting output handler");

    // Extract id
    unsigned long id = get_id_from_woof_path(woof_name);

    // std::cout << id << ": " << result->seq << std::endl;

    // Extract namespace
    int ns = get_ns_from_woof_path(woof_name);

    // Exit if double-fired
    if (seqno > 1) {
        operand prev;
        err = woof_get(woof_name, &prev, seqno - 1);
        if (err < 0) {
            std::cout << "Error reading woof (o): " << woof_name << std::endl;
            return 0;
        }

        if (prev.seq == result->seq) {
            // std::cout << "OUTPUT HANDLER DONE (early) "  << woof_name << std::endl;
            return 0;
        }
    }

    /* Get subscribers */
    std::string submap_woof = generate_woof_path(SUBSCRIBER_MAP_WOOF_TYPE, ns);
    std::string subdata_woof = generate_woof_path(SUBSCRIBER_DATA_WOOF_TYPE, ns);
    unsigned long start_idx, end_idx;
    unsigned long last_seq = woof_last_seq(submap_woof);

    // Get data range (TODO: factor out into function for woofmap)
    err = woof_get(submap_woof, &start_idx, id);
    if (err < 0) {
        std::cout << "Error reading submap woof (o1): " << submap_woof << std::endl;
        return 0;
    }

    if (id == last_seq) {
        end_idx = woof_last_seq(subdata_woof) + 1;
    } else {
        err = woof_get(submap_woof, &end_idx, id + 1);
        if (err < 0) {
            std::cout << "Error reading submap woof (o2): " << submap_woof << std::endl;
            return 0;
        }
    }

    unsigned long res;

    // Iterate over subscribers and push to respective woofs
    for (unsigned long i = start_idx; i < end_idx; i++) {
        // Get subscriber data
        subscriber sub;
        err = woof_get(subdata_woof, &sub, i);
        if (err < 0) {
            std::cout << "Error reading subdata woof: " << subdata_woof << std::endl;
            return 0;
        }

        std::string subscriber_woof = generate_woof_path(SUBSCRIPTION_EVENTS_WOOF_TYPE, sub.ns, sub.id);
        subscription_event subevent(sub.ns, sub.id, sub.port, result->seq);

        // std::cout << woof_name << "[" << result->seq << "]: P\t-> " << subscriber_woof << std::endl << std::flush;
        res = woof_put(subscriber_woof, SUBSCRIPTION_EVENT_HANDLER, &subevent);
        // std::cout << woof_name << "[" << result->seq << "]: V\t-> " << subscriber_woof << std::endl << std::flush;

        /* add to the buffer if it is a remote woof which could not be put */
        if (res == (unsigned long)-1 && !subscriber_woof.rfind("woof://", 0)) {
            DEBUG_PRINT("Buffer for remote put");
            event_buffer.push_back(subevent);
        }
    }

    // keep retrying to send the subscription events in the buffer until empty
    int itr = 0;
    int MAX_RETRIES = 100;
    while (itr <= MAX_RETRIES) {
        if (itr > 0) {
            std::cout << "retrying" << std::endl;
        }

        while (!event_buffer.empty()) {
            subscription_event subevent = event_buffer.front();
            event_buffer.pop_front();

            std::string subscriber_woof = generate_woof_path(SUBSCRIPTION_EVENTS_WOOF_TYPE, subevent.ns, subevent.id);
            res = woof_put(subscriber_woof, SUBSCRIPTION_EVENT_HANDLER, &subevent);

            /* add back the buffer if it is a remote woof which could not be put */
            if (WooFInvalid(res)) {
                std::cout << "Retry Failed : " << subscriber_woof << std::endl;
                event_buffer.push_back(subevent);
            } else {
                // duplicate event sent randomly
                if (rand() % 2) {
                    woof_put(subscriber_woof, SUBSCRIPTION_EVENT_HANDLER, &subevent);
                }
                std::cout << "Retry Success : " << subscriber_woof << std::endl;
            }
        }

        if (event_buffer.empty()) {
            break;
        }

        int ms = std::min((int(pow(2, itr)) * 1000) + (rand() % 100), 32000);
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
        itr++;
    }

    // std::cout << "OUTPUT HANDLER DONE" << std::endl;

    // // linreg_multinode
    // if (id == 1 && woof_name == "laminar-5.output.1") {

    // // linreg_uninode
    // if (id == 1 && woof_name == "laminar-1.output.1") {

    //     auto end = std::chrono::high_resolution_clock::now();
    //     std::cout << "end" << ": "
    //             << std::chrono::duration_cast<std::chrono::nanoseconds>(
    //                     end.time_since_epoch())
    //                     .count()
    //             << "ns" << std::endl;
    // }
    return 0;
}
