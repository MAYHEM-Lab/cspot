#include "../dfinterface.h"

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <random>
#include <cmath>

#include <unistd.h>


void simple_test() {
    std::ofstream out("test.txt");
    int ns = 1;
    std::string p = "laminar-" + std::to_string(ns);


    add_node(ns, 1, ADD);
    add_node(ns, 2, MUL);

    add_operand(ns, 3);
    add_operand(ns, 4);
    add_operand(ns, 5);

    subscribe(ns, 1, 0, ns, 3);
    subscribe(ns, 1, 1, ns, 4);
    subscribe(ns, 2, 0, ns, 1);
    subscribe(ns, 2, 1, ns, 5);

    setup(ns);

    sleep(1);

    out << "subscriber_map size: " << woof_last_seq(p + ".subscriber_map") << std::endl;

    unsigned long last_seq;
    unsigned long idx;
    subscriber sub;

    last_seq = woof_last_seq(p + ".subscriber_map");
    for (unsigned long seq = 1; seq <= last_seq; seq++) {
        woof_get(p + ".subscriber_map", &idx, seq);
        out << seq << ": " << idx << std::endl;
    }

    out << std::endl;
    out << "subscriber_data size: " << woof_last_seq(p + ".subscriber_data") << std::endl;

    last_seq = woof_last_seq(p + ".subscriber_data");
    for (unsigned long seq = 1; seq <= last_seq; seq++) {
        woof_get(p + ".subscriber_data", &sub, seq);
        out << seq << ": {id=" << sub.id << ", port=" << sub.port << "}" << std::endl;
    }

    out << std::endl;
    out << "subscription_map size: " << woof_last_seq(p + ".subscription_map") << std::endl;

    last_seq = woof_last_seq(p + ".subscription_map");
    for (unsigned long seq = 1; seq <= last_seq; seq++) {
        woof_get(p + ".subscription_map", &idx, seq);
        out << seq << ": " << idx << std::endl;
    }

    out << std::endl;
    out << "subscription_data size: " << woof_last_seq(p + ".subscription_data") << std::endl;

    last_seq = woof_last_seq(p + ".subscription_data");
    for (unsigned long seq = 1; seq <= last_seq; seq++) {
        woof_get(p + ".subscription_data", &sub, seq);
        out << seq << ": {id=" << sub.id << ", port=" << sub.port << "}" << std::endl;
    }

    operand op(100.0);
    woof_put(p + ".output.3", "output_handler", &op);// sleep(2);
    woof_put(p + ".output.4", "output_handler", &op);// sleep(2);
    woof_put(p + ".output.5", "output_handler", &op);// sleep(2);

    sleep(2);
    subscription_event se;
    woof_get(p + ".subscription_events.1", &se, 1);
    std::cout << "{id=" << se.id << ", port=" << se.port << "}" << std::endl;

    operand result_node1, result_node2;
    woof_get(p + ".output.1", &result_node1, 1);
    woof_get(p + ".output.2", &result_node2, 1);
    std::cout << "node #1 result = " << result_node1.value << std::endl;
    std::cout << "node #2 result = " << result_node2.value << std::endl;

    std::cout << "DONE" << std::endl;
}


void simple_test_2() {
    int ns = 1;
    std::string p = "laminar-" + std::to_string(ns);

    add_node(ns, 1, ADD);

    add_operand(ns, 2);
    add_operand(ns, 3);

    subscribe(ns, 1, 0, ns, 2);
    subscribe(ns, 1, 1, ns, 3);

    setup(ns);
    sleep(1);

    operand op1(1.0, 1);
    woof_put(p + ".output.2", "output_handler", &op1);
    woof_put(p + ".output.3", "output_handler", &op1);

    operand op2(2.0, 2);
    woof_put(p + ".output.2", "output_handler", &op2);
    woof_put(p + ".output.3", "output_handler", &op2);

    operand op3(3.0, 3);
    operand op4(3.0, 4);
    woof_put(p + ".output.2", "output_handler", &op3);
    woof_put(p + ".output.2", "output_handler", &op4);

    woof_put(p + ".output.3", "output_handler", &op3);
    woof_put(p + ".output.3", "output_handler", &op4);

    sleep(3);


    std::vector<operand> v;
    unsigned long last = woof_last_seq(p + ".output.1");
    for (unsigned long i = 1; i <= last; i++) {
        woof_get(p + ".output.1", &op1, i);
        v.push_back(op1);
    }

    // Expected: 2 4 6 6
    std::cout << "OUTPUTS: ";
    for (auto& i : v) {
        std::cout << i.seq << ": " << i.value << " | ";
    }
    std::cout << std::endl;
}


