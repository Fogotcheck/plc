#include "spi.h"

extern SPI_HandleTypeDef hspi1;
extern SPI_HandleTypeDef hspi2;

static inline int SpiInit(void *Handle, uint32_t *Param);
static inline int SpiSetDefault(void *Handle, uint32_t *Param);
static inline int SpiDeInit(void *Handle);
static inline int SpiParamInterpret(uint16_t type, uint32_t *param, char *name,
				    char *data);

int SpiGetHandle(SupportInterface_t *Item, uint8_t Type)
{
	char TmpName[SUPPORT_INTERFACE_NAME_SIZE] = { 0 };
	switch (Type) {
	case SPI_TYPE_1: {
		Item->Handle = &hspi1;
		itoa(SPI_TYPE_1, TmpName, 10);
		Item->Gpio.Port = SPI1_CS_GPIO_Port;
		Item->Gpio.Pin = SPI1_CS_Pin;
		break;
	}
	case SPI_TYPE_2: {
		Item->Handle = &hspi2;
		itoa(SPI_TYPE_2, TmpName, 10);
		Item->Gpio.Port = SPI2_CS_GPIO_Port;
		Item->Gpio.Pin = SPI2_CS_Pin;
		break;
	}
	default:
		return -1;
	}
	strcat(TmpName, SPI_SUFFIX_NAME);
	memcpy(Item->Name, TmpName, strlen(TmpName));
	Item->Init = SpiInit;
	Item->SetDefault = SpiSetDefault;
	Item->DeInit = SpiDeInit;
	Item->ParamInterpret = SpiParamInterpret;

	return 0;
}

static inline int SpiInit(void *Handle, uint32_t *Param)
{
	SupportInterface_t *Interface = (SupportInterface_t *)Handle;
	SPI_HandleTypeDef *hspi = (SPI_HandleTypeDef *)Interface->Handle;
	if (hspi == NULL) {
		return -1;
	}

	hspi->Instance = (SPI_TypeDef *)Param[INSTANCE];
	hspi->Init.Mode = Param[MODE];
	hspi->Init.Direction = Param[DIRECTION];
	hspi->Init.DataSize = Param[DATASIZE];
	hspi->Init.CLKPolarity = Param[CLKPOLARITY];
	hspi->Init.CLKPhase = Param[CLKPHASE];
	hspi->Init.NSS = Param[NSS];
	hspi->Init.BaudRatePrescaler = Param[BAUDRATEPRESCALER];
	hspi->Init.FirstBit = Param[FIRSTBIT];
	hspi->Init.TIMode = Param[TIMODE];
	hspi->Init.CRCCalculation = Param[CRCCALCULATION];
	hspi->Init.CRCPolynomial = Param[CRCPOLYNOMIAL];
	if (HAL_SPI_Init(hspi) != HAL_OK) {
		return -1;
	}

	return 0;
}

static inline int SpiSetDefault(void *Handle, uint32_t *Param)
{
	SupportInterface_t *Interface = (SupportInterface_t *)Handle;
	SPI_HandleTypeDef *hspi = (SPI_HandleTypeDef *)Interface->Handle;
	if (hspi == NULL) {
		return -1;
	}

	do {
		if (hspi == &hspi1) {
			Param[INSTANCE] = (uint32_t)SPI1;
			break;
		}
		if (hspi == &hspi2) {
			Param[INSTANCE] = (uint32_t)SPI2;
			break;
		}
		return -1;
	} while (0);

	Param[MODE] = SPI_MODE_MASTER;
	Param[DIRECTION] = SPI_DIRECTION_2LINES;
	Param[DATASIZE] = SPI_DATASIZE_8BIT;
	Param[CLKPOLARITY] = SPI_POLARITY_LOW;
	Param[CLKPHASE] = SPI_PHASE_1EDGE;
	Param[NSS] = SPI_NSS_SOFT;
	Param[BAUDRATEPRESCALER] = SPI_BAUDRATEPRESCALER_8;
	Param[FIRSTBIT] = SPI_FIRSTBIT_MSB;
	Param[TIMODE] = SPI_TIMODE_DISABLE;
	Param[CRCCALCULATION] = SPI_CRCCALCULATION_DISABLE;
	Param[CRCPOLYNOMIAL] = 10;

	return 0;
}

static inline int SpiDeInit(void *Handle)
{
	SupportInterface_t *Interface = (SupportInterface_t *)Handle;
	SPI_HandleTypeDef *hspi = (SPI_HandleTypeDef *)Interface->Handle;
	if (hspi == NULL) {
		return -1;
	}
	if (HAL_SPI_DeInit(hspi)) {
		return -1;
	}
	return 0;
}

static inline int SpiParamInterpret(uint16_t type, uint32_t *param, char *name,
				    char *data)
{
	switch (type) {
	case INSTANCE: {
		strcat(name, "INSTANCE 0x");
		break;
	}
	case MODE: {
		strcat(name, "MODE 0x");
		break;
	}
	case DIRECTION: {
		strcat(name, "DIRECTION 0x");
		break;
	}
	case DATASIZE: {
		strcat(name, "DATASIZE 0x");
		break;
	}
	case CLKPOLARITY: {
		strcat(name, "CLKPOLARITY 0x");
		break;
	}
	case CLKPHASE: {
		strcat(name, "CLKPHASE 0x");
		break;
	}
	case NSS: {
		strcat(name, "NSS 0x");
		break;
	}
	case BAUDRATEPRESCALER: {
		strcat(name, "BAUDRATEPRESCALER 0x");
		break;
	}
	case FIRSTBIT: {
		strcat(name, "FIRSTBIT 0x");
		break;
	}
	case TIMODE: {
		strcat(name, "TIMODE 0x");
		break;
	}
	case CRCCALCULATION: {
		strcat(name, "CRCCALCULATION 0x");
		break;
	}
	case CRCPOLYNOMIAL: {
		strcat(name, "CRCPOLYNOMIAL 0x");
		break;
	}

	default:
		return -1;
	}

	itoa(*param, data, 16);

	return 0;
}
