#include <cstdlib>
extern "C" {
#include "log.h"
#include "woofc.h"
}

#include "debug.h"
#include "global.h"
#include "net.h"
#include "woofc-access.h"
#include "woofc-priv.h"

#include <atomic>
#include <errno.h>
#include <fmt/format.h>
#include <iostream>
#include <map>
#include <memory>
#include <pthread.h>
#include <signal.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <thread>
#include <time.h>
#include <unistd.h>
#include <vector>

struct runner_params {
    int container_count;
};

class cspot_backend {
public:
    virtual void run(const runner_params&, const cspot::context& ctx) = 0;
    virtual void cleanup() = 0;
    virtual ~cspot_backend() = default;
};

namespace {
std::unique_ptr<cspot_backend> backend;
std::atomic<bool> should_exit;
pid_t do_spawn(const std::string& path, std::vector<std::string_view> args, std::vector<std::string> envp) {
    pid_t pid;

    std::vector<std::string> argv(args.size() + 1);
    argv.front() = path;
    std::copy(args.begin(), args.end(), argv.begin() + 1);

    std::vector<char*> c_argv(argv.size() + 1);
    std::transform(argv.begin(), argv.end(), c_argv.begin(), [](auto& str) { return &str[0]; });

    std::vector<char*> c_envp(envp.size() + 1);
    std::transform(envp.begin(), envp.end(), c_envp.begin(), [](auto& str) { return &str[0]; });

    if (posix_spawn(&pid, path.c_str(), nullptr, nullptr, c_argv.data(), c_envp.data()) != 0) {
        perror("posix_spawn failed");
        throw std::runtime_error("posix_spawn failed!");
    }

    return pid;
}
} // namespace

void WooFShutdown(int sig) {
    int val;

    should_exit = true;
    while (sem_getvalue(&Name_log->tail_wait, &val) >= 0) {
        if (val > 0) {
            break;
        }
        V(&Name_log->tail_wait);
    }
}

class docker_backend : public cspot_backend {
public:
    explicit docker_backend(std::string_view docker_exe_path = "docker")
        : m_docker_exe_path(docker_exe_path) {
    }

    void run(const runner_params& params, const cspot::context& ctx) override {
        /*
         * find the last directory in the path
         */

        // build the names for the workers to spawn
        DEBUG_LOG("worker names\n");
        for (int count = 0; count < params.container_count; ++count) {
            auto name_path_part = ctx.WooF_dir;
            std::replace(name_path_part.begin(), name_path_part.end(), '/', '_');
            DEBUG_LOG(name_path_part.c_str());

            worker_containers.emplace_back(
                fmt::format("CSPOTWorker-{}-{:x}-{}", name_path_part, WooFNameHash(ctx.WooF_namespace.c_str()), count));
            DEBUG_LOG("\t - %s\n", worker_containers[count].c_str());
        }

        cleanup();

        // now create the new containers
        DEBUG_LOG("WooFContainerLauncher started\n");

        std::vector<std::thread> launch_threads;

        for (int count = 0; count < params.container_count; count++) {
            DEBUG_LOG("WooFContainerLauncher: launch %d\n", count + 1);

            std::string launch_string;

            auto port = WooFPortHash(ctx.WooF_namespace.c_str());

            std::map<std::string, std::string> env_map;
            env_map.emplace("LD_LIBRARY_PATH", "/usr/local/lib");
            env_map.emplace("WOOFC_NAMESPACE", ctx.WooF_namespace);
            env_map.emplace("WOOFC_DIR", ctx.WooF_dir);
            env_map.emplace("WOOF_NAMELOG_DIR", ctx.WooF_namelog_dir);
            env_map.emplace("WOOF_NAME_ID", std::to_string(ctx.Name_id));
            env_map.emplace("WOOF_NAMELOG_NAME", ctx.Namelog_name);
            env_map.emplace("WOOF_HOST_IP", ctx.Host_ip);

            std::vector<std::string> env_pieces(env_map.size());
            std::transform(env_map.begin(), env_map.end(), env_pieces.begin(), [](auto& env_var) {
                auto& [key, val] = env_var;
                return fmt::format("-e {}={}", key, val);
            });

            launch_string = fmt::format("{docker_exec} run --network=host --rm --name {container_name} {envs} ",
                                        fmt::arg("docker_exec", m_docker_exe_path),
                                        fmt::arg("container_name", worker_containers[count]),
                                        fmt::arg("envs", fmt::join(env_pieces, " ")));

            if (count == 0) {
                launch_string += fmt::format("-p {0}:{0} ", port);
            }

            if (ctx.WooF_dir != ctx.WooF_namelog_dir) {
                launch_string += fmt::format("-v {0}:{0} ");
            }

            launch_string += fmt::format("-v {}:{} cspot-docker-centos7 {}/{} ",
                                         ctx.WooF_namelog_dir,
                                         ctx.WooF_namelog_dir,
                                         ctx.WooF_dir,
                                         "woofc-container");


            if (count == 0) {
                launch_string += "-M ";
            }

            DEBUG_LOG("\tcommand: '%s'\n", launch_string.c_str());

            std::cerr << launch_string << '\n';

            launch_threads.emplace_back([launch_string = std::move(launch_string)] {
                DEBUG_LOG("LAUNCH: %s\n", launch_string.c_str());

                system(launch_string.c_str());
                DEBUG_LOG("DONE: %s\n", launch_string.c_str());
            });
        }

        for (auto& thread : launch_threads) {
            thread.join();
        }

        DEBUG_LOG("WooFContainerLauncher backend exiting\n");
    }

