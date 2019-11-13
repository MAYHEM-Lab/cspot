#include <stdlib.h>
#include "DataItem.h"

char *DI_get_str(DI di){

    char *retval;

    retval = (char *)malloc(S3_KEY_SIZE * sizeof(char));
    strncpy(retval, di.key, S3_KEY_SIZE);

    return retval;

}
