#ifndef __wire1_h__
#define __wire1_h__

#include "main.h"
#include "SupportTypes.h"

#define WIRE1_SUFFIX_NAME "_wire_one"

enum Wire1Type {
	WIRE1_TYPE_1,
	WIRE1_TYPE_2,

	WIRE1_COUNT
};

enum WIRE1_PARAM_DEF {
	WIRE1_INSTANCE,
	WIRE1_BAUDRATE,
	WIRE1_WORDLENGTH,
	WIRE1_STOPBITS,
	WIRE1_PARITY,
	WIRE1_MODE_TX_RX,
	WIRE1_HWCONTROL,
	WIRE1_OVERSAMPLING
};

int Wire1GetHandle(SupportInterface_t *Item, uint8_t Type);

#endif //__wire1_h__
