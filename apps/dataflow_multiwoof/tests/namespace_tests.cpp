#include "../dfinterface.h"
#include "test_utils.h"
#include "tests.h"

#include <iostream>
#include <string>
#include <vector>

#include <unistd.h>

void simple_namespace_test() {
    TEST("Simple namespace test");
    set_host(1);
    add_host(1, "127.0.0.1", "/home/centos/cspot/build/bin/");

    add_operand(1, 1, 1); // a
    add_operand(1, 1, 2); // b

    add_node(2, 1, 1, ADD);
    add_node(2, 1, 2, MUL);
    add_node(2, 1, 3, MUL);

    add_node(3, 1, 1, ADD);
    add_node(3, 1, 2, MUL);
    add_node(3, 1, 3, MUL);

    add_node(4, 1, 1, ADD);
    add_node(4, 1, 2, MUL);
    add_node(4, 1, 3, MUL);

    subscribe("2:1:0", "1:1");
    subscribe("2:1:1", "1:2");
    subscribe("2:2:0", "2:1");
    subscribe("2:2:1", "2:1");
    subscribe("2:3:0", "2:1");
    subscribe("2:3:1", "2:1");

    subscribe("3:1:0", "2:2");
    subscribe("3:1:1", "2:3");
    subscribe("3:2:0", "3:1");
    subscribe("3:2:1", "3:1");
    subscribe("3:3:0", "3:1");
    subscribe("3:3:1", "3:1");

    subscribe("4:1:0", "3:2");
    subscribe("4:1:1", "3:3");

    setup();

    operand op(1.0);
    woof_put(generate_woof_path(OUTPUT_WOOF_TYPE, 1, 1), OUTPUT_HANDLER, &op);
    woof_put(generate_woof_path(OUTPUT_WOOF_TYPE, 1, 2), OUTPUT_HANDLER, &op);

    do {
        usleep(1e5);
    } while (woof_last_seq(generate_woof_path(OUTPUT_WOOF_TYPE, 4, 1)) == 0);

    std::vector<double> v;
    unsigned long last = woof_last_seq(generate_woof_path(OUTPUT_WOOF_TYPE, 4, 1));
    for (unsigned long i = 1; i <= last; i++) {
        woof_get(generate_woof_path(OUTPUT_WOOF_TYPE, 4, 1), &op, i);
        v.push_back(op.value);
    }

    // // Expected: 128
    // std::cout << "OUTPUTS: ";
    // for (auto& i : v) {
    //     std::cout << i << " ";
    // }

    ASSERT_EQ(v[0], 128, "2(2(1+1)^2)^2 = 128");

    END_TEST();
}

void namespace_tests() {
    
    simple_namespace_test();
}