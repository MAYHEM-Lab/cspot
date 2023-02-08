#include "../dfinterface.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

int main() {
    char* wf = "test";
    init(wf, 100000);

    srand(time(NULL));
    int random_number = rand();

    int run_count = 15;
    for (int i = 0; i < run_count; ++i) {
        add_operand(wf, "1", 0, i);
        add_operand(wf, "1", 1, i);
        add_operand(wf, "1", 2, i);
        add_operand(wf, "1", 3, i);
        add_operand(wf, "1", 4, i);
        add_operand(wf, "1", 5, i);
        add_operand(wf, "1", 6, i);
        add_operand(wf, "1", 7, i);
        add_operand(wf, "1", 8, i);
        add_operand(wf, "1", 9, 1000 + random_number);

        add_node(wf, "1", 10, ADD, "0", 0);

        reset_nodes();
    }

    /*
    sleep(10);
    for (int i = 0; i < run_count; ++i) {
        int value = random_number + 1000 + 9 * i;
        printf("SEARCHING FOR: %d\n", value);
        while (found_result(wf, value) == 0) {
        }
    }
    printf("ALL RESULTS WERE FOUND");
     */
}