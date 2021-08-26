#include "hw.h"
#include "woofc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ARGS "W:N:"
char* Usage = "hw -W woof_name\n";

char Wname[4096];

int main(int argc, char** argv) {
    int c;
    HW_EL el;
    unsigned long ndx;
    unsigned long cnt;

    while ((c = getopt(argc, argv, ARGS)) != EOF) {
        switch (c) {
        case 'W':
            strncpy(Wname, optarg, sizeof(Wname));
            break;
        case 'N':
            cnt = strtoul(optarg, NULL, 10);
            break;
        default:
            fprintf(stderr, "unrecognized command %c\n", (char)c);
            fprintf(stderr, "%s", Usage);
            exit(1);
        }
    }

    if (Wname[0] == 0) {
        fprintf(stderr, "must specify woof name\n");
        fprintf(stderr, "%s", Usage);
        fflush(stderr);
        exit(1);
    }

    unsigned long i;
    for (i = 0; i < cnt; ++i) {
        ndx = WooFGetLatestSeqno(Wname);
        if (WooFInvalid(ndx)) {
            fprintf(stderr, "WooFGetLatestSeqno failed\n");
            fflush(stderr);
            exit(1);
        }
        if (WooFGet(Wname, &el, ndx) < 0) {
            fprintf(stderr, "WooFGet(%lu) failed\n", ndx);
            fflush(stderr);
            exit(1);
        }
        printf("%lu: %s\n", i, el.string);
        fflush(stdout);
    }

    return (0);
}
