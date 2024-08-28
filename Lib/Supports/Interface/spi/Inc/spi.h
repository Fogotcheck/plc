#ifndef __spi_h__
#define __spi_h__

#include "main.h"
#include "SupportTypes.h"

#define SPI_SUFFIX_NAME "_spi"

enum SpiType {
	SPI_TYPE_1,
	SPI_TYPE_2,

	SPI_COUNT
};

enum SPI_PARAM_DEF {
	SPI_INSTANCE,
	SPI_MODE,
	SPI_DIRECTION,
	SPI_DATASIZE,
	SPI_CLKPOLARITY,
	SPI_CLKPHASE,
	SPI_NSS,
	SPI_BAUDRATEPRESCALER,
	SPI_FIRSTBIT,
	SPI_TIMODE,
	SPI_CRCCALCULATION,
	SPI_CRCPOLYNOMIAL,
};

int SpiGetHandle(SupportInterface_t *Item, uint8_t Type);

#endif //__spi_h__
