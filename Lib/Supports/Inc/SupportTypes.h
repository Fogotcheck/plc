#ifndef __SupportTypes_h__
#define __SupportTypes_h__

#include <stdlib.h>
#include <string.h>

#include "SupportDef.h"
#include "main.h"

#include "FreeRTOS.h"
#include "event_groups.h"

typedef struct GpioType {
	GPIO_TypeDef *Port;
	uint16_t Pin;
} GpioType_t;

typedef int(InterfaceInit_t)(void *, uint32_t *);
typedef int(InterfaceDeInit_t)(void *);
typedef int(InterfaceSetDefault_t)(void *, uint32_t *);
typedef void(InterfaceCB_t)(EventGroupHandle_t, uint32_t);
typedef int(InterfaceParamInterpret_t)(uint16_t, uint32_t *, char *, char *);

typedef struct InterfaceCallBack {
	EventBits_t BitRawData;
	EventGroupHandle_t EventHandle;
	InterfaceCB_t *Func;
} InterfaceCallBack_t;

typedef struct SupportInterface {
	char Name[SUPPORT_INTERFACE_NAME_SIZE];
	void *Handle;
	GpioType_t Gpio;
	InterfaceInit_t *Init;
	InterfaceDeInit_t *DeInit;
	InterfaceSetDefault_t *SetDefault;
	InterfaceCallBack_t CallBack;
	InterfaceParamInterpret_t *ParamInterpret;
} SupportInterface_t;

typedef int(DriverInit_t)(void *, uint32_t *);
typedef int(DriverSetDefault_t)(void *, uint32_t *);
typedef int(DriverRequest_t)(void *, uint32_t *, uint32_t *);
typedef int(DriverParamInterpret_t)(uint32_t *, char *, char *);
typedef void(DriverRawToCplt_t)(uint32_t *, uint32_t *, uint32_t *);
typedef int(DriverInterpretData_t)(uint16_t, uint32_t *, char *, char *);

typedef struct SupportDrivers {
	char Name[SUPPORT_DRIVER_NAME_SIZE];
	DriverInit_t *Init;
	DriverSetDefault_t *SetDefault;
	DriverRequest_t *Request;
	DriverParamInterpret_t *ParamInterpret;
	DriverRawToCplt_t *RawDataHandle;
	DriverInterpretData_t *DataInterpret
} SupportDrivers_t;

#endif //__SupportTypes_h__
