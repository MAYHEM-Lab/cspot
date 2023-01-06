#include "../dfinterface.h"

#include <unistd.h>

/*
 *   0, 1, 2           0, 1, 2, 3
 *   node 2 (ADD)     node 3  (ADD)
 *       |        |
 *         0, 1
 *      node 1 (MUL)
 */
int main() {
    char* wf = "test";
    init(wf, 10000);

    // z = x * y (= 90)
    add_node(wf, 1, MUL, 0, 0, 2);
    add_node(wf, 2, ADD, 1, 0, 3);
    add_node(wf, 3, ADD, 1, 1, 4);

    sleep(2);
    // x = 1 + 2 + 3 (= 6)
    add_operand(wf, 1.0, 2, 0);
    // sleep(2);
    add_operand(wf, 2.0, 2, 1);
    // sleep(2);
    add_operand(wf, 3.0, 2, 2);
    // sleep(2);
    // 6.0

    // y = 4 + 5 + 6 + 0 (= 15)
    add_operand(wf, 4.0, 3, 0);
    // sleep(2);
    add_operand(wf, 5.0, 3, 1);
    // sleep(2);
    add_operand(wf, 6.0, 3, 2);
    // sleep(2);
    add_operand(wf, 0.0, 3, 3);
    // 15.0

    // result : 90.0
}