#include "Adxl345.h"

static inline int Adxl345Init(void *Handle, uint32_t *Param);
static inline int Adxl345SpiInit(SupportInterface_t *Interface,
				 uint32_t *Param);
static inline int Adxl345I2cInit(SupportInterface_t *Interface,
				 uint32_t *Param);

static inline int Adxl345SetDefaultParam(void *Handle, uint32_t *Param);
static inline void Adxl345SpiSetDefault(uint32_t *Param);
static inline void Adxl345I2cSetDefault(uint32_t *Param);
static inline int Adxl345Request(void *Handle, uint32_t *Param, uint32_t *Buf);
static inline int Adxl345SpiRequest(SupportInterface_t *Interface,
				    uint32_t *Param, uint32_t *Buf);
static inline int Adxl345I2cRequest(SupportInterface_t *Interface,
				    uint32_t *Param, uint32_t *Buf);
static inline int Adxl345ParamInterpret(uint32_t *param, char *name,
					char *data);
static inline void Adxl345RawToCplt(uint32_t *param, uint32_t *raw,
				    uint32_t *cplt);
static inline int Adxl345IterpretData(uint16_t type, uint32_t *cplt, char *name,
				      char *data);

void tmpTest(void);
int Adxl345GetHandle(SupportDrivers_t *Item)
{
	memset(Item->Name, 0, sizeof(Item->Name));
	strcat(Item->Name, ADXL345_SUFFIX_NAME);
	Item->Init = Adxl345Init;
	Item->SetDefault = Adxl345SetDefaultParam;
	Item->Request = Adxl345Request;
	Item->ParamInterpret = Adxl345ParamInterpret;
	Item->RawDataHandle = Adxl345RawToCplt;
	Item->DataInterpret = Adxl345IterpretData;

	return 0;
}

#include "DLog.h"

void tmpTest(void)
{
	MX_SPI2_Init();
	HAL_StatusTypeDef ret = 0;
	HAL_GPIO_WritePin(SPI2_CS_GPIO_Port, SPI2_CS_Pin, 1);
	HAL_Delay(10);
	extern SPI_HandleTypeDef hspi2;
	uint8_t TXbuf = ADXL345_READ | ADXL345_DEVID;
	uint8_t RXbuf = 0;
	HAL_GPIO_WritePin(SPI2_CS_GPIO_Port, SPI2_CS_Pin, 0);
	ret = HAL_SPI_Transmit(&hspi2, &TXbuf, 1, 10);
	if (ret) {
		ErrMessage();
	}

	ret = HAL_SPI_Receive(&hspi2, &RXbuf, 1, 10);
	if (ret) {
		ErrMessage();
	}

	HAL_GPIO_WritePin(SPI2_CS_GPIO_Port, SPI2_CS_Pin, 1);
	HAL_Delay(10);
	if (RXbuf != ADXL345_VAL_DEVID) {
		ErrMessage("RX::0x%2hhx", RXbuf);
	}
	uint8_t tmpConf[] = { ADXL345_POWER_CTL, 0x08 };

	HAL_GPIO_WritePin(SPI2_CS_GPIO_Port, SPI2_CS_Pin, 0);
	ret = HAL_SPI_Transmit(&hspi2, tmpConf, sizeof(tmpConf), 10);
	if (ret) {
		ErrMessage();
	}
	HAL_GPIO_WritePin(SPI2_CS_GPIO_Port, SPI2_CS_Pin, 1);
	HAL_Delay(10);
	TXbuf = ADXL345_DATAX0 | ADXL345_READ;
	while (1) {
		HAL_GPIO_WritePin(SPI2_CS_GPIO_Port, SPI2_CS_Pin, 0);
		ret = HAL_SPI_Transmit(&hspi2, &TXbuf, 1, 10);
		if (ret) {
			ErrMessage();
		}

		ret = HAL_SPI_Receive(&hspi2, &RXbuf, 1, 10);
		if (ret) {
			ErrMessage();
		}
		HAL_GPIO_WritePin(SPI2_CS_GPIO_Port, SPI2_CS_Pin, 1);
		DebugMessage("Rx::0x%2hhx\tTX::0x%2hhx", RXbuf, TXbuf);
		HAL_Delay(1000);
	}
}

static inline int Adxl345Init(void *Handle, uint32_t *Param)
{
	SupportInterface_t *Interface = (SupportInterface_t *)Handle;
	if (strstr(Interface->Name, "spi") != NULL) {
		if (Adxl345SpiInit(Interface, Param)) {
			return -1;
		}
		return 0;
	}
	if (strstr(Interface->Name, "i2c") != NULL) {
		if (Adxl345I2cInit(Interface, Param)) {
			return -1;
		}
		return -1;
	}

	return -1;
}

