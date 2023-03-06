#include "hw.h"
#include "woofc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ARGS "W:C:N:L"
char* Usage = "hw_put_stress -W woof_name\n\
\t-N number of messages\n\
\t-L use local woof\n";

char Wname[4096];

int main(int argc, char** argv) {
    int c;
    HW_EL el = {0};
    char msg[4096] = {0};
    unsigned long ndx;
    unsigned long cnt;
    int local = 0;

    while ((c = getopt(argc, argv, ARGS)) != EOF) {
        switch (c) {
        case 'W':
            strncpy(Wname, optarg, sizeof(Wname));
            break;
        case 'C':
            strncpy(msg, optarg, sizeof(msg));
            break;
        case 'N':
            cnt = strtoul(optarg, NULL, 10);
            break;
	case 'L':
	    local = 1;
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

    if(local == 1) {
		WooFInit();
    }

    unsigned long i;
    for (i = 0; i < cnt; ++i) {
        sprintf(el.string, "%s: %lu", msg, i);
        ndx = WooFPut(Wname, "hw_stress", (void*)&el);
        if (WooFInvalid(ndx)) {
            fprintf(stderr, "first WooFPut failed for %s\n", Wname);
            fflush(stderr);
            exit(1);
        }
    }

    return (0);
}
