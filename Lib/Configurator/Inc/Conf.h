#ifndef __Conf_h__
#define __Conf_h__

#include <string.h>
#include <stdlib.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "ConfTypes.h"

int ConfInit(void);
void ConfSwitch(uint8_t State);

#endif //__Conf_h__
