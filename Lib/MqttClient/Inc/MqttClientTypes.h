#ifndef __MqttClientTypes_h__
#define __MqttClientTypes_h__

#include "MqttClientDef.h"
#include "lwip/apps/mqtt_opts.h"

typedef struct MqttClientReport {
	uint8_t Type;
	char TopicName[(MQTT_OUTPUT_RINGBUF_SIZE >> 1) - 1];
	char TopicData[(MQTT_OUTPUT_RINGBUF_SIZE >> 1) - 1];
} MqttClientReport_t;

#endif //__MqttClientTypes_h__
