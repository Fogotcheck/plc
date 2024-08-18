#ifndef __MqttClientDef_h__
#define __MqttClientDef_h__

#define MQTT_CLIENT_THR_NAME "MqttTask"
#define MQTT_CLIENT_THR_STACK 512
#define MQTT_CLIENT_THR_PRIOR (osPriorityRealtime7 - 4)

#define MQTT_CLIENT_REPORT_QUEUE_COUNT 100

#define MQTT_CLENT_TIMER_PERIOD_MS 1000
#define MQTT_CLIENT_TIMER_DELAYED_START 500

#endif //__MqttClientDef_h__
