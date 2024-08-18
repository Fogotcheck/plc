#ifndef __spi_h__
#define __spi_h__

#include <stdlib.h>
#include <string.h>

#include "main.h"
#include "SupportTypes.h"

#define SPI_SUFFIX_NAME "_spi"

enum SpiType {
	SPI_TYPE_1,
	SPI_TYPE_2,

	SPI_COUNT
};

enum SPI_PARAM_DEF {
	INSTANCE,
	MODE,
	DIRECTION,
	DATASIZE,
	CLKPOLARITY,
	CLKPHASE,
	NSS,
	BAUDRATEPRESCALER,
	FIRSTBIT,
	TIMODE,
	CRCCALCULATION,
	CRCPOLYNOMIAL,
};

int SpiGetHandle(SupportInterface_t *Item, uint8_t Type);

#endif //__spi_h__
