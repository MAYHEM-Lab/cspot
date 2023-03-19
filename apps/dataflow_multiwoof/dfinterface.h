#ifndef DFINTERFACE_H
#define DFINTERFACE_H

#include "df.h"
#include "woofc.h"

#include <map>
#include <set>
#include <string>

#define OUTPUT_HANDLER "output_handler"
#define SUBSCRIPTION_EVENT_HANDLER "subscription_event_handler"

void woof_create(std::string name, unsigned long element_size, unsigned long history_size);
unsigned long woof_put(std::string name, std::string handler, const void* element);
int woof_get(std::string name, void* element, unsigned long seq_no);
unsigned long woof_last_seq(std::string name);

void set_host(int host_id);
void add_host(int host_id, std::string host_ip, std::string woof_path);
void add_node(int ns, int host_id, int id, int opcode);
void add_operand(int ns, int host_id, int id);

void subscribe(int dst_ns, int dst_id, int dst_port, int src_ns, int src_id);
void subscribe(std::string dst_addr, std::string src_addr);

void setup();
void reset();

std::string graphviz_representation();

std::string generate_woof_path(DFWoofType woof_type, int ns = -1, int id = -1, int host_id = -1, int port_id = -1);

unsigned long get_id_from_woof_path(std::string woof_path);

int get_ns_from_woof_path(std::string woof_path);

#endif // DFINTERFACE_H