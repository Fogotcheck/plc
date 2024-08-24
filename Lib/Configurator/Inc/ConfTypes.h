#ifndef __ConfTypes_h__
#define __ConfTypes_h__

#include "ConDef.h"
#include "MqttClient.h"
#include "lwjson/lwjson.h"

typedef struct ConfigBuf {
	uint16_t size;
	uint8_t flag;
	char data[MQTT_OUTPUT_RINGBUF_SIZE];
} ConfigBuf_t;

int ConfRequest(ConfigBuf_t *Buf);

#endif //__ConfTypes_h__
