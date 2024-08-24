#ifndef __Conf_h__
#define __Conf_h__

#include <string.h>
#include <stdlib.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "MqttClient.h"
#include "lwjson/lwjson.h"

#define CONF_THR_NAME "ConfTask"
#define CONF_THR_STACK 512
#define CONF_THR_PRIOR (osPriorityHigh7)

#define CONF_QUEUE_SIZE 100

#define CONF_TOPIC_SUFFIX "Configurator"
#define CONF_DATA_HELLO "Configurator is wait"

enum ConfSwitchState {
	CONF_DIS,
	CONF_EN,
};

int ConfInit(void);
void ConfSwitch(uint8_t State);

#endif //__Conf_h__
