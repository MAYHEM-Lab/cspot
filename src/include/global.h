#ifndef CSPOT_GLOBAL_H
#define CSPOT_GLOBAL_H

#if defined(__cplusplus)
extern "C" {
#endif

#include "log.h"
extern char WooF_namespace[2048];
extern char WooF_dir[2048];
extern char WooF_namelog_dir[2048];
extern char Namelog_name[2048];
extern unsigned long Name_id;
extern char Host_ip[25];

extern LOG* Name_log;

#if defined(__cplusplus)
}

#include <string_view>

namespace cspot::globals {
void set_namespace(std::string_view ns);
void set_dir(std::string_view dir);
void set_namelog_dir(std::string_view dir);
void set_namelog_name(std::string_view name);
void set_host_ip(std::string_view ip);
}
#endif
#endif