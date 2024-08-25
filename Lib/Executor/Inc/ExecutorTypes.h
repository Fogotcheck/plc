#ifndef __ExecutorTypes_h__
#define __ExecutorTypes_h__

#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#include "queue.h"
#include "ConfTypes.h"
#include "Support.h"

typedef struct ExecutorHandle {
	TaskHandle_t Thr;
	EventGroupHandle_t Event;
	QueueHandle_t Queue;
} ExecutorHandle_t;

typedef struct ExecutorTypes {
	uint32_t ID;
	ExecutorHandle_t *Handle;
	ConfChExecute_t Conf;
	SupportInterface_t *Interface;
} ExecutorTypes_t;

#endif //__ExecutorTypes_h__
