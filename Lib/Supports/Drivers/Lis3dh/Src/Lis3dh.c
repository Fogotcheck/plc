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
static inline void Lis3dhRawToCplt(uint32_t *param, uint32_t *raw,
				   uint32_t *cplt);
static inline int Lis3dhIterpretData(uint16_t type, uint32_t *cplt, char *name,
				     char *data);

int Lis3dhGetHandle(SupportDrivers_t *Item)
{
	memset(Item->Name, 0, sizeof(Item->Name));
	strcat(Item->Name, LIS3DH_SUFFIX_NAME);
	Item->Init = Lis3dhInit;
	Item->SetDefault = Lis3dhSetDefaultParam;
	Item->Request = Lis3dhRequest;
	Item->ParamInterpret = Lis3dhParamInterpret;
	Item->RawDataHandle = Lis3dhRawToCplt;
	Item->DataInterpret = Lis3dhIterpretData;
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

	uint8_t TXbuf = LIS3DH_READ | LIS3DH_ADD_WHO_AM_I;
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

	DriverParamType_t hdr = { 0 };
	for (uint16_t i = 0; i < SUPPORT_DRIVER_PARAM_SIZE; i++) {
		DriverParamType_t *tmp = (DriverParamType_t *)&Param[i];
		hdr = *tmp;
		if (tmp->type == INIT_PARAM) {
			HAL_GPIO_WritePin(Interface->Gpio.Port,
					  Interface->Gpio.Pin, 0);
			hdr.addr |= LIS3DH_WRITE;
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
		(INIT_PARAM << OFS_TYPE) | (LIS3DH_ADD_CTRL_REG5 << OFS_ADDR) |
		(0b10000000 << OFS_DATA); // reboot mem
	Param[LIS3DH_DEF_SYSTEM_INIT_1] =
		(INIT_PARAM << OFS_TYPE) |
		(LIS3DH_ADD_TEMP_CFG_REG << OFS_ADDR) |
		(0b11000000 << OFS_DATA); // en Temperature sensor
	Param[LIS3DH_DEF_SYSTEM_INIT_2] =
		(INIT_PARAM << OFS_TYPE) | (LIS3DH_ADD_CTRL_REG1 << OFS_ADDR) |
		(0b10010111
		 << OFS_DATA); // Turn normal mode and 1.344kHz data rate on
	Param[LIS3DH_DEF_SYSTEM_INIT_3] =
		(INIT_PARAM << OFS_TYPE) | (LIS3DH_ADD_CTRL_REG4 << OFS_ADDR) |
		(0b10000000
		 << OFS_DATA); // Turn block data update on (for temperature sensing)

	Param[LIS3DH_DEF_REQUEST_0] =
		(REQUEST_PARAM << OFS_TYPE) |
		(LIS3DH_ADD_STATUS_REG_AUX << OFS_ADDR); // request status

	Param[LIS3DH_DEF_REQUEST_1] =
		(REQUEST_PARAM << OFS_TYPE) |
		(LIS3DH_ADD_OUT_X_H << OFS_ADDR); // request adc1 (xh)
	Param[LIS3DH_DEF_REQUEST_2] =
		(REQUEST_PARAM << OFS_TYPE) |
		(LIS3DH_ADD_OUT_X_L << OFS_ADDR); // request adc1 (xl)

	Param[LIS3DH_DEF_REQUEST_3] =
		(REQUEST_PARAM << OFS_TYPE) |
		(LIS3DH_ADD_OUT_Y_H << OFS_ADDR); // request adc1 (yh)
	Param[LIS3DH_DEF_REQUEST_4] =
		(REQUEST_PARAM << OFS_TYPE) |
		(LIS3DH_ADD_OUT_Y_L << OFS_ADDR); // request adc1 (yl)

	Param[LIS3DH_DEF_REQUEST_5] =
		(REQUEST_PARAM << OFS_TYPE) |
		(LIS3DH_ADD_OUT_Z_H << OFS_ADDR); // request adc1 (zh)
	Param[LIS3DH_DEF_REQUEST_6] =
		(REQUEST_PARAM << OFS_TYPE) |
		(LIS3DH_ADD_OUT_Z_L << OFS_ADDR); // request adc1 (zl)
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

	DriverParamType_t hdr = { 0 };
	for (uint16_t i = 0; i < SUPPORT_DRIVER_PARAM_SIZE; i++) {
		DriverParamType_t *tmp = (DriverParamType_t *)&Param[i];
		hdr = *tmp;
		hdr.addr |= LIS3DH_READ;
		if (tmp->type == REQUEST_PARAM) {
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
	DriverParamType_t *tmp = (DriverParamType_t *)param;
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

static inline void Lis3dhRawToCplt(uint32_t *param, uint32_t *raw,
				   uint32_t *cplt)
{
	cplt[LIS3DH_COUNT] = LIS3DH_OUT_ADC_3 + 1;
	for (uint16_t i = 0; i < SUPPORT_DRIVER_PARAM_SIZE; i++) {
		DriverParamType_t *tmp = (DriverParamType_t *)&param[i];

		switch (tmp->addr) {
		case LIS3DH_ADD_OUT_ADC1_L:
			cplt[LIS3DH_OUT_ADC_1] &= ~(0xff << 0);
			cplt[LIS3DH_OUT_ADC_1] |= (raw[i] << 0);
			break;
		case LIS3DH_ADD_OUT_ADC1_H:
			cplt[LIS3DH_OUT_ADC_1] &= ~(0xff << 8);
			cplt[LIS3DH_OUT_ADC_1] |= (raw[i] << 8);
			break;
		case LIS3DH_ADD_OUT_ADC2_L:
			cplt[LIS3DH_OUT_ADC_2] &= ~(0xff << 0);
			cplt[LIS3DH_OUT_ADC_2] |= (raw[i] << 0);
			break;
		case LIS3DH_ADD_OUT_ADC2_H:
			cplt[LIS3DH_OUT_ADC_2] &= ~(0xff << 8);
			cplt[LIS3DH_OUT_ADC_2] |= (raw[i] << 8);
			break;
		case LIS3DH_ADD_OUT_ADC3_L:
			cplt[LIS3DH_OUT_ADC_3] &= ~(0xff << 0);
			cplt[LIS3DH_OUT_ADC_3] |= (raw[i] << 0);
			break;
		case LIS3DH_ADD_OUT_ADC3_H:
			cplt[LIS3DH_OUT_ADC_3] &= ~(0xff << 8);
			cplt[LIS3DH_OUT_ADC_3] |= (raw[i] << 8);
			break;
		case LIS3DH_ADD_OUT_X_L:
			cplt[LIS3DH_OUT_X] &= ~(0xff << 0);
			cplt[LIS3DH_OUT_X] |= (raw[i] << 0);
			break;
		case LIS3DH_ADD_OUT_X_H:
			cplt[LIS3DH_OUT_X] &= ~(0xff << 8);
			cplt[LIS3DH_OUT_X] |= (raw[i] << 8);
			break;
		case LIS3DH_ADD_OUT_Y_L:
			cplt[LIS3DH_OUT_Y] &= ~(0xff << 0);
			cplt[LIS3DH_OUT_Y] |= (raw[i] << 0);
			break;
		case LIS3DH_ADD_OUT_Y_H:
			cplt[LIS3DH_OUT_Y] &= ~(0xff << 8);
			cplt[LIS3DH_OUT_Y] |= (raw[i] << 8);
			break;
		case LIS3DH_ADD_OUT_Z_L:
			cplt[LIS3DH_OUT_Z] &= ~(0xff << 0);
			cplt[LIS3DH_OUT_Z] |= (raw[i] << 0);
			break;
		case LIS3DH_ADD_OUT_Z_H:
			cplt[LIS3DH_OUT_Z] &= ~(0xff << 8);
			cplt[LIS3DH_OUT_Z] |= (raw[i] << 8);
			break;
		}
	}
}

static inline int Lis3dhIterpretData(uint16_t type, uint32_t *cplt, char *name,
				     char *data)
{
	switch (type) {
	case LIS3DH_OUT_X:
		strcat(name, "OUT_X");
		break;
	case LIS3DH_OUT_Y:
		strcat(name, "OUT_Y");
		break;
	case LIS3DH_OUT_Z:
		strcat(name, "OUT_Z");
		break;
	case LIS3DH_OUT_ADC_1:
		strcat(name, "ADC_1");
		break;
	case LIS3DH_OUT_ADC_2:
		strcat(name, "ADC_2");
		break;
	case LIS3DH_OUT_ADC_3:
		strcat(name, "ADC_3");
		break;

	default:
		return -1;
	}
	itoa(*cplt, data, 10);
	return 0;
}
