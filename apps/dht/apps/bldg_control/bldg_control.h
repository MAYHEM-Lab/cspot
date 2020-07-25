#ifndef BLDG_CONTROL_H

#define TOPIC_BUILDING_GATE_TRAFFIC "gate"
#define TOPIC_ROOM_101_TRAFFIC "room_101"
#define TOPIC_ROOM_102_TRAFFIC "room_102"
#define TOPIC_ROOM_103_TRAFFIC "room_103"
#define TOPIC_ROOM_201_TRAFFIC "room_201"
#define TOPIC_ROOM_202_TRAFFIC "room_202"
#define TOPIC_BUILDING_TOTAL "building_total"
#define TOPIC_1F_TOTAL "1f_total"
#define TOPIC_2F_TOTAL "2f_total"
#define TOPIC_HOURLY_CHANGE "hourly_change"

#define BLDG_HISTORY_LENGTH 256
#define TRAFFIC_INGRESS 0
#define TRAFFIC_EGRESS 1

typedef struct traffic_el {
    int type;
} TRAFFIC_EL;

typedef struct count_el {
    int count;
} COUNT_EL;

#endif