#ifndef AVG_TEMP_H

#define ROOM_TEMP_TOPIC "room_temperature"
#define AVG_TEMP_TOPIC "average_temperature"
#define AVG_TEMP_HANDLER "h_avg_temp"
#define AVG_TEMP_HISTORY_LENGTH 256

typedef struct temp_el {
    double value;
} TEMP_EL;

#endif