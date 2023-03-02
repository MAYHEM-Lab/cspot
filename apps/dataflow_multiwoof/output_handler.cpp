#include "df.h"
#include "woofc.h"
#include "dfinterface.h"

#include <iostream>
#include <fstream>

#include <chrono>
#include <ctime>

extern "C" int output_handler(WOOF* wf, unsigned long seqno, void* ptr) {
    // std::cout << "OUTPUT HANDLER STARTED" << std::endl;

    operand* result = static_cast<operand*>(ptr);

    // std::cout << "wf: " << WoofGetFileName(wf) << std::endl;
    // std::cout << "seqno: " << seqno << std::endl;
    // std::cout << "operand: " << result->value << std::endl;

    // Get name of this woof
    std::string woof_name(WoofGetFileName(wf));
    
    // Extract id
    // Name format: [namespace].output.[id]
    size_t last_dot = woof_name.find_last_of('.');
    std::string id_str = woof_name.substr(last_dot + 1);
    // std::cout << "woof_name: " << woof_name << std::endl;
    // std::cout << "id_str: " << id_str << std::endl;
    unsigned long id = std::stoul(id_str);

    // Extract namespace
    size_t first_dot = woof_name.find('.');
    std::string program = woof_name.substr(0, first_dot);

    // Exit if double-fired
    if (seqno > 1) {
        operand prev;
        woof_get(woof_name, &prev, seqno - 1);
        if (prev.seq == result->seq) {
            // std::cout << "[" << woof_name << "] double-fired, exiting" << std::endl;
            // std::cout << "OUTPUT HANDLER DONE (early)" << std::endl;
            return 0;
        }
    }

    /* Get subscribers */

    std::string submap = program + ".subscriber_map";
    std::string subdata = program + ".subscriber_data";
    unsigned long start_idx, end_idx;
    unsigned long last_seq = woof_last_seq(submap);

    // Get data range (TODO: factor out into function for woofmap)
    woof_get(submap, &start_idx, id);
    if (id == last_seq) {
        end_idx = woof_last_seq(subdata) + 1;
    } else {
        woof_get(submap, &end_idx, id + 1);
    }

    // std::cout << "start_idx: " << start_idx << ", end_idx: " << end_idx << std::endl;

    // Iterate over subscribers and push to respective woofs
    for (unsigned long i = start_idx; i < end_idx; i++) {
        // Get subscriber data
        subscriber sub;
        woof_get(subdata, &sub, i);

        // Push event to subscription_event woof of subscriber
        std::string subscriber_woof = "laminar-" + std::to_string(sub.ns) +
                                      ".subscription_events." +
                                      std::to_string(sub.id);
        subscription_event subevent(sub.ns, sub.id, sub.port, result->seq);
        woof_put(subscriber_woof, "subscription_event_handler", &subevent);

        // std::cout << "{ns=" << sub.ns << ", id=" << sub.id << ", port=" << sub.port << ", seqno=" << seqno << "}" << std::endl;
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