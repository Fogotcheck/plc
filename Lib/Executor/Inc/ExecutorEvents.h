#ifndef __ExecutorEvents_h__
#define __ExecutorEvents_h__

#include "ExecutorTypes.h"

extern ExecutorHandle_t ExeHandlers[];

enum ExecutorEvents {
	EXE_INIT = (EventBits_t)(1 << 0),
	EXE_REQUEST_DATA = (EventBits_t)(1 << 1),
	EXE_HANDLE_DATA = (EventBits_t)(1 << 2),

	EXE_ALL_EVENTS = (EventBits_t)((EXE_INIT) | (EXE_REQUEST_DATA) |
				       (EXE_HANDLE_DATA))
};

#endif //__ExecutorEvents_h__
