#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "CheckPointer.h"
#include "Helper.h"

#include "woofc.h"
#include "woofc-host.h"

int CHECK_POINT_RECORD_SIZE;
int CHECK_POINT_HISTORY_SIZE;
char CHECK_POINT_WOOF_NAME[256];
int WOOF_NAME_LENGTH;

void CP_init(int check_point_record_size, char *check_point_woof_name, int WooF_name_length, unsigned long check_point_history_size){

    CHECK_POINT_RECORD_SIZE = check_point_record_size;
    CHECK_POINT_HISTORY_SIZE = check_point_history_size;
    strcpy(CHECK_POINT_WOOF_NAME, check_point_woof_name);
    WOOF_NAME_LENGTH = WooF_name_length;
    WooFCreate(CHECK_POINT_WOOF_NAME, CHECK_POINT_RECORD_SIZE, CHECK_POINT_HISTORY_SIZE);

}

CPRR *CP_reader(CPR cpr){

    int num_of_elements;
    int i;
    int j;
    int k;
    int string_start_idx;
    int seq_no_start_idx;
    unsigned long temp;
    CPRR *cprr;
    char **WooF_names;
    unsigned long *seq_nos;

    cprr = (CPRR *)malloc(sizeof(CPRR));

    num_of_elements = cpr.record[0];
    WooF_names = (char **)malloc(num_of_elements * sizeof(char *));
    for(i = 0; i < num_of_elements; ++i){
        WooF_names[i] = (char *)malloc((WOOF_NAME_LENGTH + 1) * sizeof(char));
        memset(WooF_names[i], 0, (WOOF_NAME_LENGTH + 1));
    }
    seq_nos = (unsigned long *)malloc(num_of_elements * sizeof(unsigned long));

    string_start_idx = 1;
    seq_no_start_idx = string_start_idx + WOOF_NAME_LENGTH;
    for(i = 0; i < num_of_elements; ++i){
        for(j = string_start_idx, k = 0; j < (string_start_idx + WOOF_NAME_LENGTH); ++j, ++k){
            WooF_names[i][k] = cpr.record[j];
        }
        string_start_idx += WOOF_NAME_LENGTH + sizeof(unsigned long);

        temp = 0;
        k = 7;
        for(j = seq_no_start_idx; j < (seq_no_start_idx + sizeof(unsigned long)); ++j, --k){
            temp |= cpr.record[j];
            if(k) temp <<= 8;
        }
        seq_no_start_idx += sizeof(unsigned long) + WOOF_NAME_LENGTH;
        seq_nos[i] = temp;
    }

    cprr->num_of_elements = num_of_elements;
    cprr->WooF_names = WooF_names;
    cprr->seq_nos = seq_nos;

    return cprr;

}

CPRR *CP_read(unsigned long version_stamp){

    CPR cpr;
    CPRR *cprr;
    char *buffer;
    
    buffer = (char *)malloc(CHECK_POINT_RECORD_SIZE * sizeof(char));
    WooFGet(CHECK_POINT_WOOF_NAME, (void *)buffer, version_stamp);
    cpr.record = buffer;
    cprr = CP_reader(cpr);

    return cprr;

}

CPR *CP_writer(char num_of_elements, char **WooF_names, unsigned long *seq_nos){
    CPR *cpr;
    int i;
    int j;
    int k;
    unsigned long mask;
    //CPRR *cprr;

    cpr = (CPR *)malloc(sizeof(CPR));
    cpr->record = (char *)malloc(CHECK_POINT_RECORD_SIZE * sizeof(char));
    k = 0;
    cpr->record[k++] = num_of_elements;
    for(i = 0; i < num_of_elements; ++i){
        //string part
        for(j = 0; j < WOOF_NAME_LENGTH; ++j){
            cpr->record[k++] = WooF_names[i][j];
        }
        //seq_no part
        mask = 0xFF00000000000000;
        for(j = 1; j <= sizeof(unsigned long); ++j){
            cpr->record[k++] = (seq_nos[i] & mask) >> ((sizeof(unsigned long) - j) * 8);
            mask >>= 8;
        }
    }

    return cpr;
    //cprr = CP_reader(cpr);
    //for(i = 0; i < cprr->num_of_elements; ++i){
    //    printf("%s %lu\n", cprr->WooF_names[i], cprr->seq_nos[i]);
    //}

}

void CP_write(char num_of_elements, char **WooF_names, unsigned long *seq_nos){

    CPR *cpr;
    cpr = CP_writer(num_of_elements, WooF_names, seq_nos);
    insertIntoWooF(CHECK_POINT_WOOF_NAME, NULL, (void *)cpr->record);

}
