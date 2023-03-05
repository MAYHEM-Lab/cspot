#include "df.h"
#include "dfinterface.h"

#include <iostream>
#include <vector>

#include <unistd.h>

#define WOOFSIZE 10000

// {namspace --> entries}
std::map<int, int> subscribe_entries;
// {namespace --> {id --> [subscribers...]}}
std::map<int, std::map<int, std::set<subscriber>>> subscribers;
// {namespace --> {id --> [subscriptions...]}}
std::map<int, std::map<int, std::set<subscription>>> subscriptions;
// {namespace --> [nodes...]}
std::map<int, std::set<node>> nodes;

void woof_create(std::string name, unsigned long element_size,
                 unsigned long history_size) {
    WooFInit(); /* attach local namespace for create */

    int err = WooFCreate(name.c_str(), element_size, history_size);
    if (err < 0) {
        std::cerr << "ERROR -- creation of " << name << " failed\n";
        exit(1);
    }
}

unsigned long woof_put(std::string name, std::string handler,
                       const void* element) {
    unsigned long seqno;
    if (handler == "") {
        seqno = WooFPut(name.c_str(), NULL, element);
    } else {
        seqno = WooFPut(name.c_str(), handler.c_str(), element);
    }

    if (WooFInvalid(seqno)) {
        std::cerr << "ERROR -- put to " << name << " failed\n";
        exit(1);
    }

    return seqno;
}

void woof_get(std::string name, void* element, unsigned long seq_no) {
    int err = WooFGet(name.c_str(), element, seq_no);
    if (err < 0) {
        std::cerr << "ERROR -- get [" << seq_no << "] from " << name << " failed\n";
        exit(1);
    }
}

unsigned long woof_last_seq(std::string name) {
    return WooFGetLatestSeqno(name.c_str());
}

std::vector<std::string> split(std::string s, char delim = ',') {
    std::vector<std::string> result;
    size_t start = 0;
    size_t end = s.find(delim);
    std::string value;

    while (end != std::string::npos) {
        value = s.substr(start, end - start);
        result.push_back(value);
        start = end + 1;
        end = s.find(delim, start);
    }

    value = s.substr(start, end);
    result.push_back(value);
    return result;
}

void add_node(int ns, int id, int opcode) {
    std::string id_str = std::to_string(id);
    std::string ns_str = std::to_string(ns);
    std::string program = "laminar-" + ns_str;

    // Create output woof
    woof_create(program + ".output." + id_str, sizeof(operand), WOOFSIZE);

    // Create subscription_events woof
    woof_create(program + ".subscription_events." + id_str,
                sizeof(subscription_event), WOOFSIZE);

    // Create consumer_pointer woof
    std::string consumer_ptr_woof = program + ".subscription_pointer." + id_str;
    unsigned long initial_consumer_ptr = 1;
    // TODO: consumer_ptr_woof should be of size 1, but CSPOT hangs when
    // writing to full woof (instead of overwriting), so the size has
    // been increased temporarily as a stop-gap measure while testing
    woof_create(consumer_ptr_woof, sizeof(unsigned long), WOOFSIZE);
    woof_put(consumer_ptr_woof, "", &initial_consumer_ptr);

    // Create node
    nodes[ns].insert(node(id, opcode));
}

void add_operand(int ns, int id) {
    std::string id_str = std::to_string(id);
    std::string ns_str = std::to_string(ns);
    std::string program = "laminar-" + ns_str;

    // Create output woof
    woof_create(program + ".output." + id_str, sizeof(operand), WOOFSIZE);

    nodes[ns].insert(node(id, OPERAND));
}

void subscribe(int dst_ns, int dst_id, int dst_port, int src_ns, int src_id) {
    subscribers[src_ns][src_id].insert(subscriber(dst_ns, dst_id, dst_port));
    subscriptions[dst_ns][dst_id].insert(subscription(src_ns, src_id, dst_port));

    subscribe_entries[src_ns]++;
    subscribe_entries[dst_ns]++;
}

void subscribe(std::string dst_addr, std::string src_addr) {
    std::vector<std::string> dst = split(dst_addr, ':');
    std::vector<std::string> src = split(src_addr, ':');

    subscribe(std::stoi(dst[0]), std::stoi(dst[1]), std::stoi(dst[2]),
              std::stoi(src[0]), std::stoi(src[1]));
}