void quadratic_test(double a, double b, double c) {
    int ns = 1;
    std::string p = "laminar-" + std::to_string(ns);

    add_node(ns, 1, DIV);
    add_node(ns, 2, MUL);
    add_node(ns, 3, MUL);
    add_node(ns, 4, ADD);
    add_node(ns, 5, SQR);
    add_node(ns, 6, SUB);
    add_node(ns, 7, MUL);
    add_node(ns, 8, MUL);

    add_operand(ns, 9); // a
    add_operand(ns, 10); // b
    add_operand(ns, 11); // c
    add_operand(ns, 12); // 2 for 2 * a
    add_operand(ns, 13); // -1 for -1 * b
    add_operand(ns, 14); // 4 for 4 * a * c

    subscribe(ns, 1, 0, ns, 4);
    subscribe(ns, 1, 1, ns, 2);
    subscribe(ns, 2, 0, ns, 9);
    subscribe(ns, 2, 1, ns, 12);
    subscribe(ns, 3, 0, ns, 13);
    subscribe(ns, 3, 1, ns, 10);
    subscribe(ns, 4, 0, ns, 5);
    subscribe(ns, 4, 1, ns, 3);
    subscribe(ns, 5, 0, ns, 6);
    subscribe(ns, 6, 0, ns, 7);
    subscribe(ns, 6, 1, ns, 8);
    subscribe(ns, 7, 0, ns, 10);
    subscribe(ns, 7, 1, ns, 10);
    subscribe(ns, 8, 0, ns, 9);
    subscribe(ns, 8, 1, ns, 11);
    subscribe(ns, 8, 2, ns, 14);

    setup(ns);

    sleep(2);

    std::cout << "Finished setup" << std::endl;

    operand op(a);
    woof_put(p + ".output.9", "output_handler", &op);
    op.value = b;
    woof_put(p + ".output.10", "output_handler", &op);
    op.value = c;
    woof_put(p + ".output.11", "output_handler", &op);
    op.value = 2;
    woof_put(p + ".output.12", "output_handler", &op);
    op.value = -1;
    woof_put(p + ".output.13", "output_handler", &op);
    op.value = 4;
    woof_put(p + ".output.14", "output_handler", &op);
    
    do {
        sleep(1);
    } while (woof_last_seq(p + ".output.1") == 0);

    operand result;
    for (int i = 14; i > 0; i--) {
        woof_get(p + ".output." + std::to_string(i), &result, 1);
        std::cout << "node #" << i << " result = " << result.value << std::endl;
    }

    std::cout << "DONE" << std::endl;
}

void quadratic_graphviz_test() {
    int ns = 1;

    add_node(ns, 1, DIV);
    add_node(ns, 2, MUL);
    add_node(ns, 3, MUL);
    add_node(ns, 4, ADD);
    add_node(ns, 5, SQR);
    add_node(ns, 6, SUB);
    add_node(ns, 7, MUL);
    add_node(ns, 8, MUL);

    add_operand(ns, 9); // a
    add_operand(ns, 10); // b
    add_operand(ns, 11); // c
    add_operand(ns, 12); // 2 for 2 * a
    add_operand(ns, 13); // -1 for -1 * b
    add_operand(ns, 14); // 4 for 4 * a * c

    subscribe(ns, 1, 0, ns, 4);
    subscribe(ns, 1, 1, ns, 2);
    subscribe(ns, 2, 0, ns, 9);
    subscribe(ns, 2, 1, ns, 12);
    subscribe(ns, 3, 0, ns, 13);
    subscribe(ns, 3, 1, ns, 10);
    subscribe(ns, 4, 0, ns, 5);
    subscribe(ns, 4, 1, ns, 3);
    subscribe(ns, 5, 0, ns, 6);
    subscribe(ns, 6, 0, ns, 7);
    subscribe(ns, 6, 1, ns, 8);
    subscribe(ns, 7, 0, ns, 10);
    subscribe(ns, 7, 1, ns, 10);
    subscribe(ns, 8, 0, ns, 9);
    subscribe(ns, 8, 1, ns, 11);
    subscribe(ns, 8, 2, ns, 14);

    std::cout << graphviz_representation() << std::endl;
}

void namespace_graphviz_test() {

    add_operand(1, 1); // a
    add_operand(1, 2); // b

    add_node(2, 1, ADD);
    add_node(2, 2, MUL);
    add_node(2, 3, MUL);

    add_node(3, 1, ADD);
    add_node(3, 2, MUL);
    add_node(3, 3, MUL);

    add_node(4, 1, ADD);
    add_node(4, 2, MUL);
    add_node(4, 3, MUL);

    // subscribe(1, 1, 0, 4, 1);
    // subscribe(1, 1, 1, 4, 2);
    // subscribe(1, 2, 0, 1, 1);
    // subscribe(1, 2, 1, 1, 1);
    // subscribe(1, 3, 0, 1, 1);
    // subscribe(1, 3, 1, 1, 1);

    subscribe("2:1:0", "1:1");
    subscribe("2:1:1", "1:2");
    subscribe("2:2:0", "2:1");
    subscribe("2:2:1", "2:1");
    subscribe("2:3:0", "2:1");
    subscribe("2:3:1", "2:1");

    // subscribe(2, 1, 0, 1, 2);
    // subscribe(2, 1, 1, 1, 3);
    // subscribe(2, 2, 0, 2, 1);
    // subscribe(2, 2, 1, 2, 1);
    // subscribe(2, 3, 0, 2, 1);
    // subscribe(2, 3, 1, 2, 1);

    subscribe("3:1:0", "2:2");
    subscribe("3:1:1", "2:3");
    subscribe("3:2:0", "3:1");
    subscribe("3:2:1", "3:1");
    subscribe("3:3:0", "3:1");
    subscribe("3:3:1", "3:1");

    // subscribe(3, 1, 0, 2, 2);
    // subscribe(3, 1, 1, 2, 3);
    // subscribe(3, 2, 0, 3, 1);
    // subscribe(3, 2, 1, 3, 1);
    // subscribe(3, 3, 0, 3, 1);
    // subscribe(3, 3, 1, 3, 1);

    subscribe("4:1:0", "3:2");
    subscribe("4:1:1", "3:3");
    subscribe("4:2:0", "4:1");
    subscribe("4:2:1", "4:1");
    subscribe("4:3:0", "4:1");
    subscribe("4:3:1", "4:1");

    std::cout << graphviz_representation() << std::endl;
}

