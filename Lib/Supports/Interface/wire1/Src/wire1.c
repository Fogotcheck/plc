#include "wire1.h"

extern UART_HandleTypeDef huart4;
extern UART_HandleTypeDef huart5;

static inline int Wire1Init(void *Handle, uint32_t *Param);
static inline int Wire1SetDefault(void *Handle, uint32_t *Param);
static inline int Wire1DeInit(void *Handle);
static inline int Wire1ParamInterpret(uint16_t type, uint32_t *param,
				      char *name, char *data);

int Wire1GetHandle(SupportInterface_t *Item, uint8_t Type)
{
	char TmpName[SUPPORT_INTERFACE_NAME_SIZE] = { 0 };
	switch (Type) {
	case WIRE1_TYPE_1:
		Item->Handle = &huart4;
		itoa(WIRE1_TYPE_1, TmpName, 10);
		break;
	case WIRE1_TYPE_2:
		Item->Handle = &huart5;
		itoa(WIRE1_TYPE_2, TmpName, 10);
		break;
	default:
		return -1;
	}

	strcat(TmpName, WIRE1_SUFFIX_NAME);
	memcpy(Item->Name, TmpName, strlen(TmpName));
	Item->Init = Wire1Init;
	Item->SetDefault = Wire1SetDefault;
	Item->DeInit = Wire1DeInit;
	Item->ParamInterpret = Wire1ParamInterpret;

	return 0;
}

static inline int Wire1Init(void *Handle, uint32_t *Param)
{
	SupportInterface_t *Interface = (SupportInterface_t *)Handle;
	UART_HandleTypeDef *huart = (UART_HandleTypeDef *)Interface->Handle;
	huart->Instance = (USART_TypeDef *)Param[WIRE1_INSTANCE];
	huart->Init.BaudRate = Param[WIRE1_BAUDRATE];
	huart->Init.WordLength = Param[WIRE1_WORDLENGTH];
	huart->Init.StopBits = Param[WIRE1_STOPBITS];
	huart->Init.Parity = Param[WIRE1_PARITY];
	huart->Init.Mode = Param[WIRE1_MODE_TX_RX];
	huart->Init.HwFlowCtl = Param[WIRE1_HWCONTROL];
	huart->Init.OverSampling = Param[WIRE1_OVERSAMPLING];
	if (HAL_HalfDuplex_Init(&huart4) != HAL_OK) {
		return -1;
	}
	return 0;
}

static inline int Wire1SetDefault(void *Handle, uint32_t *Param)
{
	SupportInterface_t *Interface = (SupportInterface_t *)Handle;
	UART_HandleTypeDef *huart = (UART_HandleTypeDef *)Interface->Handle;

	do {
		if (huart == &huart4) {
			Param[WIRE1_INSTANCE] = (uint32_t)UART4;
			break;
		}
		if (huart == &huart5) {
			Param[WIRE1_INSTANCE] = (uint32_t)UART5;
			break;
		}
		return -1;
	} while (0);

	Param[WIRE1_BAUDRATE] = 9600;
	Param[WIRE1_WORDLENGTH] = UART_WORDLENGTH_8B;
	Param[WIRE1_STOPBITS] = UART_STOPBITS_1;
	Param[WIRE1_PARITY] = UART_PARITY_NONE;
	Param[WIRE1_MODE_TX_RX] = UART_MODE_TX_RX;
	Param[WIRE1_HWCONTROL] = UART_HWCONTROL_NONE;
	Param[WIRE1_OVERSAMPLING] = UART_OVERSAMPLING_16;

	return 0;
}

static inline int Wire1DeInit(void *Handle)
{
	SupportInterface_t *Interface = (SupportInterface_t *)Handle;
	UART_HandleTypeDef *huart = (UART_HandleTypeDef *)Interface->Handle;
	if (huart == NULL) {
		return -1;
	}
	if (HAL_UART_DeInit(huart)) {
		return -1;
	}

	return 0;
}

static inline int Wire1ParamInterpret(uint16_t type, uint32_t *param,
				      char *name, char *data)
{
	switch (type) {
	case WIRE1_INSTANCE:
		strcat(name, "INSTANCE 0x");
		break;
	case WIRE1_BAUDRATE:
		strcat(name, "BAUDRATE 0x");
		break;
	case WIRE1_WORDLENGTH:
		strcat(name, "WORDLENGTH 0x");
		break;
	case WIRE1_STOPBITS:
		strcat(name, "STOPBITS 0x");
		break;
	case WIRE1_PARITY:
		strcat(name, "PARITY 0x");
		break;
	case WIRE1_MODE_TX_RX:
		strcat(name, "MODE_TX_RX 0x");
		break;
	case WIRE1_HWCONTROL:
		strcat(name, "HWCONTROL 0x");
		break;
	case WIRE1_OVERSAMPLING:
		strcat(name, "OVERSAMPLING 0x");
		break;

	default:
		return -1;
	}
	itoa(*param, data, 16);
	return 0;
}
