#ifndef __ExecutorTypes_h__
#define __ExecutorTypes_h__

#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#include "queue.h"

typedef struct ExecutorHandle {
	TaskHandle_t Thr;
	EventGroupHandle_t Event;
} ExecutorHandle_t;

typedef struct ExecutorTypes {
	uint32_t ID;
	ExecutorHandle_t *Handle;

} ExecutorTypes_t;

#endif //__ExecutorTypes_h__
