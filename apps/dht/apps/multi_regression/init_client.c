#include "multi_regression.h"
#include "woofc-host.h"
#include "woofc.h"

int main(int argc, char** argv) {
    WooFInit();
    if (WooFCreate(CLIENT_WOOF_NAME, sizeof(PUBLISH_ARG), CLIENT_WOOF_LENGTH) < 0) {
        fprintf(stderr, "failed to create %s\n", CLIENT_WOOF_NAME);
        exit(1);
    }

    return 0;
}
