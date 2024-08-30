#include "ds1820.h"
#include "DLog.h"
static inline int Ds1820Init(void *Handle, uint32_t *Param);
uint8_t tmpRXbuf[16] = { 0 };
int Ds1820GetHandle(SupportDrivers_t *Item)
{
	memset(Item->Name, 0, sizeof(Item->Name));
	strcat(Item->Name, DS1820_SUFFIX_NAME);
	Item->Init = Ds1820Init;
	extern UART_HandleTypeDef huart5;

	if (HAL_HalfDuplex_EnableTransmitter(&huart5) != HAL_OK) {
		return -1;
	}
	if (HAL_HalfDuplex_EnableReceiver(&huart5) != HAL_OK) {
		ErrMessage();
		return -1;
	}
	if (HAL_HalfDuplex_EnableTransmitter(&huart5) != HAL_OK) {
		ErrMessage();
		return -1;
	}

	const uint8_t convert_T[] = { 0xcc, 0x44 };

	if (HAL_UART_Transmit(&huart5, convert_T, sizeof(convert_T), 100) !=
	    HAL_OK) {
		ErrMessage();
		return -1;
	}
	HAL_Delay(750);

	const uint8_t read_scratch[] = { 0xcc, 0xbe };

	if (HAL_HalfDuplex_EnableTransmitter(&huart5) != HAL_OK) {
		return -1;
	}
	if (HAL_HalfDuplex_EnableReceiver(&huart5) != HAL_OK) {
		ErrMessage();
		return -1;
	}
	if (HAL_HalfDuplex_EnableTransmitter(&huart5) != HAL_OK) {
		ErrMessage();
		return -1;
	}

	if (HAL_UART_Transmit(&huart5, read_scratch, sizeof(read_scratch),
			      100) != HAL_OK) {
		ErrMessage();
		return -1;
	}

	if (HAL_HalfDuplex_EnableReceiver(&huart5) != HAL_OK) {
		ErrMessage();
		return -1;
	}

	HAL_UART_Receive_IT(&huart5, tmpRXbuf, sizeof(tmpRXbuf));

	HAL_Delay(10000);

	return 0;
}

static inline int Ds1820Init(void *Handle,
			     __attribute__((unused)) uint32_t *Param)
{
	SupportInterface_t *Interface = (SupportInterface_t *)Handle;
	if (strstr(Interface->Name, "_wire1") == NULL) {
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

	if (HAL_UART_Transmit(huart, &TXbuf, 1, 100) != HAL_OK) {
		ErrMessage();
		return -1;
	}

	HAL_Delay(1);
	if (HAL_HalfDuplex_EnableReceiver(huart) != HAL_OK) {
		ErrMessage();
		return -1;
	}
	if (HAL_UART_Receive(huart, &RXbuf, 1, 100) != HAL_OK) {
		ErrMessage("RX::%hhu", RXbuf);
		return -1;
	}
	if (RXbuf != 0xf0) {
		ErrMessage();
		return -1;
	}

	return 0;
}
