#include "Support.h"

SupportInterface_t Interfaces[SUPPORT_INTERFACES_COUNT] = { 0 };

int SupportModuleInit(void)
{
	uint8_t count = 0;
	uint8_t Type = 0;
	for (count = 0, Type = SPI_TYPE_1; count < SPI_COUNT; count++, Type++) {
		if (SpiGetHandle(&Interfaces[count], Type)) {
			return -1;
		}
	}

	return 0;
}
