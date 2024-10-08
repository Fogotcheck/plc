#ifndef __Support_h__
#define __Support_h__

#include "main.h"
#include "SupportTypes.h"

#include "spi.h"
// #include "wire1.h" ебанутая затея

#include "Lis3dh.h"
// #include "ds1820.h" не работает
#include "Adxl345.h"

enum SupportInterfaceEnum {
	SUP_SPI1,
	SUP_SPI2,
	SUP_WIRE1_1,
	SUP_WIRE1_2,
	SUP_I2C1,
	SUP_I2C2,

	SUPPORT_INTERFACES_COUNT
};

enum SupportDriversEnum {
	SUP_LIS3DH,
	SUP_DS1820,
	SUP_ADXL345,

	SUPPORT_DRIVERS_COUNT
};

int SupportModuleInit(void);
int SupportGetInterface(char *Name, SupportInterface_t **ItemInterface);
int SupportGetDriver(char *Name, SupportDrivers_t **ItemDriver);

#endif //__Support_h__
