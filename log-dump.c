#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "log.h"

#define ARGS "f:"
char *Usage = "log-dump -f filename\n";

char log_name[4096];

int main(int argc, char **argv)
{
    unsigned long last_seq_no = 0;
    unsigned long first;
    int c;
    LOG *log;
    LOG *log_tail;
    MIO *lmio;
    EVENT *ev;

    while ((c = getopt(argc, argv, ARGS)) != EOF)
    {
        switch (c)
        {
        case 'f':
            strncpy(log_name, optarg, sizeof(log_name));
            break;
        default:
            fprintf(stderr,
                    "unrecognized command %c\n", (char)c);
            fprintf(stderr, "%s", Usage);
            exit(1);
        }
    }

    if (log_name[0] == 0)
    {
        fprintf(stderr, "must specify log name\n");
        fprintf(stderr, "%s", Usage);
        exit(1);
    }

    lmio = MIOReOpen(log_name);
    if (lmio == NULL)
    {
        fprintf(stderr,
                "couldn't open mio for log %s\n", log_name);
        fflush(stderr);
        exit(1);
    }
    log = (LOG *)MIOAddr(lmio);
    if (log == NULL)
    {
        fprintf(stderr, "couldn't open log as %s\n", log_name);
        fflush(stderr);
        exit(1);
    }

    LogPrint(stdout, log);
    fflush(stdout);

    return (0);
}
