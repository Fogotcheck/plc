#include "Lis3dh.h"

static inline int Lis3dhInit(void *Handle, uint32_t *Param);
static inline int Lis3dhSpiInit(SupportInterface_t *Interface, uint32_t *Param);
static inline int Lis3dhI2cInit(SupportInterface_t *Interface, uint32_t *Param);

int Lis3dhGetHandle(SupportDrivers_t *Item)
{
	memset(Item->Name, 0, sizeof(Item->Name));
	strcat(Item->Name, LIS3DH_SUFFIX_NAME);
	Item->Init = Lis3dhInit;
	return 0;
}

static inline int Lis3dhInit(void *Handle, uint32_t *Param)
{
	SupportInterface_t *Interface = (SupportInterface_t *)Handle;
	if (strstr(Interface->Name, "spi") != NULL) {
		if (Lis3dhSpiInit(Interface, Param)) {
			return -1;
		}
		return 0;
	}
	if (strstr(Interface->Name, "i2c") != NULL) {
		if (Lis3dhI2cInit(Interface, Param)) {
			return -1;
		}
		return 0;
	}

	return -1;
}

static inline int Lis3dhSpiInit(SupportInterface_t *Interface,
				__attribute__((unused)) uint32_t *Param)
{
	SPI_HandleTypeDef *hspi = (SPI_HandleTypeDef *)Interface->Handle;
	if (hspi == NULL) {
		return -1;
	}
	if (Interface->Gpio.Port == NULL) {
		return -1;
	}

	HAL_StatusTypeDef ret = 0;
	HAL_GPIO_WritePin(Interface->Gpio.Port, Interface->Gpio.Pin, 1);
	HAL_Delay(10);

	uint8_t TXbuf = READ | LIS3DH_ADD_WHO_AM_I;
	uint8_t RXbuf = 0;

	HAL_GPIO_WritePin(Interface->Gpio.Port, Interface->Gpio.Pin, 0);
	ret = HAL_SPI_Transmit(hspi, &TXbuf, 1, 10);
	if (ret) {
		return -1;
	}
	ret = HAL_SPI_Receive(hspi, &RXbuf, 1, 10);
	if (ret) {
		return -1;
	}
	HAL_GPIO_WritePin(Interface->Gpio.Port, Interface->Gpio.Pin, 1);

	if (RXbuf != LIS3DH_VAL_WHO_AM_I) {
		return -1;
	}

	return 0;
}

static inline int Lis3dhI2cInit(__attribute__((unused))
				SupportInterface_t *Interface,
				__attribute__((unused)) uint32_t *Param)
{
	/* todo */
	return 1;
}
