#include "Lis3dh.h"

static inline int Lis3dhInit(void *Handle, uint32_t *Param);
static inline int Lis3dhSpiInit(SupportInterface_t *Interface, uint32_t *Param);
static inline int Lis3dhI2cInit(SupportInterface_t *Interface, uint32_t *Param);
static inline int Lis3dhSetDefaultParam(void *Handle, uint32_t *Param);
static inline void Lis3dhSpiSetDefault(uint32_t *Param);
static inline void Lis3dhI2cSetDefault(uint32_t *Param);
static inline int Lis3dhRequest(void *Handle, uint32_t *Param, uint32_t *Buf);
static inline int Lis3dhSpiRequest(SupportInterface_t *Interface,
				   uint32_t *Param, uint32_t *Buf);
static inline int Lis3dhI2cRequest(SupportInterface_t *Interface,
				   uint32_t *Param, uint32_t *Buf);
static inline int Lis3dhParamInterpret(uint32_t *param, char *name, char *data);

int Lis3dhGetHandle(SupportDrivers_t *Item)
{
	memset(Item->Name, 0, sizeof(Item->Name));
	strcat(Item->Name, LIS3DH_SUFFIX_NAME);
	Item->Init = Lis3dhInit;
	Item->SetDefault = Lis3dhSetDefaultParam;
	Item->Request = Lis3dhRequest;
	Item->ParamInterpret = Lis3dhParamInterpret;
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

static inline int Lis3dhSpiInit(SupportInterface_t *Interface, uint32_t *Param)
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

	Lsi3dhParamType_t hdr = { 0 };
	for (uint16_t i = 0; i < SUPPORT_DRIVER_PARAM_SIZE; i++) {
		Lsi3dhParamType_t *tmp = (Lsi3dhParamType_t *)&Param[i];
		hdr = *tmp;
		if (tmp->type == LIS3DH_INIT_PARAM) {
			HAL_GPIO_WritePin(Interface->Gpio.Port,
					  Interface->Gpio.Pin, 0);
			hdr.addr |= WRITE;
			ret = HAL_SPI_Transmit(hspi, &hdr.addr, 1, 10);
			if (ret) {
				return -1;
			}
			ret = HAL_SPI_Transmit(hspi, (uint8_t *)&hdr.data, 1,
					       10);
			if (ret) {
				return -1;
			}
			HAL_GPIO_WritePin(Interface->Gpio.Port,
					  Interface->Gpio.Pin, 1);
			HAL_Delay(10);
		}
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

static inline int Lis3dhSetDefaultParam(void *Handle, uint32_t *Param)
{
	SupportInterface_t *Interface = (SupportInterface_t *)Handle;
	if (strstr(Interface->Name, "spi") != NULL) {
		Lis3dhSpiSetDefault(Param);
		return 0;
	}
	if (strstr(Interface->Name, "i2c") != NULL) {
		Lis3dhI2cSetDefault(Param);
		return 1;
	}

	return -1;
}

static inline void Lis3dhSpiSetDefault(uint32_t *Param)
{
	Param[LIS3DH_DEF_SYSTEM_INIT_0] =
		(LIS3DH_INIT_PARAM << LIS3DH_TYPE) |
		(LIS3DH_ADD_CTRL_REG5 << LIS3DH_ADDR) |
		(0b10000000 << LIS3DH_DATA); // reboot mem
	Param[LIS3DH_DEF_SYSTEM_INIT_1] =
		(LIS3DH_INIT_PARAM << LIS3DH_TYPE) |
		(LIS3DH_ADD_TEMP_CFG_REG << LIS3DH_ADDR) |
		(0b11000000 << LIS3DH_DATA); // en Temperature sensor
	Param[LIS3DH_DEF_SYSTEM_INIT_2] =
		(LIS3DH_INIT_PARAM << LIS3DH_TYPE) |
		(LIS3DH_ADD_CTRL_REG1 << LIS3DH_ADDR) |
		(0b10010111
		 << LIS3DH_DATA); // Turn normal mode and 1.344kHz data rate on
	Param[LIS3DH_DEF_SYSTEM_INIT_3] =
		(LIS3DH_INIT_PARAM << LIS3DH_TYPE) |
		(LIS3DH_ADD_CTRL_REG4 << LIS3DH_ADDR) |
		(0b10000000
		 << LIS3DH_DATA); // Turn block data update on (for temperature sensing)

	Param[LIS3DH_DEF_REQUEST_0] =
		(LIS3DH_REQUEST_PARAM << LIS3DH_TYPE) |
		(LIS3DH_ADD_STATUS_REG_AUX << LIS3DH_ADDR); // request status

	Param[LIS3DH_DEF_REQUEST_1] =
		(LIS3DH_REQUEST_PARAM << LIS3DH_TYPE) |
		(LIS3DH_ADD_OUT_X_H << LIS3DH_ADDR); // request adc1 (xh)
	Param[LIS3DH_DEF_REQUEST_2] =
		(LIS3DH_REQUEST_PARAM << LIS3DH_TYPE) |
		(LIS3DH_ADD_OUT_X_L << LIS3DH_ADDR); // request adc1 (xl)

	Param[LIS3DH_DEF_REQUEST_3] =
		(LIS3DH_REQUEST_PARAM << LIS3DH_TYPE) |
		(LIS3DH_ADD_OUT_Y_H << LIS3DH_ADDR); // request adc1 (yh)
	Param[LIS3DH_DEF_REQUEST_4] =
		(LIS3DH_REQUEST_PARAM << LIS3DH_TYPE) |
		(LIS3DH_ADD_OUT_Y_L << LIS3DH_ADDR); // request adc1 (yl)

	Param[LIS3DH_DEF_REQUEST_5] =
		(LIS3DH_REQUEST_PARAM << LIS3DH_TYPE) |
		(LIS3DH_ADD_OUT_Z_H << LIS3DH_ADDR); // request adc1 (zh)
	Param[LIS3DH_DEF_REQUEST_6] =
		(LIS3DH_REQUEST_PARAM << LIS3DH_TYPE) |
		(LIS3DH_ADD_OUT_Z_L << LIS3DH_ADDR); // request adc1 (zl)
}

static inline void Lis3dhI2cSetDefault(__attribute__((unused)) uint32_t *Param)
{
	/* todo */
}

static inline int Lis3dhRequest(void *Handle, uint32_t *Param, uint32_t *Buf)
{
	SupportInterface_t *Interface = (SupportInterface_t *)Handle;
	if (strstr(Interface->Name, "spi") != NULL) {
		if (Lis3dhSpiRequest(Interface, Param, Buf)) {
			return -1;
		}
		return 0;
	}
	if (strstr(Interface->Name, "i2c") != NULL) {
		if (Lis3dhI2cRequest(Interface, Param, Buf)) {
			return -1;
		}

		return 0;
	}
	return -1;
}

static inline int Lis3dhSpiRequest(SupportInterface_t *Interface,
				   uint32_t *Param, uint32_t *Buf)
{
	SPI_HandleTypeDef *hspi = (SPI_HandleTypeDef *)Interface->Handle;
	HAL_StatusTypeDef ret = 0;

	Lsi3dhParamType_t hdr = { 0 };
	for (uint16_t i = 0; i < SUPPORT_DRIVER_PARAM_SIZE; i++) {
		Lsi3dhParamType_t *tmp = (Lsi3dhParamType_t *)&Param[i];
		hdr = *tmp;
		hdr.addr |= READ;
		if (tmp->type == LIS3DH_REQUEST_PARAM) {
			HAL_GPIO_WritePin(Interface->Gpio.Port,
					  Interface->Gpio.Pin, 0);
			ret = HAL_SPI_Transmit(hspi, &hdr.addr, 1, 10);
			if (ret) {
				HAL_GPIO_WritePin(Interface->Gpio.Port,
						  Interface->Gpio.Pin, 1);
				return -1;
			}
			ret = HAL_SPI_Receive(hspi, (uint8_t *)&Buf[i], 1, 10);
			if (ret) {
				HAL_GPIO_WritePin(Interface->Gpio.Port,
						  Interface->Gpio.Pin, 1);
				return -1;
			}
			HAL_GPIO_WritePin(Interface->Gpio.Port,
					  Interface->Gpio.Pin, 1);
		}
	}
	if (Interface->CallBack.Func != NULL) {
		Interface->CallBack.Func(Interface->CallBack.EventHandle,
					 Interface->CallBack.BitRawData);
	}

	return 0;
}

static inline int Lis3dhI2cRequest(__attribute__((unused))
				   SupportInterface_t *Interface,
				   __attribute__((unused)) uint32_t *Param,
				   __attribute__((unused)) uint32_t *Buf)
{
	return -1;
}

static inline int Lis3dhParamInterpret(uint32_t *param, char *name, char *data)
{
	Lsi3dhParamType_t *tmp = (Lsi3dhParamType_t *)param;
	switch (tmp->addr) {
	case LIS3DH_ADD_STATUS_REG_AUX:
		strcat(name, "STATUS_REG_AUX 0x");
		break;
	case LIS3DH_ADD_OUT_ADC1_L:
		strcat(name, "OUT_ADC1_L 0x");
		break;
	case LIS3DH_ADD_OUT_ADC1_H:
		strcat(name, "OUT_ADC1_H 0x");
		break;
	case LIS3DH_ADD_OUT_ADC2_L:
		strcat(name, "OUT_ADC2_L 0x");
		break;
	case LIS3DH_ADD_OUT_ADC2_H:
		strcat(name, "OUT_ADC2_H 0x");
		break;
	case LIS3DH_ADD_OUT_ADC3_L:
		strcat(name, "OUT_ADC3_L 0x");
		break;
	case LIS3DH_ADD_OUT_ADC3_H:
		strcat(name, "OUT_ADC3_H 0x");
		break;
	case LIS3DH_ADD_WHO_AM_I:
		strcat(name, "WHO_AM_I 0x");
		break;
	case LIS3DH_ADD_CTRL_REG0:
		strcat(name, "CTRL_REG0 0x");
		break;
	case LIS3DH_ADD_TEMP_CFG_REG:
		strcat(name, "TEMP_CFG_REG 0x");
		break;
	case LIS3DH_ADD_CTRL_REG1:
		strcat(name, "CTRL_REG1 0x");
		break;
	case LIS3DH_ADD_CTRL_REG2:
		strcat(name, "CTRL_REG2 0x");
		break;
	case LIS3DH_ADD_CTRL_REG3:
		strcat(name, "CTRL_REG3 0x");
		break;
	case LIS3DH_ADD_CTRL_REG4:
		strcat(name, "CTRL_REG4 0x");
		break;
	case LIS3DH_ADD_CTRL_REG5:
		strcat(name, "CTRL_REG5 0x");
		break;
	case LIS3DH_ADD_CTRL_REG6:
		strcat(name, "CTRL_REG6 0x");
		break;
	case LIS3DH_ADD_REFERENCE:
		strcat(name, "Warning REFERENCE 0x");
		break;
	case LIS3DH_ADD_STATUS_REG:
		strcat(name, "STATUS_REG 0x");
		break;
	case LIS3DH_ADD_OUT_X_L:
		strcat(name, "OUT_X_L 0x");
		break;
	case LIS3DH_ADD_OUT_X_H:
		strcat(name, "OUT_X_H 0x");
		break;
	case LIS3DH_ADD_OUT_Y_L:
		strcat(name, "OUT_Y_L 0x");
		break;
	case LIS3DH_ADD_OUT_Y_H:
		strcat(name, "OUT_Y_H 0x");
		break;
	case LIS3DH_ADD_OUT_Z_L:
		strcat(name, "OUT_Z_L 0x");
		break;
	case LIS3DH_ADD_OUT_Z_H:
		strcat(name, "OUT_Z_H 0x");
		break;
	case LIS3DH_ADD_FIFO_CTRL_REG:
		strcat(name, "FIFO_CTRL_REG 0x");
		break;
	case LIS3DH_ADD_FIFO_SRC_REG:
		strcat(name, "FIFO_SRC_REG 0x");
		break;
	case LIS3DH_ADD_CLICK_CFG:
		strcat(name, "CLICK_CFG 0x");
		break;
	case LIS3DH_ADD_CLICK_SRC:
		strcat(name, "CLICK_SRC 0x");
		break;
	case LIS3DH_ADD_CLICK_THS:
		strcat(name, "CLICK_THS 0x");
		break;
	case LIS3DH_ADD_TIME_LIMIT:
		strcat(name, "TIME_LIMIT 0x");
		break;
	case LIS3DH_ADD_TIME_LATENCY:
		strcat(name, "TIME_LATENCY 0x");
		break;
	case LIS3DH_ADD_TIME_WINDOW:
		strcat(name, "TIME_WINDOW 0x");
		break;
	case LIS3DH_ADD_ACT_THS:
		strcat(name, "ACT_THS 0x");
		break;
	case LIS3DH_ADD_ACT_DUR:
		strcat(name, "ACT_DUR 0x");
		break;
	default:
		return -1;
	}
	itoa(*param, data, 16);
	return 0;
}