static inline int Adxl345SpiInit(SupportInterface_t *Interface, uint32_t *Param)
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

	uint8_t TXbuf = ADXL345_READ | ADXL345_DEVID;
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

	if (RXbuf != ADXL345_VAL_DEVID) {
		return -1;
	}

	DriverParamType_t hdr = { 0 };
	for (uint16_t i = 0; i < SUPPORT_DRIVER_PARAM_SIZE; i++) {
		DriverParamType_t *tmp = (DriverParamType_t *)&Param[i];
		hdr = *tmp;
		if (tmp->type == INIT_PARAM) {
			HAL_GPIO_WritePin(Interface->Gpio.Port,
					  Interface->Gpio.Pin, 0);
			hdr.addr |= ADXL345_WRITE;
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

static inline int Adxl345I2cInit(__attribute__((unused))
				 SupportInterface_t *Interface,
				 __attribute__((unused)) uint32_t *Param)
{
	/* todo:  */
	return -1;
}

static inline int Adxl345SetDefaultParam(void *Handle, uint32_t *Param)
{
	SupportInterface_t *Interface = (SupportInterface_t *)Handle;
	if (strstr(Interface->Name, "spi") != NULL) {
		Adxl345SpiSetDefault(Param);
		return 0;
	}
	if (strstr(Interface->Name, "i2c") != NULL) {
		Adxl345I2cSetDefault(Param);
		return 1;
	}

	return -1;
}

static inline void Adxl345SpiSetDefault(uint32_t *Param)
{
	Param[ADXL345_DEF_SYSTEM_INIT_0] = (INIT_PARAM << OFS_TYPE) |
					   (ADXL345_POWER_CTL << OFS_ADDR) |
					   (0x08 << OFS_DATA); // Measure Bit
	Param[ADXL345_DEF_SYSTEM_INIT_1] =
		(INIT_PARAM << OFS_TYPE) | (ADXL345_FIFO_CTL << OFS_ADDR) |
		(0x0 << OFS_DATA); // FIFO is bypassed.

	Param[ADXL345_DEF_REQUEST_0] =
		(REQUEST_PARAM << OFS_TYPE) |
		(ADXL345_DATAX0 << OFS_ADDR); // request x l
	Param[ADXL345_DEF_REQUEST_1] =
		(REQUEST_PARAM << OFS_TYPE) |
		(ADXL345_DATAX1 << OFS_ADDR); // request x h
	Param[ADXL345_DEF_REQUEST_2] =
		(REQUEST_PARAM << OFS_TYPE) |
		(ADXL345_DATAY0 << OFS_ADDR); // request y l
	Param[ADXL345_DEF_REQUEST_3] =
		(REQUEST_PARAM << OFS_TYPE) |
		(ADXL345_DATAY1 << OFS_ADDR); // request y h
	Param[ADXL345_DEF_REQUEST_4] =
		(REQUEST_PARAM << OFS_TYPE) |
		(ADXL345_DATAZ0 << OFS_ADDR); // request z l
	Param[ADXL345_DEF_REQUEST_5] =
		(REQUEST_PARAM << OFS_TYPE) |
		(ADXL345_DATAZ1 << OFS_ADDR); // request z h

	return;
}

static inline void Adxl345I2cSetDefault(__attribute__((unused)) uint32_t *Param)
{
	/* todo */
	return;
}

static inline int Adxl345Request(void *Handle, uint32_t *Param, uint32_t *Buf)
{
	SupportInterface_t *Interface = (SupportInterface_t *)Handle;
	if (strstr(Interface->Name, "spi") != NULL) {
		if (Adxl345SpiRequest(Interface, Param, Buf)) {
			return -1;
		}
		return 0;
	}
	if (strstr(Interface->Name, "i2c") != NULL) {
		if (Adxl345I2cRequest(Interface, Param, Buf)) {
			return -1;
		}

		return 0;
	}
	return -1;
}

static inline int Adxl345SpiRequest(SupportInterface_t *Interface,
				    uint32_t *Param, uint32_t *Buf)
{
	SPI_HandleTypeDef *hspi = (SPI_HandleTypeDef *)Interface->Handle;
	HAL_StatusTypeDef ret = 0;
	DriverParamType_t hdr = { 0 };
	for (uint16_t i = 0; i < SUPPORT_DRIVER_PARAM_SIZE; i++) {
		DriverParamType_t *tmp = (DriverParamType_t *)&Param[i];
		hdr = *tmp;
		hdr.addr |= ADXL345_READ;
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

static inline int Adxl345I2cRequest(__attribute__((unused))
				    SupportInterface_t *Interface,
				    __attribute__((unused)) uint32_t *Param,
				    __attribute__((unused)) uint32_t *Buf)
{
	return -1;
}

static inline int Adxl345ParamInterpret(uint32_t *param, char *name, char *data)
{
	DriverParamType_t *tmp = (DriverParamType_t *)param;
	switch (tmp->addr) {
	case ADXL345_Reserved:
		strcat(name, "Warning Reserved 0x");
		break;
	case ADXL345_THRESH_TAP:
		strcat(name, "THRESH_TAP 0x");
		break;
	case ADXL345_OFSX:
		strcat(name, "OFSX 0x");
		break;
	case ADXL345_OFSY:
		strcat(name, "OFSY 0x");
		break;
	case ADXL345_OFSZ:
		strcat(name, "OFSZ 0x");
		break;
	case ADXL345_DUR:
		strcat(name, "DUR 0x");
		break;
	case ADXL345_Latent:
		strcat(name, "Latent 0x");
		break;
	case ADXL345_Window:
		strcat(name, "Window 0x");
		break;
	case ADXL345_THRESH_ACT:
		strcat(name, "THRESH_ACT 0x");
		break;
	case ADXL345_THRESH_INACT:
		strcat(name, "THRESH_INACT 0x");
		break;
	case ADXL345_TIME_INACT:
		strcat(name, "TIME_INACT 0x");
		break;
	case ADXL345_ACT_INACT_CTL:
		strcat(name, "ACT_INACT_CTL 0x");
		break;
	case ADXL345_THRESH_FF:
		strcat(name, "THRESH_FF 0x");
		break;
	case ADXL345_TIME_FF:
		strcat(name, "TIME_FF 0x");
		break;
	case ADXL345_TAP_AXES:
		strcat(name, "TAP_AXES 0x");
		break;
	case ADXL345_ACT_TAP_STATUS:
		strcat(name, "ACT_TAP_STATUS 0x");
		break;
	case ADXL345_BW_RATE:
		strcat(name, "BW_RATE 0x");
		break;
	case ADXL345_POWER_CTL:
		strcat(name, "POWER_CTL 0x");
		break;
	case ADXL345_INT_ENABLE:
		strcat(name, "INT_ENABLE 0x");
		break;
	case ADXL345_INT_MAP:
		strcat(name, "INT_MAP 0x");
		break;
	case ADXL345_INT_SOURCE:
		strcat(name, "INT_SOURCE 0x");
		break;
	case ADXL345_DATA_FORMAT:
		strcat(name, "DATA_FORMAT 0x");
		break;
	case ADXL345_FIFO_CTL:
		strcat(name, "FIFO_CTL 0x");
		break;

	default:
		return -1;
	}
	itoa(*param, data, 16);
	return 0;
}

static inline void Adxl345RawToCplt(uint32_t *param, uint32_t *raw,
				    uint32_t *cplt)
{
	cplt[ADXL345_COUNT] = ADXL345_OUT_Z + 1;
	for (uint16_t i = 0; i < SUPPORT_DRIVER_PARAM_SIZE; i++) {
		DriverParamType_t *tmp = (DriverParamType_t *)&param[i];

		switch (tmp->addr) {
		case ADXL345_DATAX0:
			cplt[ADXL345_OUT_X] &= ~(0xff << 0);
			cplt[ADXL345_OUT_X] |= (raw[i] << 0);
			break;
		case ADXL345_DATAX1:
			cplt[ADXL345_OUT_X] &= ~(0xff << 8);
			cplt[ADXL345_OUT_X] |= (raw[i] << 8);
			break;
		case ADXL345_DATAY0:
			cplt[ADXL345_OUT_Y] &= ~(0xff << 0);
			cplt[ADXL345_OUT_Y] |= (raw[i] << 0);
			break;
		case ADXL345_DATAY1:
			cplt[ADXL345_OUT_Y] &= ~(0xff << 8);
			cplt[ADXL345_OUT_Y] |= (raw[i] << 8);
			break;
		case ADXL345_DATAZ0:
			cplt[ADXL345_OUT_Z] &= ~(0xff << 0);
			cplt[ADXL345_OUT_Z] |= (raw[i] << 0);
			break;
		case ADXL345_DATAZ1:
			cplt[ADXL345_OUT_Z] &= ~(0xff << 8);
			cplt[ADXL345_OUT_Z] |= (raw[i] << 8);
			break;
		}
	}
}

static inline int Adxl345IterpretData(uint16_t type, uint32_t *cplt, char *name,
				      char *data)
{
	switch (type) {
	case ADXL345_OUT_X:
		strcat(name, "OUT_X");
		break;
	case ADXL345_OUT_Y:
		strcat(name, "OUT_Y");
		break;
	case ADXL345_OUT_Z:
		strcat(name, "OUT_Z");
		break;
	default:
		return -1;
	}
	itoa(*cplt, data, 10);
	return 0;
}