void selector_test() {
    int ns = 1;
    std::string p = "laminar-" + std::to_string(ns);

    add_node(ns, 1, ADD); // (a or b) + 1
    add_node(ns, 2, SEL); // a or b

    add_operand(ns, 3); // a
    add_operand(ns, 4); // b
    add_operand(ns, 5); // 1
    add_operand(ns, 6); // Selector (0 or 1)

    subscribe(ns, 1, 0, ns, 2); // SEL --> ADD:0
    subscribe(ns, 1, 1, ns, 5); // 1 --> ADD:1
    subscribe(ns, 2, 0, ns, 6); // Selector --> SEL:0
    subscribe(ns, 2, 1, ns, 3); // a --> SEL:1
    subscribe(ns, 2, 2, ns, 4); // b --> SEL:2

    setup(ns); sleep(2);
    std::cout << "Finished setup" << std::endl;

    operand op(10); // a
    woof_put(p + ".output.3", "output_handler", &op);
    op.value = 20;  // b
    woof_put(p + ".output.4", "output_handler", &op);
    op.value = 1;   // 1
    woof_put(p + ".output.5", "output_handler", &op);
    op.value = 1;   // Selector
    woof_put(p + ".output.6", "output_handler", &op);
    
    do {
        sleep(1);
    } while (woof_last_seq(p + ".output.1") == 0);

    operand result;
    // woof_get(p + ".output.1", &result, 1);
    // std::cout << "result = " << result.value << std::endl;
    for (int i = 6; i > 0; i--) {
        woof_get(p + ".output." + std::to_string(i), &result, 1);
        std::cout << "node #" << i << " result = " << result.value << std::endl;
    }
    // Selector = 0 --> a --> result = a + 1 = 11
    // Selector = 1 --> b --> result = b + 1 = 21

    std::cout << "DONE" << std::endl;
}

void filter_test() {
    int ns = 1;
    std::string p = "laminar-" + std::to_string(ns);

    add_node(ns, 1, FILTER);

    add_operand(ns, 2); // Filter condition
    add_operand(ns, 3); // Value

    subscribe(ns, 1, 0, ns, 2);
    subscribe(ns, 1, 1, ns, 3);

    setup(ns);
    sleep(1);

    operand filter(0.0);
    operand data(1.0);
    woof_put(p + ".output.2", "output_handler", &filter);
    woof_put(p + ".output.3", "output_handler", &data);

    filter.value = 1.0;
    data.value = 2.0;
    woof_put(p + ".output.2", "output_handler", &filter);
    woof_put(p + ".output.3", "output_handler", &data);

    filter.value = 1.0;
    data.value = 3.0;
    woof_put(p + ".output.2", "output_handler", &filter);
    woof_put(p + ".output.3", "output_handler", &data);

    filter.value = 0.0;
    data.value = 4.0;
    woof_put(p + ".output.2", "output_handler", &filter);
    woof_put(p + ".output.3", "output_handler", &data);

    filter.value = 1.0;
    data.value = 5.0;
    woof_put(p + ".output.2", "output_handler", &filter);
    woof_put(p + ".output.3", "output_handler", &data);

    sleep(2);


    std::vector<double> v;
    unsigned long last = woof_last_seq(p + ".output.1");
    for (unsigned long i = 1; i <= last; i++) {
        woof_get(p + ".output.1", &data, i);
        v.push_back(data.value);
    }

    // Expected: 2 3 5
    std::cout << "OUTPUTS: ";
    for (auto& i : v) {
        std::cout << i << " ";
    }
}

