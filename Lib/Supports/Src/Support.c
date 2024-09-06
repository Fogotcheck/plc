#include "Support.h"

SupportInterface_t Interfaces[SUPPORT_INTERFACES_COUNT] = { 0 };
SupportDrivers_t Drivers[SUPPORT_DRIVERS_COUNT] = { 0 };

int SupportModuleInit(void)
{
	uint8_t count = 0;
	uint8_t Type = 0;
	for (count = SUP_SPI1, Type = SPI_TYPE_1; Type < SPI_COUNT;
	     count++, Type++) {
		if (SpiGetHandle(&Interfaces[count], Type)) {
			return -1;
		}
	}

	// for (count = SUP_WIRE1_1, Type = WIRE1_TYPE_1; Type < WIRE1_COUNT;
	//      count++, Type++) {
	// 	if (Wire1GetHandle(&Interfaces[count], Type)) {
	// 		return -1;
	// 	}
	// }

	if (Lis3dhGetHandle(&Drivers[SUP_LIS3DH])) {
		return -1;
	}
	// if (Ds1820GetHandle(&Drivers[SUP_DS1820])) {
	// 	return -1;
	// }

	if (Adxl345GetHandle(&Drivers[SUP_ADXL345])) {
		return -1;
	}

	return 0;
}

int SupportGetInterface(char *Name, SupportInterface_t **ItemInterface)
{
	for (size_t i = 0; i < sizeof(Interfaces) / sizeof(Interfaces[0]);
	     i++) {
		if (strcmp(Interfaces[i].Name, Name) == 0) {
			*ItemInterface = &Interfaces[i];
			return 0;
		}
	}
	return -1;
}

int SupportGetDriver(char *Name, SupportDrivers_t **ItemDriver)
{
	for (size_t i = 0; i < sizeof(Drivers) / sizeof(Drivers[0]); i++) {
		if (strcmp(Drivers[i].Name, Name) == 0) {
			*ItemDriver = &Drivers[i];
			return 0;
		}
	}

	return -1;
}
