#ifndef __MqttClientTypes_h__
#define __MqttClientTypes_h__

#include "MqttClientDef.h"
#include "lwip/apps/mqtt_opts.h"

typedef struct MqttClientReport {
	char TopicName[MQTT_VAR_HEADER_BUFFER_LEN - 1];
	char TopicData[MQTT_OUTPUT_RINGBUF_SIZE - 1];
} MqttClientReport_t;

#endif //__MqttClientTypes_h__
