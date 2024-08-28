#include "ds1820.h"

static inline int Ds1820Init(void *Handle, uint32_t *Param);

int Ds1820GetHandle(SupportDrivers_t *Item)
{
	memset(Item->Name, 0, sizeof(Item->Name));
	strcat(Item->Name, DS1820_SUFFIX_NAME);
	Item->Init = Ds1820Init;
	return 0;
}

static inline int Ds1820Init(void *Handle,
			     __attribute__((unused)) uint32_t *Param)
{
	SupportInterface_t *Interface = (SupportInterface_t *)Handle;
	if (strstr(Interface->Name, "_wire_one") == NULL) {
		return -1;
	}
	UART_HandleTypeDef *huart = (UART_HandleTypeDef *)Interface->Handle;
	if (huart == NULL) {
		return -1;
	}

	if (HAL_HalfDuplex_EnableTransmitter(huart) != HAL_OK) {
		return -1;
	}
	uint8_t TXbuf = 0xf0;
	uint8_t RXbuf = 0;
	HAL_UART_Transmit(huart, &TXbuf, 1, 10);
	HAL_Delay(1);
	if (HAL_HalfDuplex_EnableReceiver(huart) != HAL_OK) {
		return -1;
	}
	HAL_UART_Receive(huart, &RXbuf, 1, 10);
	if (RXbuf != 0xf0) {
		return -1;
	}

	return 0;
}
