#ifndef __MqttClient_h__
#define __MqttClient_h__

#include <string.h>

#include "FreeRTOS.h"
#include "main.h"
#include "task.h"
#include "timers.h"
#include "queue.h"

#include "lwip.h"
#include "lwip/apps/mqtt.h"

#include "Dlog.h"
#include "MqttClientTypes.h"
#include "MainEvent.h"

#include "ConfTypes.h"

int MqttClientInit(void);
void MqttClientStart(void);
void MqttClientStop(void);
int MqttClientReportRequest(MqttClientReport_t *Report);

#endif //__MqttClient_h__
