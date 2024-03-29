#include "../dfinterface.h"

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <random>
#include <chrono>
#include <thread>
#include <unistd.h>

void online_linreg_multinode() {
    // Update variables (num, x, y, xx, xy)

    add_node(1, 1, 1, MUL);    // num *= decay_rate
    add_node(1, 1, 2, MUL);    // x *= decay_rate
    add_node(1, 1, 3, MUL);    // y *= decay_rate
    add_node(1, 1, 4, MUL);    // xx *= decay_rate
    add_node(1, 1, 5, MUL);    // xy *= decay_rate
    add_node(1, 1, 6, ADD);    // num += 1
    add_node(1, 1, 7, ADD);    // x += new_x
    add_node(1, 1, 8, ADD);    // y += new_y
    add_node(1, 1, 9, MUL);    // new_x ^ 2
    add_node(1, 1, 10, ADD);   // xx += new_x ^ 2
    add_node(1, 1, 11, MUL);   // new_x * new_y
    add_node(1, 1, 12, ADD);   // xy += new_x * new_y
    add_node(1, 1, 13, OFFSET);// num seq + 1
    add_node(1, 1, 14, OFFSET);// x seq + 1
    add_node(1, 1, 15, OFFSET);// y seq + 1
    add_node(1, 1, 16, OFFSET);// xx seq + 1
    add_node(1, 1, 17, OFFSET);// xy seq + 1
    add_node(1, 1, 18, SEL);   // num or 0?
    add_node(1, 1, 19, SEL);   // x or 0?
    add_node(1, 1, 20, SEL);   // y or 0?
    add_node(1, 1, 21, SEL);   // xx or 0?
    add_node(1, 1, 22, SEL);   // xy or 0?

    // Calculate slope and intercept

    add_node(2, 1, 1, MUL);    // num * xx
    add_node(2, 1, 2, MUL);    // x * x
    add_node(2, 1, 3, SUB);    // det = num * xx - x * x
    add_node(2, 1, 4, MUL);    // xx * y
    add_node(2, 1, 5, MUL);    // xy * x
    add_node(2, 1, 6, SUB);    // xx * y - xy * x
    add_node(2, 1, 7, DIV);    // intercept = (xx * y - xy * x) / det;
    add_node(2, 1, 8, MUL);    // xy * num
    add_node(2, 1, 9, MUL);    // x * y
    add_node(2, 1, 10, SUB);   // xy * num - x * y
    add_node(2, 1, 11, DIV);   // slope = (xy * num - x * y) / det;
    add_node(2, 1, 12, GT);    // det > 1e-10?
    // add_node(2, 13, SEL);   // intercept or 0?
    // add_node(2, 14, SEL);   // slope or 0?

    // Constants

    add_operand(3, 1, 1);      // const 1
    add_operand(3, 1, 2);      // const 0
    add_operand(3, 1, 3);      // decay_factor = exp(-dt / T)
    add_operand(3, 1, 4);      // handle init (0, 1, 1, ..., 1)
    add_operand(3, 1, 5);      // const 1e-10

    // Inputs

    add_operand(4, 1, 1);      // input x
    add_operand(4, 1, 2);      // input y

    // Outputs

    add_node(5, 2, 1, SEL);         // intercept = intercept or 0
    add_node(5, 2, 2, SEL);         // slope = slope or 0

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

    setup();

    sleep(2);
    // Initialization
    int curr_host_id = get_curr_host_id();
    int iters = 100;
    

    if(curr_host_id == 1) {
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

        std::chrono::time_point<std::chrono::system_clock> start, end;
        std::vector<host> hosts;
        hosts.push_back(host(2, "woof://169.231.235.188/home/centos/cspot/build/bin/"));
        hosts.push_back(host(2, "woof://169.231.235.168/home/centos/cspot/build/bin/"));

        for (int i = 1; i <= iters; i++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            std::cout << "Adding inputs " << std::to_string(i) << std::endl;
            woof_put("laminar-4.output.1", "output_handler", &x_values[i - 1]);
            woof_put("laminar-4.output.2", "output_handler", &y_values[i - 1]);

            if(i == 5 || i == 20 || i == 50 || i == 90) {
                woof_put(generate_woof_path(HOSTS_WOOF_TYPE), "", &hosts[0]);         
                start = std::chrono::system_clock::now();
            }
            
            if(i == 7 || i == 23 || i == 54 || i == 95) {
                woof_put(generate_woof_path(HOSTS_WOOF_TYPE), "", &hosts[1]);  
                end = std::chrono::system_clock::now();
                std::chrono::duration<double> elapsed_seconds = end - start;

                std::cout << "Network partition time : "  << elapsed_seconds.count() << std::endl;
            }
        }
    }

    if(curr_host_id == 2) {
        std::vector<double> intercepts;
        std::vector<double> slopes;

        for (int i = 1; i <= iters; i++) {

            while(woof_last_seq("laminar-5.output.1") < i) {
                sleep(1);
            }
            operand op1;
            woof_get("laminar-5.output.1", &op1, i);
            intercepts.push_back(op1.value);
            std::cout << std::to_string(i) << " Intercept " << std::to_string(op1.value) << std::endl;

            while(woof_last_seq("laminar-5.output.2") < i) {
                sleep(1);
            }

            operand op2;
            woof_get("laminar-5.output.2", &op2, i);
            slopes.push_back(op2.value);
            std::cout << std::to_string(i) << " Slope " << std::to_string(op2.value) << std::endl;
        }
    }

    std::cout << "Execution Completed" << std::endl;
}

int main() {
    
    set_host(1);
    add_host(1, "169.231.235.185", "/home/centos/cspot/build/bin/");
    add_host(2, "169.231.235.168", "/home/centos/cspot/build/bin/");

    online_linreg_multinode();
}