void namespace_test() {
    add_operand(1, 1); // a
    add_operand(1, 2); // b

    add_node(2, 1, ADD);
    add_node(2, 2, MUL);
    add_node(2, 3, MUL);

    add_node(3, 1, ADD);
    add_node(3, 2, MUL);
    add_node(3, 3, MUL);

    add_node(4, 1, ADD);
    add_node(4, 2, MUL);
    add_node(4, 3, MUL);

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

    setup(1);
    setup(2);
    setup(3);
    setup(4);

    sleep(1);

    operand op(1.0);
    woof_put("laminar-1.output.1", "output_handler", &op);
    woof_put("laminar-1.output.2", "output_handler", &op);

    sleep(2);

    std::vector<double> v;
    unsigned long last = woof_last_seq("laminar-4.output.1");
    for (unsigned long i = 1; i <= last; i++) {
        woof_get("laminar-4.output.1", &op, i);
        v.push_back(op.value);
    }

    // Expected: 128
    std::cout << "OUTPUTS: ";
    for (auto& i : v) {
        std::cout << i << " ";
    }


}

void sqrt_loop_test() {
    /* Nodes */
    
    // Inputs
    add_operand(1, 1);      // X
    add_operand(1, 2);      // Epsilon

    // // Initialization
    add_node(2, 1, DIV);    // Root = X / 2.0
    add_node(2, 2, SEL);    // Initial root or body output?
    add_node(2, 3, OFFSET); // Account for no body output before first iter
    add_operand(2, 4);      // 2.0
    add_operand(2, 5);      // SEL: root, body, body, ...
    add_operand(2, 6);      // Offset = 1
    
    // Test
    add_node(3, 1, MUL);    
    add_node(3, 2, SUB);
    add_node(3, 3, ABS);
    add_node(3, 4, LT);
    add_node(3, 5, NOT);
    add_node(3, 6, FILTER); // Repeat body
    add_node(3, 7, FILTER); // Produce result
    
    // Body
    add_node(4, 1, DIV);
    add_node(4, 2, ADD);
    add_node(4, 3, DIV);    // Root
    add_operand(4, 4);      // 2.0

    /* Edges */

    // Initialization
    subscribe("2:1:0", "1:1");  // Root = X / _
    subscribe("2:1:1", "2:4");  // Root = X / 2.0
    subscribe("2:2:0", "2:5");  // Selector
    subscribe("2:2:1", "2:1");  // SEL=0: Root = X / 2.0
    subscribe("2:2:2", "2:3");  // SEL=1: Root = result of body
    subscribe("2:3:0", "2:6");  // Offset = 1
    subscribe("2:3:1", "3:6");  // Result of body with offset

    // Test
    subscribe("3:1:0", "4:3");  // 3N1 = Root * _
    subscribe("3:1:1", "4:3");  // 3N1 = Root * Root
    subscribe("3:2:0", "1:1");  // 3N2 = X - _
    subscribe("3:2:1", "3:1");  // 3N2 = X - 3N1
    subscribe("3:3:0", "3:2");  // 3N3 = ABS(3N2)
    subscribe("3:4:0", "3:3");  // 3N4 = 3N3 < _
    subscribe("3:4:1", "1:2");  // 3N4 = 3N3 < Epsilon
    subscribe("3:5:0", "3:4");  // 3N5 = NOT(3N4)
    subscribe("3:6:0", "3:5");  // Repeat if !(delta < epsilon)
    subscribe("3:6:1", "4:3");  // Pass root back to body
    subscribe("3:7:0", "3:4");  // Result if (delta < epsilon)
    subscribe("3:7:1", "4:3");  // Pass root to result
    
    // Body
    subscribe("4:1:0", "1:1");  // 4N1 = X / _
    subscribe("4:1:1", "2:2");  // 4N1 = X / Root
    subscribe("4:2:0", "4:1");  // 4N2 = 4N1 + _
    subscribe("4:2:1", "2:2");  // 4N2 = 4N1 + Root
    subscribe("4:3:0", "4:2");  // 4N3 = 4N2 / _
    subscribe("4:3:1", "4:4");  // 4N3 = 4N2 / 2.0

    /* Run program */

    setup(1);
    setup(2);
    setup(3);
    setup(4);

    sleep(1);

    double x = 144.0;
    double epsilon = 10;

    

    operand op(2.0, 1);
    woof_put("laminar-4.output.4", "output_handler", &op);
    operand op2(2.0, 2);
    woof_put("laminar-4.output.4", "output_handler", &op2);
    operand op3(2.0, 3);
    woof_put("laminar-4.output.4", "output_handler", &op3);
    operand op4(2.0, 4);
    woof_put("laminar-4.output.4", "output_handler", &op4);

    op.value = op2.value = op3.value = op4.value = x;
    woof_put("laminar-1.output.1", "output_handler", &op);
    woof_put("laminar-1.output.1", "output_handler", &op2);
    woof_put("laminar-1.output.1", "output_handler", &op3);
    woof_put("laminar-1.output.1", "output_handler", &op4);
    op.value = op2.value = op3.value = op4.value = epsilon;
    woof_put("laminar-1.output.2", "output_handler", &op);
    woof_put("laminar-1.output.2", "output_handler", &op2);
    woof_put("laminar-1.output.2", "output_handler", &op3);
    woof_put("laminar-1.output.2", "output_handler", &op4);

    // Initialization

    // Seed initialization feedback with junk (not used in first iter)
    op.value = 0;
    woof_put("laminar-2.output.3", "output_handler", &op);
    // unsigned long consumer_ptr = 2;
    // woof_put("laminar-2.subscription_pointer.3", "", &consumer_ptr);

    op.value = op2.value = op3.value = op4.value = 2.0;
    woof_put("laminar-2.output.4", "output_handler", &op);
    woof_put("laminar-2.output.4", "output_handler", &op2);
    woof_put("laminar-2.output.4", "output_handler", &op3);
    woof_put("laminar-2.output.4", "output_handler", &op4);
    op.value = 0;
    op2.value = op3.value = op4.value = 1;
    woof_put("laminar-2.output.5", "output_handler", &op);
    woof_put("laminar-2.output.5", "output_handler", &op2);
    woof_put("laminar-2.output.5", "output_handler", &op3);
    woof_put("laminar-2.output.5", "output_handler", &op4);
    op.value = op2.value = op3.value = op4.value = 1;
    woof_put("laminar-2.output.6", "output_handler", &op);
    woof_put("laminar-2.output.6", "output_handler", &op2);
    woof_put("laminar-2.output.6", "output_handler", &op3);
    woof_put("laminar-2.output.6", "output_handler", &op4);

    sleep(4);

    std::vector<double> v;
    unsigned long last = woof_last_seq("laminar-3.output.6");
    for (unsigned long i = 1; i <= last; i++) {
        woof_get("laminar-3.output.6", &op, i);
        v.push_back(op.value);
    }

    double result;
    woof_get("laminar-3.output.7", &op, 0);
    result = op.value;

    // Expected: 37, 20.4459, ...
    std::cout << "Intermediate values: ";
    for (auto& i : v) {
        std::cout << i << " | ";
    }
    std::cout << std::endl;

    std::cout << "Result: " << result << std::endl;
    
}

