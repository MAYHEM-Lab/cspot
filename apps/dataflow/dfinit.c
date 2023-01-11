#include "df.h"
#include "woofc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
 * create the woofs needed for a dataflow program
 *
 * woofname.dfprogram: contains program nodes
 * woofname.dfoperand: contains program operands
 */

#define ARGS "W:s:"
char* Usage = "dfinit -W woofname\n\
\t-s logsize for each woof\n";

int main(int argc, char** argv) {
    int err;
    int c;
    char woof[4096];
    char op_woof[4096];
    char prog_woof[4096];
    int size;

    memset(woof, 0, sizeof(prog_woof));
    size = 0;
    while ((c = getopt(argc, argv, ARGS)) != EOF) {
        switch (c) {
        case 'W':
            strncpy(woof, optarg, sizeof(woof));
            strncpy(prog_woof, woof, sizeof(prog_woof));
            strncpy(op_woof, woof, sizeof(op_woof));
            strncat(prog_woof, ".dfprogram", sizeof(prog_woof));
            strncat(op_woof, ".dfoperand", sizeof(op_woof));
            break;
        case 's':
            size = atoi(optarg);
            break;
        default:
            fprintf(stderr, "ERROR -- dfinit: unrecognized command %c\n", (char)c);
            fprintf(stderr, "%s", Usage);
            exit(1);
        }
    }

    if (size == 0) {
        fprintf(stderr, "ERROR -- dfinit: must size\n");
        fprintf(stderr, "%s", Usage);
        exit(1);
    }

    if (prog_woof[0] == 0) {
        fprintf(stderr, "ERROR -- dfinit: must specify woof name\n");
        fprintf(stderr, "%s", Usage);
        exit(1);
    }

    WooFInit(); /* attach local namespace for create*/

    err = WooFCreate(prog_woof, sizeof(DF_NODE), size);
    if (err < 0) {
        fprintf(stderr, "ERROR -- create of %s failed\n", prog_woof);
        exit(1);
    }

    err = WooFCreate(op_woof, sizeof(DF_OPERAND), size);
    if (err < 0) {
        fprintf(stderr, "ERROR -- create of %s failed\n", prog_woof);
        exit(1);
    }

    exit(0);
}