void setup(int ns) {
    std::string ns_str = std::to_string(ns);
    std::string program = "laminar-" + ns_str;

    // Create woof hashmaps to hold subscribers
    woof_create(program + ".subscriber_map", sizeof(unsigned long), nodes[ns].size());
    woof_create(program + ".subscriber_data", sizeof(subscriber),
                subscribe_entries[ns]);

    unsigned long current_data_pos = 1;
    for (size_t i = 1; i <= nodes[ns].size(); i++) {
        // Add entry in map (idx = node id, val = start idx in subscriber_data)
        woof_put(program + ".subscriber_map", "", &current_data_pos);        
        
        for (auto& sub : subscribers[ns][i]) {
            woof_put(program + ".subscriber_data", "", &sub);
            current_data_pos++;
        }
    }

    // Create woof hashmaps to hold subscriptions
    woof_create(program + ".subscription_map", sizeof(unsigned long), nodes[ns].size());
    woof_create(program + ".subscription_data", sizeof(subscription),
                subscribe_entries[ns]);

    current_data_pos = 1;
    for (size_t i = 1; i <= nodes[ns].size(); i++) {
        // Add entry in map (idx = node id, val = start idx in subscription_data)
        woof_put(program + ".subscription_map", "", &current_data_pos);
        
        for (auto& sub : subscriptions[ns][i]) {
            woof_put(program + ".subscription_data", "", &sub);
            current_data_pos++;
        }
    }

    // Create woof to hold node data
    woof_create(program + ".nodes", sizeof(node), nodes[ns].size());

    for (auto& node : nodes[ns]) {
        woof_put(program + ".nodes", "", &node);
    }
}

std::string graphviz_representation() {
    std::string g = "digraph G {\n\tnode [shape=\"record\", style=\"rounded\"];";

    // Add nodes
    for (auto& [ns, ns_nodes] : nodes) {
        g += "\n\tsubgraph cluster_" + std::to_string(ns) + " { ";
        g += "\n\t\tlabel=\"Subgraph #" + std::to_string(ns) + "\";";

        auto n = ns_nodes.begin();
        auto s = subscriptions[ns].begin();
        while (n != ns_nodes.end()) {
            g += "\n\t\tnode_" + std::to_string(ns) + "_" + std::to_string(n->id) + " [label=\"{";
            // Add ports
            if (n->opcode != OPERAND) {
                g += "{";
                for (size_t port = 0; port < s->second.size(); port++) {
                    std::string p = std::to_string(port);
                    g += "<" + p + "> " + p;
                    if (port < s->second.size() - 1) {
                        g += '|';
                    }
                }
                g += "}|";
            }

            g += "<out>[" + std::string(OPCODE_STR[n->opcode]);
            g += "]\\nNode #" + std::to_string(n->id) + "}\"];";
            
            n++;

            if (n->opcode != OPERAND) {
                s++;
            }
        }

        g += "\n\t}";
    }

    // Add edges
    for (auto& [ns, ns_subscriptions] : subscriptions) {
        for (auto& [id, subs] : ns_subscriptions) {
            for (auto s = subs.begin(); s != subs.end(); s++) {
                g += "\n\tnode_" + std::to_string(s->ns) + "_" + std::to_string(s->id) + ":out -> ";
                g += "node_" + std::to_string(ns) + "_" + std::to_string(id) + ":" + std::to_string(s->port) + ";";
            }
        }
    }

    g += "\n}";

    return g;
}



// void add_node(char* prog, int id) {
//     char woof_name_base[4096] = ""; // [prog].[id]
//     char woof_name[4096] = "";
//     char id_str[WOOFSIZE] = "";

//     // Convert id to string
//     snprintf(id_str, sizeof(id_str), "%d", id);
    
//     // woof_name_base = "[prog].[id]"
//     strncpy(woof_name_base, prog, sizeof(woof_name_base));
//     strncat(woof_name_base, id_str,
//             sizeof(woof_name_base) - strlen(woof_name_base) - 1);

//     // Create output woof
//     strncpy(woof_name, woof_name_base, sizeof(woof_name));
//     strncat(woof_name, ".output", sizeof(woof_name) - strlen(woof_name) - 1);
//     woof_create(woof_name, sizeof(operand), 10);

//     // Create subscription_events woof
//     strncpy(woof_name, woof_name_base, sizeof(woof_name));
//     strncat(woof_name, ".subscription_events", 
//             sizeof(woof_name) - strlen(woof_name) - 1);
//     woof_create(woof_name, sizeof(subscription_event), 25);

//     // Create consumer_pointer woof
//     strncpy(woof_name, woof_name_base, sizeof(woof_name));
//     strncat(woof_name, ".subscription_pointer",
//             sizeof(woof_name) - strlen(woof_name) - 1);
//     woof_create(woof_name, sizeof(unsigned long), 1);
// }
