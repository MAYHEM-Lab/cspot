#include "../dfinterface.h"

#include <malloc.h>
#include <math.h>
#include <stdio.h>
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

    // x = 1 + 2 + 3 (= 6)
    add_operand(wf, "2", 0, 1.0);
    add_operand(wf, "2", 1, 2.0);
    add_operand(wf, "2", 2, 3.0);
    // 6.0

    // y = 4 + 5 + 6 + 0 (= 15)
    add_operand(wf, "3", 0, 4.0);
    add_operand(wf, "3", 1, 5.0);
    add_operand(wf, "3", 2, 6.0);
    add_operand(wf, "3", 3, 0.0);
    // 15.0

    add_node(wf, "1", 2, MUL, "0", 0);
    add_node(wf, "2", 3, ADD, "1", 0);
    add_node(wf, "3", 4, ADD, "1", 1);
}