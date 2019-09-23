#include <stdio.h>

#include "Data.h"

void DATA_display(DATA data){
    
    fprintf(stdout, "Data Item: %s\n", DI_get_str(data.di));
    fprintf(stdout, "DATA created at: %lu\n", data.vs);
    fprintf(stdout, "DATA link WooF: %s\n", data.lw_name);
    fflush(stdout);

}
