#ifndef __ExecutorTypes_h__
#define __ExecutorTypes_h__

#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#include "queue.h"
#include "ConfTypes.h"

typedef struct ExecutorHandle {
	TaskHandle_t Thr;
	EventGroupHandle_t Event;
	QueueHandle_t Queue;
} ExecutorHandle_t;

typedef struct ExecutorTypes {
	uint32_t ID;
	ExecutorHandle_t *Handle;
	ConfChExecute_t Conf;
} ExecutorTypes_t;

extern ExecutorHandle_t ExeHandlers[];

#endif //__ExecutorTypes_h__
