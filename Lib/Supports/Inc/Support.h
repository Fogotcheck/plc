#ifndef __Support_h__
#define __Support_h__

#include "main.h"
#include "SupportTypes.h"

#include "spi.h"

enum SupportInterfaceEnum {
	SUP_SPI1,
	SUP_SPI2,
	SUP_I2C1,
	SUP_I2C2,

	SUPPORT_INTERFACES_COUNT
};

int SupportModuleInit(void);

#endif //__Support_h__
