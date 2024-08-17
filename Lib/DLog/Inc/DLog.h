#ifndef __DLog_h__
#define __DLog_h__

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"

#if defined(DLOG_USE_RTOS)
#include "FreeRTOS.h"
#include "semphr.h"
#endif

#define DLOG_MAX_MSG_LEN 256
#define DLOG_MAX_FIFO_LEN 20
#define DLOG_MAX_BUF_LEN 2048

typedef struct DLogMSG {
	uint8_t *Mem;
	uint16_t Size;
} DLogMSG_t;

typedef struct DLogMSGFifo {
	uint8_t state;
	DLogMSG_t MSG;
	struct DLogMSGFifo *next;
} DLogMSGList_t;

enum DLogFifoState {
	DLOG_FIFO_EMPTY,
	DLOG_FIFO_BUSY,
};

enum DLogLevelState {
	DLOG_LEVEL_UART_PORT = (uint8_t)(1 << 0),
	DLOG_LEVEL_ERRORS = (uint8_t)(1 << 1),
	DLOG_LEVEL_WARNING = (uint8_t)(1 << 2),
	DLOG_LEVEL_DEBUG = (uint8_t)(1 << 3),
};

#ifndef LOG_LEVEL
#define DLOG_LEVEL_DEFAULT                                      \
	(uint8_t)(DLOG_LEVEL_UART_PORT) | (DLOG_LEVEL_ERRORS) | \
		(DLOG_LEVEL_WARNING) | (DLOG_LEVEL_DEBUG)
#define LOG_LEVEL DLOG_LEVEL_DEFAULT
#endif

#define ErrMessage(fmt, ...)                                            \
	DLogMSGHandler(DLOG_LEVEL_ERRORS, "Error\t%s::%d\t" fmt "\r\n", \
		       __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define WarningMessage(fmt, ...)                                           \
	DLogMSGHandler(DLOG_LEVEL_WARNING, "Warning\t%s::%d\t" fmt "\r\n", \
		       __FUNCTION__, __LINE__, ##__VA_ARGS__)

#define DebugMessage(fmt, ...)                                         \
	DLogMSGHandler(DLOG_LEVEL_DEBUG, "Debug\t%s::%d\t" fmt "\r\n", \
		       __FUNCTION__, __LINE__, ##__VA_ARGS__)

int DLogInit(void);

#endif //__DLog_h__
