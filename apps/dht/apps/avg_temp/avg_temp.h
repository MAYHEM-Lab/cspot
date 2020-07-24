#ifndef AVG_TEMP_H

#define ROOM_TEMP_TOPIC "room_temp"
#define AVG_TEMP_TOPIC "avg_temp"
#define AVG_TEMP_HANDLER "avg_temp"
#define AVG_TEMP_HISTORY_LENGTH 256

typedef struct temp_el {
    double value;
} TEMP_EL;

#endif