void mat_test(const std::vector<std::vector<double>>& a,
              const std::vector<std::vector<double>>& b) {
    int rows_a = a.size();
    int cols_a = a[0].size();
    int rows_b = b.size();
    int cols_b = b[0].size();

    // Result matrix dimensions    
    int rows_r = a.size();
    int cols_r = b[0].size();

    // Create operands
    for (int i = 0; i < rows_a; i++) {
        for (int j = 0; j < cols_a; j++) {
            add_operand(1, i * cols_a + j + 1);
        }
    }

    for (int i = 0; i < rows_b; i++) {
        for (int j = 0; j < cols_b; j++) {
            add_operand(2, i * cols_b + j + 1);
        }
    }

    int id;
    std::string id_str;
    for (int r = 0; r < rows_r; r++) {
        for (int c = 0; c < cols_r; c++) {
            // Create addition node for intermediate products
            add_node(4, r * cols_r + c + 1, ADD);

            // Create all multiplication nodes for one output cell
            for (int i = 0; i < cols_a; i++) {
                id = r * (cols_r * cols_a) + c * cols_a + i + 1;
                add_node(3, id, MUL);
                subscribe(3, id, 0, 1, r * cols_a + i + 1);
                subscribe(3, id, 1, 2, i * cols_b + c + 1);

                // Connect product to be summed
                subscribe(4, r * cols_r + c + 1, i, 3, id);
            }
        }
    }
    
    /* Run program */

    setup(1);
    setup(2);
    setup(3);
    setup(4);
    sleep(1);

    // Write matrices to operands
    for (int i = 0; i < rows_a; i++) {
        for (int j = 0; j < cols_a; j++) {
            operand op(a[i][j], 1);
            id = i * cols_a + j + 1;
            id_str = std::to_string(id);
            woof_put("laminar-1.output." + id_str, "output_handler", &op);
        }
    }

    for (int i = 0; i < rows_b; i++) {
        for (int j = 0; j < cols_b; j++) {
            operand op(b[i][j], 1);
            id = i * cols_b + j + 1;
            id_str = std::to_string(id);
            woof_put("laminar-2.output." + id_str, "output_handler", &op);
        }
    }    

    sleep(3);

    operand op;
    std::vector<std::vector<double>> v;
    for (int r = 0; r < rows_r; r++) {
        v.push_back({});
        for (int c = 0; c < cols_r; c++) {
            id = r * cols_r + c + 1;
            id_str = std::to_string(id);
            woof_get("laminar-4.output." + id_str, &op, 1);
            v[r].push_back(op.value);
        }
    }

    std::cout << "OUTPUTS:" << std::endl;
    for (auto& row : v) {
        for (auto& i : row) {
            std::cout << i << " ";
        }
        std::cout << std::endl;
    }
}

