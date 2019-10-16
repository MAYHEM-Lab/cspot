#ifndef CHECK_POINTER_H
#define CHECK_POINTER_H

extern int CHECK_POINT_RECORD_SIZE;
extern char CHECK_POINT_WOOF_NAME[256];
extern int WOOF_NAME_LENGTH;

struct CheckPointRecord{
    char *record;
};

typedef struct CheckPointRecord CPR;

struct CheckPointReaderRecord{

    int num_of_elements;
    char **WooF_names;
    unsigned long *seq_nos;

};

typedef struct CheckPointReaderRecord CPRR;

void CP_init(int check_point_record_size, char *check_point_woof_name, int WooF_name_length, unsigned long check_point_history_size);
CPR *CP_writer(char num_of_elements, char **WooF_names, unsigned long *seq_nos);
CPRR *CP_reader(CPR cpr);
CPRR *CP_read(unsigned long version_stamp);
void CP_write(char num_of_elements, char **WooF_names, unsigned long *seq_nos);

#endif
