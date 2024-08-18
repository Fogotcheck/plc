#ifndef __MainEvent_h__
#define __MainEvent_h__

#include "FreeRTOS.h"
#include "event_groups.h"

extern EventGroupHandle_t MainEvent;

enum MainEventsEnum {
	ETH_LINK_UP = (EventBits_t)(1 << 0),
	ETH_LINK_DOWN = (EventBits_t)(1 << 1),
	MQTT_LINK_UP = (EventBits_t)(1 << 2),
	MQTT_LINK_DOWN = (EventBits_t)(1 << 3),

	MAIN_CRITICAL_ERR = (EventBits_t)(1 << 4),

	MAIN_ALL_EVENTS = (EventBits_t)(ETH_LINK_UP) | (ETH_LINK_DOWN) |
			  (MQTT_LINK_UP) | (MQTT_LINK_DOWN) |
			  (MAIN_CRITICAL_ERR)

};

#endif //__MainEvent_h__
