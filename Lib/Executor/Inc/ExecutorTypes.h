#ifndef __ExecutorTypes_h__
#define __ExecutorTypes_h__

#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#include "queue.h"
#include "timers.h"

#include "ConfTypes.h"
#include "Support.h"

typedef struct ExecutorHandle {
	TaskHandle_t Thr;
	EventGroupHandle_t Event;
	QueueHandle_t Queue;
	TimerHandle_t Timer;
} ExecutorHandle_t;

typedef struct ExecutorBufs {
	uint32_t tmp[SUPPORT_DRIVER_PARAM_SIZE];
	uint32_t raw[SUPPORT_DRIVER_PARAM_SIZE];
	uint32_t cplt[SUPPORT_DRIVER_PARAM_SIZE];
} ExecutorBufs_t;

typedef struct ExecutorTypes {
	uint32_t ID;
	ExecutorHandle_t *Handle;
	ConfChExecute_t Conf;
	SupportInterface_t *Interface;
	SupportDrivers_t *Driver;
	ExecutorBufs_t *Buf;
} ExecutorTypes_t;

#endif //__ExecutorTypes_h__
