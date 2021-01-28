#include <cstdlib>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <woofc-priv.h>
#include <global.h>
#include <debug.h>

#include <fmt/format.h>

namespace {
const char* from_env_or(const char* env_key, const char* or_) {
    auto env = getenv(env_key);
    if (!env) {
        return or_;
    }
    return env;
}
} // namespace

namespace cspot {
bool is_platform = false;
}

int WooFInit() {
    struct timeval tm;
    int err;
    char* str;

    gettimeofday(&tm, NULL);
    srand48(tm.tv_sec + tm.tv_usec);

    str = getenv("WOOFC_DIR");
    if (str == NULL) {
        getcwd(WooF_dir, sizeof(WooF_dir));
    } else {
        strncpy(WooF_dir, str, sizeof(WooF_dir));
    }
    if (strcmp(WooF_dir, "/") == 0) {
        fprintf(stderr, "WooFInit: WOOFC_DIR can't be %s\n", WooF_dir);
        exit(1);
    }

    if (WooF_dir[strlen(WooF_dir) - 1] == '/') {
        WooF_dir[strlen(WooF_dir) - 1] = 0;
    }

    setenv("WOOFC_DIR", WooF_dir, 1);

    DEBUG_LOG("WooFInit: WooF_dir: %s\n", WooF_dir);

    /*
     * note that the container will always have WOOFC_NAMESPACE specified by the launcher
     */
    cspot::globals::set_namespace(from_env_or("WOOFC_NAMESPACE", WooF_dir));
    DEBUG_LOG("WooFInit: namespace: %s\n", WooF_namespace);

    cspot::globals::set_namelog_dir(from_env_or("WOOF_NAMELOG_DIR", WooF_dir));
    DEBUG_LOG("WooFInit: namelog dir: %s\n", WooF_namelog_dir);

    err = mkdir(WooF_dir, 0600);
    if ((err < 0) && (errno != EEXIST)) {
        perror("WooFInit");
        return (-1);
    }

    Name_id = WooFNameHash(WooF_namelog_dir);
    sprintf(Namelog_name, "cspot-log.%20.20lu", Name_id);


    auto log_name = fmt::format("{}/{}", WooF_namelog_dir, Namelog_name);

    DEBUG_LOG("WooFInit: Name log at %s with name %s\n", log_name.c_str(), Namelog_name);

    if (!cspot::is_platform) {
        MIO* lmio = MIOReOpen(log_name.c_str());
        if (lmio == NULL) {
            fprintf(stderr, "WooFInit: name %s (%s) log not initialized.\n", log_name.c_str(), WooF_dir);
            exit(1);
        }
        Name_log = static_cast<LOG*>(MIOAddr(lmio));
    }

    return 1;
}