void knn_test() {
    add_operand(1, 1);      // K
    add_operand(1, 2);      // x
    add_operand(1, 3);      // y

    add_node(2, 1, KNN);

    subscribe("2:1:0", "1:1");
    subscribe("2:1:1", "1:2");
    subscribe("2:1:2", "1:3");

    // KNN Initialization

    std::string knn_data_woof = "laminar-2.knn_data.1";
    woof_create(knn_data_woof, sizeof(Point), 25);

    std::vector<Point> points = {
        Point(1, 0, 0),  Point(1, 1, 0),  Point(0, 1, 0), Point(10, 10, 1),
        Point(9, 10, 1), Point(10, 9, 1), Point(9, 9, 1)};

    for (auto& p : points) {
        woof_put(knn_data_woof, "", &p);
    }

    setup(1);
    setup(2);
    sleep(1);

    int k = 3;
    Point p = Point(7, 8);

    operand op(k, 1);
    woof_put("laminar-1.output.1", "output_handler", &op);

    operand op2(p.x, 1);
    woof_put("laminar-1.output.2", "output_handler", &op2);

    operand op3(p.y, 1);
    woof_put("laminar-1.output.3", "output_handler", &op3);

    sleep(2);

    double result;
    woof_get("laminar-2.output.1", &op, 0);
    result = op.value;

    // Expected output: 1
    std::cout << "LABEL: " << result << std::endl;
}

void online_linreg_test() {
    add_node(1, 1, LINREG); // Linear Regression node
    add_operand(1, 2);      // x
    add_operand(1, 3);      // y

    subscribe("1:1:0", "1:2");
    subscribe("1:1:1", "1:3");

    // Linear Regression Initialization

    int iters = 2;

    std::string data_woof = "laminar-1.linreg_data.1";
    woof_create(data_woof, sizeof(Regression), 100);

    setup(1);

    std::random_device rd;
    std::default_random_engine eng(rd());
    std::uniform_real_distribution<double> distr(-0.1, 0.1);

    std::vector<operand> x_values;
    std::vector<operand> y_values;

    for (int i = 0; i < iters; i++) {
        double x = i + distr(eng);
        double y = 3 + 2 * i + distr(eng);
        x_values.push_back(operand(x, i + 1));
        y_values.push_back(operand(y, i + 1));
    }

    for (int i = 0; i < iters; i++) {
        woof_put("laminar-1.output.2", "output_handler", &x_values[i]);
        woof_put("laminar-1.output.3", "output_handler", &y_values[i]);
    }

    while (woof_last_seq("laminar-1.output.1") < iters) {
        sleep(1);
    }

    operand op;
    double result;
    woof_get("laminar-1.output.1", &op, 0);
    result = op.value;
    std::cout << "result: " << result << std::endl;
}