    void cleanup() override {
        auto command = fmt::format("{} rm -f {}\n", m_docker_exe_path, fmt::join(worker_containers, ", "));
        DEBUG_LOG("CleanUpDocker command: %s\n", command.c_str());
        system(command.c_str());
    }

private:
    std::string m_docker_exe_path;

    std::vector<std::string> worker_containers;
    std::atomic<bool> should_exit;
};

class proc_backend : public cspot_backend {
public:
    void run(const runner_params& params, const cspot::context& ctx) override {
        // now create the new containers
        DEBUG_LOG("WooFContainerLauncher started\n");

        for (int count = 0; count < params.container_count; count++) {
            DEBUG_LOG("WooFContainerLauncher: launch %d\n", count + 1);

            std::string exec_path = fmt::format("{}/{}", ctx.WooF_dir, "woofc-container");

            std::map<std::string, std::string> env_map;
            env_map.emplace("LD_LIBRARY_PATH", "/usr/local/lib");
            env_map.emplace("WOOFC_NAMESPACE", ctx.WooF_namespace);
            env_map.emplace("WOOFC_DIR", ctx.WooF_dir);
            env_map.emplace("WOOF_NAMELOG_DIR", WooF_namelog_dir);
            env_map.emplace("WOOF_NAME_ID", std::to_string(ctx.Name_id));
            env_map.emplace("WOOF_NAMELOG_NAME", ctx.Namelog_name);
            env_map.emplace("WOOF_HOST_IP", ctx.Host_ip);

            std::vector<std::string> env_pieces(env_map.size());
            std::transform(env_map.begin(), env_map.end(), env_pieces.begin(), [](auto& env_var) {
                auto& [key, val] = env_var;
                return fmt::format("{}={}", key, val);
            });

            std::vector<std::string_view> argv;

            if (count == 0) {
                argv.emplace_back("-M");
            }

            m_pids.emplace_back(do_spawn(exec_path, std::move(argv), std::move(env_pieces)));
        }

        DEBUG_LOG("WooFContainerLauncher proc exiting\n");
    }

    void cleanup() override {
        DEBUG_LOG("Cleaning up the processes");
        for (auto& pid : m_pids) {
            kill(pid, SIGKILL);
        }

        for (auto& pid : m_pids) {
            int status;
            waitpid(pid, &status, 0);
        }
    }

private:
    std::vector<pid_t> m_pids;
};

void WooFContainerLauncher(const runner_params&, const cspot::context& ctx);

void CatchSignals();

void CleanUpDocker() {
    if (backend) {
        backend->cleanup();
    }
}


namespace cspot {
extern bool is_platform;
}

static int WooFHostInit(std::unique_ptr<cspot_backend> executor, int min_containers, int max_containers) {
    ::backend = std::move(executor);
    cspot::is_platform = true;
    WooFInit();

    auto log_name = fmt::format("{}/{}", WooF_namelog_dir, Namelog_name);

    Name_log = LogCreate(log_name.c_str(), Name_id, DEFAULT_WOOF_LOG_SIZE);
    DEBUG_FATAL_IF(Name_log == nullptr,
                   "WooFInit: couldn't create name log as %s, size %d\n",
                   log_name.c_str(),
                   DEFAULT_WOOF_LOG_SIZE);

    DEBUG_LOG("WooFHostInit: created %s\n", log_name.c_str());

    auto thread = std::thread([&] { ::backend->run(runner_params{min_containers}, cspot::globals::to_context()); });

    /*
     * sleep for 10 years
     *
     * calling pthread_join here causes pthreads to intercept the SIGINT signal handler
     * (which sounds like a bug in Linux).  Sleeping forever, however, allows SIGINT to
     * be caught which then triggers a clean up of the docker container
     */
    sleep(86400 * 365 * 10);
    thread.join();

    exit(0);
}

