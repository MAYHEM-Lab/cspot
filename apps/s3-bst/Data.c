#include <stdio.h>

#include "Data.h"

void DATA_display(DATA data){
    
    fprintf(stdout, "Data Item: %s\n", DI_get_str(data.di));
    fprintf(stdout, "DATA created at: %lu\n", data.version_stamp);
    fprintf(stdout, "DATA link WooF: %s\n", data.lw_name);
    fprintf(stdout, "DATA parent WooF: %s\n", data.pw_name);
    fflush(stdout);

}