void online_linreg_multinode() {
    // Update variables (num, x, y, xx, xy)

    add_node(1, 1, MUL);    // num *= decay_rate
    add_node(1, 2, MUL);    // x *= decay_rate
    add_node(1, 3, MUL);    // y *= decay_rate
    add_node(1, 4, MUL);    // xx *= decay_rate
    add_node(1, 5, MUL);    // xy *= decay_rate
    add_node(1, 6, ADD);    // num += 1
    add_node(1, 7, ADD);    // x += new_x
    add_node(1, 8, ADD);    // y += new_y
    add_node(1, 9, MUL);    // new_x ^ 2
    add_node(1, 10, ADD);   // xx += new_x ^ 2
    add_node(1, 11, MUL);   // new_x * new_y
    add_node(1, 12, ADD);   // xy += new_x * new_y
    add_node(1, 13, OFFSET);// num seq + 1
    add_node(1, 14, OFFSET);// x seq + 1
    add_node(1, 15, OFFSET);// y seq + 1
    add_node(1, 16, OFFSET);// xx seq + 1
    add_node(1, 17, OFFSET);// xy seq + 1
    add_node(1, 18, SEL);   // num or 0?
    add_node(1, 19, SEL);   // x or 0?
    add_node(1, 20, SEL);   // y or 0?
    add_node(1, 21, SEL);   // xx or 0?
    add_node(1, 22, SEL);   // xy or 0?

    // Calculate slope and intercept

    add_node(2, 1, MUL);    // num * xx
    add_node(2, 2, MUL);    // x * x
    add_node(2, 3, SUB);    // det = num * xx - x * x
    add_node(2, 4, MUL);    // xx * y
    add_node(2, 5, MUL);    // xy * x
    add_node(2, 6, SUB);    // xx * y - xy * x
    add_node(2, 7, DIV);    // intercept = (xx * y - xy * x) / det;
    add_node(2, 8, MUL);    // xy * num
    add_node(2, 9, MUL);    // x * y
    add_node(2, 10, SUB);   // xy * num - x * y
    add_node(2, 11, DIV);   // slope = (xy * num - x * y) / det;
    add_node(2, 12, GT);    // det > 1e-10?
    // add_node(2, 13, SEL);   // intercept or 0?
    // add_node(2, 14, SEL);   // slope or 0?

    // Constants

    add_operand(3, 1);      // const 1
    add_operand(3, 2);      // const 0
    add_operand(3, 3);      // decay_factor = exp(-dt / T)
    add_operand(3, 4);      // handle init (0, 1, 1, ..., 1)
    add_operand(3, 5);      // const 1e-10

    // Inputs

    add_operand(4, 1);      // input x
    add_operand(4, 2);      // input y

    // Outputs

    add_node(5, 1, SEL);         // intercept = intercept or 0
    add_node(5, 2, SEL);         // slope = slope or 0

    // Edges

    // Update num, x, y, xx, xy
    subscribe("1:1:0", "3:3");      // decay_rate * _
    subscribe("1:1:1", "1:18");     // decay_rate * num
    subscribe("1:2:0", "3:3");      // decay_rate * _
    subscribe("1:2:1", "1:19");     // decay_rate * x
    subscribe("1:3:0", "3:3");      // decay_rate * _
    subscribe("1:3:1", "1:20");     // decay_rate * y
    subscribe("1:4:0", "3:3");      // decay_rate * _
    subscribe("1:4:1", "1:21");     // decay_rate * xx
    subscribe("1:5:0", "3:3");      // decay_rate * _
    subscribe("1:5:1", "1:22");     // decay_rate * xy
    subscribe("1:6:0", "3:1");      // _ + 1
    subscribe("1:6:1", "1:1");      // num += 1
    subscribe("1:7:0", "4:1");      // _ + new_x
    subscribe("1:7:1", "1:2");      // x += new_x
    subscribe("1:8:0", "4:2");      // _ + new_y
    subscribe("1:8:1", "1:3");      // y += new_y
    subscribe("1:9:0", "4:1");      // new_x * _
    subscribe("1:9:1", "4:1");      // new_x * new_x
    subscribe("1:10:0", "1:9");     // _ + new_x * new_x
    subscribe("1:10:1", "1:4");     // xx += new_x * new_x
    subscribe("1:11:0", "4:1");     // x * _
    subscribe("1:11:1", "4:2");     // x * y
    subscribe("1:12:0", "1:11");    // _ + x * y
    subscribe("1:12:1", "1:5");     // xy += x * y

    // Feedback offset
    subscribe("1:13:0", "3:1");     // offset = 1
    subscribe("1:13:1", "1:6");     // num seq + 1
    subscribe("1:14:0", "3:1");     // offset = 1
    subscribe("1:14:1", "1:7");     // x seq + 1
    subscribe("1:15:0", "3:1");     // offset = 1
    subscribe("1:15:1", "1:8");     // y seq + 1
    subscribe("1:16:0", "3:1");     // offset = 1
    subscribe("1:16:1", "1:10");    // xx seq + 1
    subscribe("1:17:0", "3:1");     // offset = 1
    subscribe("1:17:1", "1:12");    // xy seq + 1
    subscribe("1:18:0", "3:4");     // SEL: 0, 1, ..., 1
    subscribe("1:18:1", "3:2");     // 0 or _?
    subscribe("1:18:2", "1:13");    // 0 or num?
    subscribe("1:19:0", "3:4");     // SEL: 0, 1, ..., 1
    subscribe("1:19:1", "3:2");     // 0 or _?
    subscribe("1:19:2", "1:14");    // 0 or x?
    subscribe("1:20:0", "3:4");     // SEL: 0, 1, ..., 1
    subscribe("1:20:1", "3:2");     // 0 or _?
    subscribe("1:20:2", "1:15");    // 0 or y?
    subscribe("1:21:0", "3:4");     // SEL: 0, 1, ..., 1
    subscribe("1:21:1", "3:2");     // 0 or _?
    subscribe("1:21:2", "1:16");    // 0 or xx?
    subscribe("1:22:0", "3:4");     // SEL: 0, 1, ..., 1
    subscribe("1:22:1", "3:2");     // 0 or _?
    subscribe("1:22:2", "1:17");    // 0 or xy?

    // Determinant
    subscribe("2:1:0", "1:6");      // num * _
    subscribe("2:1:1", "1:10");     // num * xx
    subscribe("2:2:0", "1:7");     // x * _
    subscribe("2:2:1", "1:7");     // x * x
    subscribe("2:3:0", "2:1");      // num * xx - ____
    subscribe("2:3:1", "2:2");      // det = num * xx - x * x
    subscribe("2:12:0", "2:3");     // det > ____?
    subscribe("2:12:1", "3:5");     // det > 1e-10?
    
    // Intercept
    subscribe("2:4:0", "1:10");     // xx * _
    subscribe("2:4:1", "1:8");     // xx * y
    subscribe("2:5:0", "1:12");     // xy * _
    subscribe("2:5:1", "1:7");     // xy * x
    subscribe("2:6:0", "2:4");      // xx * y - ____
    subscribe("2:6:1", "2:5");      // xx * y - xy * x
    subscribe("2:7:0", "2:6");      // (xx * y - xy * x) / ____
    subscribe("2:7:1", "2:3");      // intercept = (xx * y - xy * x) / det
    subscribe("5:1:0", "2:12");     // result SEL: det > 1e-10?
    subscribe("5:1:1", "3:2");      // result = 0 or ____?
    subscribe("5:1:2", "2:7");      // result = 0 or intercept?

    // Slope
    subscribe("2:8:0", "1:12");     // xy * _
    subscribe("2:8:1", "1:6");     // xy * num
    subscribe("2:9:0", "1:7");     // x * _
    subscribe("2:9:1", "1:8");     // x * y
    subscribe("2:10:0", "2:8");     // xy * num - ____
    subscribe("2:10:1", "2:9");     // xy * num - x * y
    subscribe("2:11:0", "2:10");    // (xy * num - x * y) / ____
    subscribe("2:11:1", "2:3");     // slope = (xy * num - x * y) / det
    subscribe("5:2:0", "2:12");     // result SEL: det > 1e-10?
    subscribe("5:2:1", "3:2");      // result = 0 or ____?
    subscribe("5:2:2", "2:11");     // result = 0 or slope?

    // std::cout << graphviz_representation();
    // return;

    for (int i = 1; i <= 5; i++) {
        setup(i);
    }

    // Initialization

    int iters = 1;

    std::cout << "Initializing constants" << std::endl;

    // Const (3:1) = 1
    for (int i = 1; i <= iters; i++) {
        operand op(1.0, i);
        woof_put("laminar-3.output.1", "", &op);
    }

    // Const (3:2) = 0
    for (int i = 1; i <= iters; i++) {
        operand op(0.0, i);
        woof_put("laminar-3.output.2", "", &op);
    }

    // Const (3:3) = exp(-dt/T)  [decay_rate]
    double dt = 1e-2;
    double T = 5e-2;
    double decay_rate = exp(-dt / T);
    for (int i = 1; i <= iters; i++) {
        operand op(decay_rate, i);
        woof_put("laminar-3.output.3", "", &op);
    }

    // Const (3:4) = 0, 1, 1, ..., 1
    for (int i = 1; i <= iters; i++) {
        int val = (i == 1 ? 0 : 1);
        operand op(val, i);
        woof_put("laminar-3.output.4", "", &op);
    }

    // Const (3:5) = 1e-10
    for (int i = 1; i <= iters; i++) {
        operand op(1e-10, i);
        woof_put("laminar-3.output.5", "", &op);
    }

    // Seed offset nodes with initial value
    for (int i = 13; i <= 17; i++) {
        operand op(0, 1);
        woof_put("laminar-1.output." + std::to_string(i), "output_handler", &op);
    }

    while (woof_last_seq("laminar-1.output.6") < 1) {
        sleep(1);
    }

    // Run program

    std::random_device rd;
    std::default_random_engine eng(rd());
    std::uniform_real_distribution<double> distr(-0.1, 0.1);

    std::vector<operand> x_values;
    std::vector<operand> y_values;

    std::cout << "Writing x and y values" << std::endl;

    for (int i = 0; i < iters; i++) {
        double x = i + distr(eng);
        double y = 3 + 2 * i + distr(eng);
        x_values.push_back(operand(x, i + 1));
        y_values.push_back(operand(y, i + 1));
    }

    for (int i = 1; i <= iters; i++) {
        woof_put("laminar-4.output.1", "output_handler", &x_values[i - 1]);
        woof_put("laminar-4.output.2", "output_handler", &y_values[i - 1]);
    }

    std::cout << "Waiting for program to finish" << std::endl;

    while (woof_last_seq("laminar-5.output.1") < iters) {
        sleep(1);
    }

    // std::vector<double> intercepts;
    // std::vector<double> slopes;

    // for (int i = 1; i <= iters; i++) {
    //     operand op1;
    //     woof_get("laminar-5.output.1", &op1, i);
    //     intercepts.push_back(op1.value);

    //     operand op2;
    //     woof_get("laminar-5.output.2", &op2, i);
    //     slopes.push_back(op2.value);
    // }

    // for (int i = 0; i < iters; i++) {
    //     std::cout << "y = " << slopes[i] << "x + " << intercepts[i] << std::endl;
    // }
}

