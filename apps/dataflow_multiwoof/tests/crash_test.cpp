#include "../dfinterface.h"

#include <iostream>
#include <string>
#include <vector>

#include <unistd.h>

void print_state() {

    enum LaminarSetupState laminar_setup_state;

    int err = woof_get("laminar-setup-state", &laminar_setup_state, 0);
    if(err < 0) {
        std::cout << "Error reading setup state " << std::endl;
        return;
    }
    
    if(laminar_setup_state == LAMINAR_SETUP_STARTED) {
        std::cout << "Laminar Setup State : STARTED " << std::endl;
    }

    if(laminar_setup_state == LAMINAR_SETUP_FINISHED) {
        std::cout << "Laminar Setup State : FINISHED " << std::endl;
    }

}

void crash_test() {

    set_host(1);

    add_host(1, "127.0.0.1", "/home/centos/cspot/build/bin/");

    int ns = 1;

    add_node(ns, 1, 1, ADD);

    add_operand(ns, 1, 2);
    add_operand(ns, 1, 3);

    subscribe(ns, 1, 0, ns, 2);
    subscribe(ns, 1, 1, ns, 3);

    // test before setup
    print_state();

    setup();

    //test after setup
    print_state();

    sleep(1);

    // 1 + 1 == 2
    operand op1(3.0, 1);
    woof_put(generate_woof_path(OUTPUT_WOOF_TYPE, ns, 2), OUTPUT_HANDLER, &op1);
    sleep(2);
    woof_put(generate_woof_path(OUTPUT_WOOF_TYPE, ns, 3), OUTPUT_HANDLER, &op1);

    do {
        usleep(1e5);
    } while (woof_last_seq(generate_woof_path(OUTPUT_WOOF_TYPE, ns, 2)) == 0);

    woof_get(generate_woof_path(OUTPUT_WOOF_TYPE, ns, 1), &op1, 1);
    woof_get(generate_woof_path(OUTPUT_WOOF_TYPE, ns, 1), &op1, 2);


    std::cout << std::to_string(op1.value) << std::endl;
}


int main() {

    // test before laminar_init
    print_state();
    
    laminar_init();

    //test after laminar_init
    print_state();

    crash_test();
}