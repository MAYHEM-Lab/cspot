#include "dfinterface.h"

#include "df.h"

#include <fstream>
#include <iostream>
#include <unistd.h>
#include <vector>


#define OUTPUT_WOOF_SIZE 1000
#define SUBSCRIPTION_EVENTS_WOOF_SIZE 1000
#define EXECUTION_ITERATION_WOOF_SIZE 1000
#define SUBSCRIPTION_POS_WOOF_SIZE 1000

// {namspace --> entries}
std::map<int, int> subscribe_entries;
// {namespace --> {id --> [subscribers...]}}
std::map<int, std::map<int, std::set<subscriber>>> subscribers;
// {namespace --> {id --> [subscriptions...]}}
std::map<int, std::map<int, std::set<subscription>>> subscriptions;
// {namespace --> [nodes...]}
std::map<int, std::set<node>> nodes;
// set of host structs for url extraction
std::set<host> hosts;

int get_curr_host() {

    int curr_host_id;
    int err = woof_get(generate_woof_path(HOST_ID_WOOF_TYPE), &curr_host_id, 0);
    if (err < 0) {
        std::cout << "Error obtaining current host id " << std::endl;
        exit(1);
    }
    return curr_host_id;
}

std::string generate_woof_path(DFWoofType woof_type, int ns, int id, int host_id, int port_id) {
    node n;
    int curr_host_id;
    std::string host_url = "";
    std::string woof_path, nodes_woof_path;


    // if it is the host_woof then ns == -1 and id == -1
    if (woof_type == HOST_ID_WOOF_TYPE || woof_type == HOSTS_WOOF_TYPE) {
        return host_url + "laminar." + DFWOOFTYPE_STR[woof_type];
    }

    // namespace level static woof return directly.
    if (id == -1) {
        return host_url + "laminar-" + std::to_string(ns) + "." + DFWOOFTYPE_STR[woof_type];
    }

    // extract the current host id from HOST_ID_WOOF_TYPE
    curr_host_id = get_curr_host();

    if (host_id == -1) {
        // get the host of the node from the node level woofs
        nodes_woof_path = host_url + "laminar-" + std::to_string(ns) + "." + DFWOOFTYPE_STR[NODES_WOOF_TYPE];
        int err = woof_get(nodes_woof_path, &n, id);
        if (err < 0) {
            std::cout << "Error reading the node info for node id: " << std::to_string(id)
                      << "node ns : " << std::to_string(ns) << std::endl;
            exit(1);
        }
        host_id = n.host_id;
    }

    // assign a host url only if it is not local; extract from HOSTS_WOOF
    if (host_id != curr_host_id) {
        host h;
        int err = woof_get(generate_woof_path(HOSTS_WOOF_TYPE), &h, host_id);
        if (err < 0) {
            std::cout << "Error reading the host info for host id: " << std::to_string(host_id) << std::endl;
            exit(1);
        }
        host_url.assign(h.host_url);
    }

    woof_path = host_url + "laminar-" + std::to_string(ns) + "." + DFWOOFTYPE_STR[woof_type] + "." + std::to_string(id);

    if (woof_type == SUBSCRIPTION_POS_WOOF_TYPE) {
        woof_path = woof_path + "." + std::to_string(port_id);
    }

    return woof_path;
}

unsigned long get_id_from_woof_path(std::string woof_path) {

    size_t last_dot = woof_path.find_last_of('.');
    std::string id_str = woof_path.substr(last_dot + 1);

    return std::stoul(id_str);
}

int get_ns_from_woof_path(std::string woof_path) {

    size_t dash = woof_path.find('-');
    std::string ns_str = woof_path.substr(dash + 1);
    size_t first_dot = ns_str.find('.');
    ns_str = ns_str.substr(0, first_dot);

    return std::stoi(ns_str);
}

void woof_create(std::string name, unsigned long element_size, unsigned long history_size) {
    WooFInit(); /* attach local namespace for create */

    int err = WooFCreate(name.c_str(), element_size, history_size);
    if (err < 0) {
        std::cerr << "ERROR -- creation of " << name << " failed\n";
        exit(1);
    }
}

unsigned long woof_put(std::string name, std::string handler, const void* element) {
    unsigned long seqno;
    if (handler == "") {
        seqno = WooFPut(name.c_str(), NULL, element);
    } else {
        seqno = WooFPut(name.c_str(), handler.c_str(), element);
    }

    if (WooFInvalid(seqno)) {
        std::cerr << "ERROR -- put to " << name << " failed\n";
    }

    return seqno;
}

