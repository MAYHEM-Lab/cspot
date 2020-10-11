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
extern LOG* Name_log;
extern char Host_ip[25];

#if defined(__cplusplus)
}
#endif
#endif