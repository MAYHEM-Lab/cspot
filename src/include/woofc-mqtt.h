#ifndef WOOFC_MQTT_H
#define WOOFC_MQTT_H

/*
 * the basics
 */
#define WOOF_MQTT_PUT (1)
#define WOOF_MQTT_GET (2)
#define WOOF_MQTT_GET_EL_SIZE (3)

#define WOOF_MQTT_MAX_SIZE (1024)

#if defined(__cplusplus)
extern "C" {
#endif

#include "textlist.h"

struct woof_mqtt_stc
{
	char woof_name[1024];
	int command;
	char *handler_name;
	unsigned char *element;
};

typedef struct woof_mqtt_stc WMQTT;

#if defined(__cplusplus)
}
#endif

#endif

