#ifndef __ds1820_h__
#define __ds1820_h__

#include "main.h"
#include "SupportTypes.h"

#define DS1820_SUFFIX_NAME "Ds1820"

enum Ds1820_Register_Mapping_Address {
	DS1820_NOID = 0xcc,
	DS1820_T_CONVERT = 0x44,
	DS1820_READ_DATA = 0xbe,
	DS1820_READ_ROM = 0x33,
};

int Ds1820GetHandle(SupportDrivers_t *Item);

#endif //__ds1820_h__
