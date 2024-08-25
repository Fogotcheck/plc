#ifndef __ConfTypes_h__
#define __ConfTypes_h__

#include "ConDef.h"
#include "MqttClient.h"
#include "lwjson/lwjson.h"

typedef struct ConfigBuf {
	uint16_t size;
	uint8_t flag;
	char data[MQTT_OUTPUT_RINGBUF_SIZE];
} ConfigBuf_t;

typedef struct ConfInterface {
	char Name[SUPPORT_INTERFACE_NAME_SIZE];
	uint32_t Param[SUPPORT_INTERFACE_PARAM_SIZE];
} ConfInterface_t;

typedef struct ConfDriver {
	char Name[SUPPORT_DRIVER_NAME_SIZE];
	uint32_t Param[SUPPORT_DRIVER_PARAM_SIZE];
} ConfDriver_t;

typedef struct ConfPostHandle {
	/* data */
} ConfPostHandle_t;

typedef struct ConfChExecute {
	uint8_t ch;
	ConfInterface_t Interface;
	ConfDriver_t Driver;
	ConfPostHandle_t PostHandle;
} ConfChExecute_t;

int ConfRequest(ConfigBuf_t *Buf);
int ExeConfigure(ConfChExecute_t *Conf);

#endif //__ConfTypes_h__