void WooFExit() {
    should_exit = true;
    pthread_exit(NULL);
}

#define ARGS "m:M:d:H:N:b:"
const char* Usage = "woofc-namespace-platform -b backend -d application woof directory\n\
\t-H directory for hostwide namelog\n\
\t-m min container count\n\
-t-M max container count\n\
-t-N namespace\n";

#ifdef USE_CMQ
extern "C" {
extern void cmq_pkt_shutdown();
}
#endif
void sig_int_handler(int) {
    fprintf(stdout, "SIGINT caught\n");
    fflush(stdout);

    CleanUpDocker();
#ifdef USE_CMQ
    cmq_pkt_shutdown();
#endif
    exit(0);
}

void CatchSignals() {
    struct sigaction action;

    action.sa_handler = sig_int_handler;
    action.sa_flags = 0;
    sigemptyset(&action.sa_mask);
    sigaddset(&action.sa_mask, SIGINT);
    sigaddset(&action.sa_mask, SIGTERM);
    sigaddset(&action.sa_mask, SIGHUP);
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGTERM, &action, NULL);
    sigaction(SIGHUP, &action, NULL);
}

std::map<std::string, std::function<std::unique_ptr<cspot_backend>()>> backend_makers{
    {"docker", [] { return std::make_unique<docker_backend>(); }},
    {"podman", [] { return std::make_unique<docker_backend>("podman"); }},
    {"spawn", [] { return std::make_unique<proc_backend>(); }}};

int main(int argc, char** argv) {
    char name_dir[2048] = {};
    char name_space[2048] = {};

    auto min_containers = 1;
    auto max_containers = 1;
    std::string backend_name = "docker";

    WooF_is_server = 1; // for signal handling

    int c;
    while ((c = getopt(argc, argv, ARGS)) != EOF) {
        switch (c) {
        case 'm':
            min_containers = atoi(optarg);
            break;
        case 'M':
            max_containers = atoi(optarg);
            break;
        case 'H':
            strncpy(name_dir, optarg, sizeof(name_dir));
            break;
        case 'N':
            strncpy(name_space, optarg, sizeof(name_space));
            break;
        case 'b':
            backend_name = optarg;
            break;
        default:
            fprintf(stderr, "unrecognized command %c\n", (char)c);
            fprintf(stderr, "%s", Usage);
            return 1;
        }
    }

    if (min_containers < 0) {
        fprintf(stderr, "must specify valid number of min containers\n");
        fprintf(stderr, "%s", Usage);
        return 1;
    }

    if (min_containers > max_containers) {
        fprintf(stderr, "min must be <= max containers\n");
        fprintf(stderr, "%s", Usage);
        return 1;
    }

    cspot::set_active_backend(cspot::get_backend_with_name(BACKEND));

    setenv("LD_LIBRARY_PATH", "$LD_LIBRARY_PATH:/usr/local/lib", 1);

    if (name_dir[0] != 0) {
        setenv("WOOF_NAMELOG_DIR", name_dir, 1);
    }

    if (name_space[0] == 0) {
        getcwd(name_space, sizeof(name_space));
    }

    setenv("WOOFC_DIR", name_space, 1);
    setenv("WOOFC_NAMESPACE", name_space, 1);

    DEBUG_LOG("starting platform listening to port %lu\n", WooFPortHash(name_space));

    //fclose(stdin);

    CatchSignals();
    atexit(CleanUpDocker);

    auto err = WooFLocalIP(Host_ip, sizeof(Host_ip));
    DEBUG_FATAL_IF(err < 0, "woofc-namespace-platform no local host IP found\n");

    setenv("WOOF_HOST_IP", Host_ip, 1);

    auto maker_it = backend_makers.find(backend_name);
    if (maker_it == backend_makers.end()) {
        std::cerr << "Unknown backend: " << backend_name << '\n';
        std::cerr << "Available backends: \n";
        for (auto& [name, fn] : backend_makers) {
            std::cerr << " + " << name << '\n';
        }
        exit(1);
    }

    DEBUG_LOG("Using backend %s", backend_name.c_str());

    WooFHostInit(maker_it->second(), min_containers, max_containers);

    pthread_exit(nullptr);
}