int woof_get(std::string name, void* element, unsigned long seq_no) {
    int err = WooFGet(name.c_str(), element, seq_no);
    if (err < 0) {
        std::cerr << "ERROR -- get [" << seq_no << "] from " << name << " failed\n";
    }
    return err;
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

void add_host(int host_id, std::string host_ip, std::string woof_path) {

    // global info stored in every host
    std::string host_url = "woof://" + host_ip + woof_path;
    char host_url_c_str[200];
    strcpy(host_url_c_str, host_url.c_str());
    hosts.insert(host(host_id, host_url_c_str));
}

void set_host(int host_id) {

    std::string hosts_woof_path = generate_woof_path(HOST_ID_WOOF_TYPE);

    woof_create(hosts_woof_path, sizeof(int), 1);

    woof_put(hosts_woof_path, "", &host_id);
}

void add_node(int ns, int host_id, int id, int opcode) {

    // global info stored in every host
    nodes[ns].insert(node(id, host_id, opcode));

    int curr_host_id = get_curr_host();

    // create node related info only for current host
    if (host_id == curr_host_id) {

        woof_create(generate_woof_path(OUTPUT_WOOF_TYPE, ns, id, host_id), sizeof(operand), OUTPUT_WOOF_SIZE);

        // Create subscription_events woof
        woof_create(
            generate_woof_path(SUBSCRIPTION_EVENTS_WOOF_TYPE, ns, id, host_id), sizeof(subscription_event), SUBSCRIPTION_EVENTS_WOOF_SIZE);


        // Create consumer_pointer woof
        std::string consumer_ptr_woof = generate_woof_path(SUBSCRIPTION_POINTER_WOOF_TYPE, ns, id, host_id);
        execution_iteration_lock initial_consumer_ptr;
        // TODO: consumer_ptr_woof should be of size 1, but CSPOT hangs when
        // writing to full woof (instead of overwriting), so the size has
        // been increased temporarily as a stop-gap measure while testing
        woof_create(consumer_ptr_woof, sizeof(execution_iteration_lock), EXECUTION_ITERATION_WOOF_SIZE);
        woof_put(consumer_ptr_woof, "", &initial_consumer_ptr);
    }
}

void add_operand(int ns, int host_id, int id) {


    // global info stored in every host
    nodes[ns].insert(node(id, host_id, OPERAND));

    int curr_host_id = get_curr_host();
    // Create output woof if the operand belongs to this host only
    if (host_id == curr_host_id) {
        woof_create(generate_woof_path(OUTPUT_WOOF_TYPE, ns, id, host_id), sizeof(operand), OUTPUT_WOOF_SIZE);
    }
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

    subscribe(std::stoi(dst[0]), std::stoi(dst[1]), std::stoi(dst[2]), std::stoi(src[0]), std::stoi(src[1]));
}

void setup() {

    // setup all the namespaces
    for (const auto& [ns, value] : nodes) {

        // Create woof to hold node data
        woof_create(generate_woof_path(NODES_WOOF_TYPE, ns), sizeof(node), nodes[ns].size());

        for (auto& node : nodes[ns]) {
            woof_put(generate_woof_path(NODES_WOOF_TYPE, ns), "", &node);
        }

        // Create woof hashmaps to hold subscribers
        woof_create(generate_woof_path(SUBSCRIBER_MAP_WOOF_TYPE, ns), sizeof(unsigned long), nodes[ns].size());
        woof_create(generate_woof_path(SUBSCRIBER_DATA_WOOF_TYPE, ns), sizeof(subscriber), subscribe_entries[ns]);

        unsigned long current_data_pos = 1;
        for (size_t i = 1; i <= nodes[ns].size(); i++) {
            // Add entry in map (idx = node id, val = start idx in subscriber_data)
            woof_put(generate_woof_path(SUBSCRIBER_MAP_WOOF_TYPE, ns), "", &current_data_pos);

            for (auto& sub : subscribers[ns][i]) {
                woof_put(generate_woof_path(SUBSCRIBER_DATA_WOOF_TYPE, ns), "", &sub);
                current_data_pos++;
            }
        }

        // Create woof hashmaps to hold subscriptions
        woof_create(generate_woof_path(SUBSCRIPTION_MAP_WOOF_TYPE, ns), sizeof(unsigned long), nodes[ns].size());
        woof_create(generate_woof_path(SUBSCRIPTION_DATA_WOOF_TYPE, ns), sizeof(subscription), subscribe_entries[ns]);

        current_data_pos = 1;
        for (size_t i = 1; i <= nodes[ns].size(); i++) {
            // Add entry in map (idx = node id, val = start idx in subscription_data)
            woof_put(generate_woof_path(SUBSCRIPTION_MAP_WOOF_TYPE, ns), "", &current_data_pos);

            for (auto& sub : subscriptions[ns][i]) {
                woof_put(generate_woof_path(SUBSCRIPTION_DATA_WOOF_TYPE, ns), "", &sub);
                current_data_pos++;
            }

            // Add woofs to hold last used seq in subscription output woof
            for (size_t port = 0; port < subscriptions[ns][i].size(); port++) {
                woof_create(generate_woof_path(SUBSCRIPTION_POS_WOOF_TYPE, ns, i, -1, port), sizeof(cached_output), SUBSCRIPTION_POS_WOOF_SIZE);
            }
        }
    }

    // setup the host related info
    // TODO: setup check for host_url size
    // std::cout << "Sizeof host : " << sizeof(host) << "Hosts size : " << hosts.size() << std::endl;
    woof_create(generate_woof_path(HOSTS_WOOF_TYPE), sizeof(host), hosts.size());

    for (auto& host : hosts) {
        woof_put(generate_woof_path(HOSTS_WOOF_TYPE), "", &host);
    }
}

void reset() {
    // {namspace --> entries}
    subscribe_entries = std::map<int, int>();
    // {namespace --> {id --> [subscribers...]}}
    subscribers = std::map<int, std::map<int, std::set<subscriber>>>();
    // {namespace --> {id --> [subscriptions...]}}
    subscriptions = std::map<int, std::map<int, std::set<subscription>>>();
    // {namespace --> [nodes...]}
    nodes = std::map<int, std::set<node>>();
    // set of host structs for url extraction
    hosts = std::set<host>();
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
