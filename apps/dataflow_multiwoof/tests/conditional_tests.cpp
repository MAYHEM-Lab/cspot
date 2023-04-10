#include "../dfinterface.h"
#include "test_utils.h"
#include "tests.h"

#include <iostream>
#include <string>
#include <unistd.h>
#include <vector>

void selector_test() {
    TEST("Selector test");

    set_host(1);
    add_host(1, "127.0.0.1", "/home/centos/cspot/build/bin/");

    int ns = 1;

    add_node(ns, 1, 1, SEL); // a or b

    add_operand(ns, 1, 2); // Selector (0 or 1)
    add_operand(ns, 1, 3); // a
    add_operand(ns, 1, 4); // b

    subscribe(ns, 1, 0, ns, 2); // Selector --> SEL:0
    subscribe(ns, 1, 1, ns, 3); // a --> SEL:1
    subscribe(ns, 1, 2, ns, 4); // b --> SEL:2

    setup();

    for (int i = 1; i <= 2; i++) {
        operand op(i - 1, i); // Selector
        woof_put(generate_woof_path(OUTPUT_WOOF_TYPE, ns, 2), OUTPUT_HANDLER, &op);
        operand a(10, i); // a
        woof_put(generate_woof_path(OUTPUT_WOOF_TYPE, ns, 3), OUTPUT_HANDLER, &a);
        operand b(20, i); // b
        woof_put(generate_woof_path(OUTPUT_WOOF_TYPE, ns, 4), OUTPUT_HANDLER, &b);
    }

    do {
        usleep(1e5);
    } while (woof_last_seq(generate_woof_path(OUTPUT_WOOF_TYPE, ns, 1)) < 2);

    operand r1;
    woof_get(generate_woof_path(OUTPUT_WOOF_TYPE, ns, 1), &r1, 1);
    ASSERT_EQ(r1.value, 10, "SEL = 0 -> result = a = 10");

    operand r2;
    woof_get(generate_woof_path(OUTPUT_WOOF_TYPE, ns, 1), &r2, 0);
    ASSERT_EQ(r2.value, 20, "SEL = 1 -> result = b = 20");

    END_TEST();
}

void filter_test() {
    TEST("Filter test");

    set_host(1);
    add_host(1, "127.0.0.1", "/home/centos/cspot/build/bin/");

    int ns = 1;

    add_node(ns, 1, 1, FILTER);

    add_operand(ns, 1, 2); // Filter condition
    add_operand(ns, 1, 3); // Value

    subscribe(ns, 1, 0, ns, 2);
    subscribe(ns, 1, 1, ns, 3);

    setup();

    operand filter1(0.0, 1);
    operand data1(1.0, 1);
    woof_put(generate_woof_path(OUTPUT_WOOF_TYPE, ns, 2), OUTPUT_HANDLER, &filter1);
    woof_put(generate_woof_path(OUTPUT_WOOF_TYPE, ns, 3), OUTPUT_HANDLER, &data1);

    operand filter2(1.0, 2);
    operand data2(2.0, 2);
    woof_put(generate_woof_path(OUTPUT_WOOF_TYPE, ns, 2), OUTPUT_HANDLER, &filter2);
    woof_put(generate_woof_path(OUTPUT_WOOF_TYPE, ns, 3), OUTPUT_HANDLER, &data2);

    operand filter3(1.0, 3);
    operand data3(3.0, 3);
    woof_put(generate_woof_path(OUTPUT_WOOF_TYPE, ns, 2), OUTPUT_HANDLER, &filter3);
    woof_put(generate_woof_path(OUTPUT_WOOF_TYPE, ns, 3), OUTPUT_HANDLER, &data3);

    operand filter4(0.0, 4);
    operand data4(4.0, 4);
    woof_put(generate_woof_path(OUTPUT_WOOF_TYPE, ns, 2), OUTPUT_HANDLER, &filter4);
    woof_put(generate_woof_path(OUTPUT_WOOF_TYPE, ns, 3), OUTPUT_HANDLER, &data4);

    operand filter5(1.0, 5);
    operand data5(5.0, 5);
    woof_put(generate_woof_path(OUTPUT_WOOF_TYPE, ns, 2), OUTPUT_HANDLER, &filter5);
    woof_put(generate_woof_path(OUTPUT_WOOF_TYPE, ns, 3), OUTPUT_HANDLER, &data5);

    do {
        usleep(1e5);
    } while (woof_last_seq(generate_woof_path(OUTPUT_WOOF_TYPE, ns, 1)) < 3);

    std::vector<operand> v;
    unsigned long last = woof_last_seq(generate_woof_path(OUTPUT_WOOF_TYPE, ns, 1));
    for (unsigned long i = 1; i <= last; i++) {
        woof_get(generate_woof_path(OUTPUT_WOOF_TYPE, ns, 1), &data1, i);
        v.push_back(data1);
    }

    // // Expected: 2 3 5
    // std::cout << "OUTPUTS: ";
    // for (auto& i : v) {
    //     std::cout << i.value << " ";
    // }

    // Note: duplicates are allowed
    std::vector<unsigned long> expected = {NULL, NULL, 2, 3, NULL, 5};
    unsigned long prev_seq = 1;
    for (auto& op : v) {
        ASSERT(op.seq >= prev_seq, "Sequence increases monotonically");
        ASSERT_EQ(op.value, expected[op.seq], "Ensure value is correct for each sequence number");
        prev_seq = op.seq;
    }

    END_TEST();
}

void conditional_tests() {

    selector_test();
    filter_test();
}
