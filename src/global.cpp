#include <global.h>
#include <cstring>

extern "C" {
char WooF_namespace[2048];
char WooF_dir[2048];
char WooF_namelog_dir[2048];
char Namelog_name[2048];
unsigned long Name_id;
LOG* Name_log;
char Host_ip[25];
}

namespace cspot::globals {
void set_namespace(std::string_view ns) {
    strncpy(WooF_namespace, ns.data(), std::min(std::size(WooF_namespace), ns.size()));
}
void set_dir(std::string_view dir) {
    strncpy(WooF_dir, dir.data(), std::min(std::size(WooF_dir), dir.size()));
}
void set_namelog_dir(std::string_view dir) {
    strncpy(WooF_namelog_dir, dir.data(), std::min(std::size(WooF_namelog_dir), dir.size()));
}
void set_namelog_name(std::string_view name) {
    strncpy(Namelog_name, name.data(), std::min(std::size(Namelog_name), name.size()));
}
void set_host_ip(std::string_view ip) {
    strncpy(Host_ip, ip.data(), std::min(std::size(Host_ip), ip.size()));
}

context to_context() {
    context ctx;
    ctx.WooF_namespace = WooF_namespace;
    ctx.WooF_dir = WooF_dir;
    ctx.WooF_namelog_dir = WooF_namelog_dir;
    ctx.Namelog_name = Namelog_name;
    ctx.Host_ip = Host_ip;
    ctx.Name_id = Name_id;
    return ctx;
}

void from_context(const context& ctx) {
    set_namespace(ctx.WooF_namespace);
    set_dir(ctx.WooF_dir);
    set_namelog_dir(ctx.WooF_namelog_dir);
    set_namelog_name(ctx.Namelog_name);
    set_host_ip(ctx.Host_ip);
    Name_id = ctx.Name_id;
}
}