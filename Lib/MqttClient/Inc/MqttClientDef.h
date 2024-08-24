#ifndef __MqttClientDef_h__
#define __MqttClientDef_h__

#include "event_groups.h"

#define MQTT_CLIENT_THR_NAME "MqttTask"
#define MQTT_CLIENT_THR_STACK 512
#define MQTT_CLIENT_THR_PRIOR (osPriorityRealtime7 - 4)

#define MQTT_CLIENT_REPORT_QUEUE_COUNT 100

#define MQTT_CLENT_TIMER_PERIOD_MS 1000
#define MQTT_CLIENT_TIMER_DELAYED_START 500

enum MqttClientReportType {
	MQTT_SUB = 1,
	MQTT_PUB = 2,
};

enum MqttClientExent {
	MQTT_OK = (EventBits_t)(1 << 0),
	MQTT_ERR = (EventBits_t)(1 << 1),
	MQTT_ALL = (EventBits_t)(MQTT_OK) | (MQTT_ERR)
};

#endif //__MqttClientDef_h__
