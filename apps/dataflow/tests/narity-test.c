#include "../src/dfinterface.h"
#include "../src/dfoperation.h"

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
    add_node(wf, 1, 2, DF_DOUBLE_ARITH_MULTIPLICATION, 0, 0);
    add_node(wf, 2, 3, DF_DOUBLE_ARITH_ADDITION, 1, 0);
    add_node(wf, 3, 4, DF_DOUBLE_ARITH_ADDITION, 1, 1);

    //sleep(1);
    // x = 1 + 2 + 3 (= 6)
    DF_VALUE value;
    value.type = DF_DOUBLE;
    value.value.df_double = 1.0;
    add_operand(wf, 2, 0, value);
    // sleep(2);
    value.value.df_double = 2.0;
    add_operand(wf, 2, 1, value);
    // sleep(2);
    value.value.df_double = 3.0;
    add_operand(wf, 2, 2, value);
    // sleep(2);
    // 6.0

    // y = 4 + 5 + 6 + 0 (= 15)
    value.value.df_double = 4.0;
    add_operand(wf, 3, 0, value);
    // sleep(2);
    value.value.df_double = 5.0;
    add_operand(wf, 3, 1, value);
    // sleep(2);
    value.value.df_double = 6.0;
    add_operand(wf, 3, 2, value);
    // sleep(2);
    value.value.df_double = 0.0;
    add_operand(wf, 3, 3, value);
    // 15.0

    // result : 90.0
}