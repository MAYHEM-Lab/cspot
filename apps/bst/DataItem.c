#include <stdlib.h>
#include "DataItem.h"

char *DI_get_str(DI di){

    char *ret;

    ret = (char *)malloc(2 * sizeof(char));
    ret[0] = di.val;
    ret[1] = '\0';

    return ret;

}