void add_benchmark_1() {
    add_node(1, 1, ADD);
    add_operand(1, 2);

    subscribe("1:1:0", "1:2");
    subscribe("1:1:1", "1:2");

    std::vector<operand> values;

    for (int i = 1; i <= 100; i++) {
        values.push_back(operand(1, i));
    }

    for (int i = 1; i <= 100; i++) {
        woof_put("laminar-1.output.2", "output_handler", &values[i]);
    }
}

int main() {
    // simple_test();
    // simple_test_2();
    // quadratic_test(2, 5, 2);
    // quadratic_graphviz_test();
    // namespace_graphviz_test();
    // selector_test();
    // filter_test();
    // namespace_test();
    // sqrt_loop_test();

    // std::vector<std::vector<double>> a = {
    //     {1, 2},
    //     {3, 4}
    // };

    // std::vector<std::vector<double>> b = {
    //     {5, 6},
    //     {7, 8}
    // };

    // std::vector<std::vector<double>> a = {
    //     {1, 2, 3},
    //     {4, 5, 6}
    // };

    // std::vector<std::vector<double>> b = {
    //     {10, 11},
    //     {20, 21},
    //     {30, 31}
    // };
    // mat_test(a, b);

    // knn_test();
    // online_linreg_test();
    online_linreg_multinode();

    // add_benchmark_1();
}