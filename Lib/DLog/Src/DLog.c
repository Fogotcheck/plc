#include "DLog.h"

DLogMSGList_t DLogFifoMem[DLOG_MAX_FIFO_LEN] = { 0 };
DLogMSGList_t *DLogHead = NULL;
uint8_t __attribute__((aligned(1))) DLogBuf[DLOG_MAX_BUF_LEN] = { 0 };

extern DMA_HandleTypeDef hdma_usart3_tx;
extern UART_HandleTypeDef huart3;
DMA_HandleTypeDef *DLog_hdma_tx = &hdma_usart3_tx;
UART_HandleTypeDef *DLogUart = &huart3;

#if defined(DLOG_USE_RTOS)
SemaphoreHandle_t DLogMutex = NULL;
#endif

static uint8_t DLogLevelState = LOG_LEVEL;

void DLogPrintTest(void);

void DLogAddItem(DLogMSG_t *data);
void DLogGetNew(DLogMSGList_t **New);
void DLogGetData(DLogMSG_t **data);
int DLogCheckMSG(DLogMSG_t *MSG);
int DLogCheckLoglevel(uint8_t *Loglevel);
void DLogRequestTransmit(void);
uint16_t DLogMSGHandler(__attribute__((unused)) uint8_t LogLevel,
			const char *tag, ...);

int DLogInit(void)
{
	/* todo:: bad firts byte */
	HAL_DMA_Abort(DLog_hdma_tx);
	HAL_UART_Abort(DLogUart);

#if defined(DLOG_USE_RTOS)
	DLogMutex = xSemaphoreCreateMutex();
	if (DLogMutex == NULL) {
		return -1;
	}
#endif

	DebugMessage("LoglevelState::0x%x", DLogLevelState);
	// DLogPrintTest();
	return 0;
}

void DLogPrintTest(void)
{
	char tmp[] = "Hello world";
	for (uint8_t k = 0; k < 5; k++) {
		for (uint8_t j = 0; j < 10; j++) {
			for (uint8_t i = 0; i < 10; i++) {
				DLogMSGHandler(1, "str[0x%x][%d]::%s\t%d\r\n",
					       k, j, tmp, i);
				if (i == 10) {
					i++;
				}
			}
			HAL_Delay(20);
		}
	}

	ErrMessage("hello::%d", 10);
	WarningMessage("hello::0x%x", 10);

	DebugMessage(
		"Hello Hello Hello Hello Hello Hello Hello Hello Hello Hello "
		"Hello Hello Hello Hello Hello Hello Hello Hello Hello Hello "
		"Hello Hello Hello Hello Hello Hello Hello Hello Hello Hello ");
}

__attribute__((format(gnu_printf, 2, 3))) uint16_t
DLogMSGHandler(uint8_t LogLevel, const char *tag, ...)
{
#if defined(DLOG_USE_RTOS)
	if (osSemaphoreAcquire(DLogMutex, portMAX_DELAY) != osOK) {
		return 0;
	}
#endif

	if (DLogCheckLoglevel(&LogLevel)) {
		return 0;
	}

	static uint8_t *Cur = &DLogBuf[0];
	if (Cur > (&DLogBuf[0] + sizeof(DLogBuf) / sizeof(DLogBuf[0]) -
		   DLOG_MAX_MSG_LEN)) {
		Cur = &DLogBuf[0];
	}
	DLogMSG_t MSG = { 0 };
	MSG.Mem = Cur;
	va_list args;
	va_start(args, tag);
	int len = vsnprintf((char *)Cur, DLOG_MAX_MSG_LEN, tag, args);
	MSG.Size = len;
	DLogAddItem(&MSG);
	va_end(args);
	DLogRequestTransmit();
	Cur += len;

#if defined(DLOG_USE_RTOS)
	if (osSemaphoreRelease(DLogMutex) != osOK) {
		return 0;
	}
#endif

	return len;
}

int DLogCheckLoglevel(uint8_t *Loglevel)
{
	if ((DLogLevelState & DLOG_LEVEL_UART_PORT) == 0) {
		return -1;
	}
	if ((*Loglevel & DLogLevelState) == 0) {
		return -2;
	}
	return 0;
}

void DLogAddItem(DLogMSG_t *data)
{
	if (DLogCheckMSG(data)) {
		return;
	}

	DLogMSGList_t *Item = NULL;
	DLogGetNew(&Item);
	Item->MSG.Mem = data->Mem;
	Item->MSG.Size = data->Size;
	Item->next = DLogHead;
	DLogHead = Item;
}

void DLogGetData(DLogMSG_t **data)
{
	*data = NULL;
	DLogMSGList_t *tmp = DLogHead;
	if (tmp == NULL) {
		return;
	}
	DLogMSGList_t *prev = tmp;
	while (tmp->next) {
		prev = tmp;
		tmp = tmp->next;
	}
	tmp->state = DLOG_FIFO_EMPTY;
	*data = &tmp->MSG;
	if (tmp == prev) {
		DLogHead = NULL;
	}

	prev->next = NULL;
}

void DLogGetNew(DLogMSGList_t **New)
{
	*New = &DLogFifoMem[0];
	for (uint16_t i = 0; i < sizeof(DLogFifoMem) / sizeof(DLogFifoMem[0]);
	     i++) {
		if (DLogFifoMem[i].state == DLOG_FIFO_EMPTY) {
			DLogFifoMem[i].state = DLOG_FIFO_BUSY;
			*New = &DLogFifoMem[i];
			return;
		}
	}
	for (uint16_t i = 0; i < sizeof(DLogFifoMem) / sizeof(DLogFifoMem[0]);
	     i++) {
		DLogFifoMem[i].state = DLOG_FIFO_EMPTY;
		DLogFifoMem[i].next = NULL;
	}
	(*New)->state = DLOG_FIFO_BUSY;
	DLogHead = NULL;
}

void DLogRequestTransmit(void)
{
	if (DLogUart->gState != HAL_UART_STATE_READY) {
		return;
	}
	DLogMSG_t *data = NULL;
	DLogGetData(&data);
	if (data == NULL) {
		return;
	}

	__attribute__((unused)) HAL_StatusTypeDef ret =
		HAL_UART_Transmit_DMA(DLogUart, data->Mem, data->Size);
}

int DLogCheckMSG(DLogMSG_t *MSG)
{
	if ((MSG->Mem < &DLogBuf[0]) ||
	    (MSG->Mem > (&DLogBuf[0] + sizeof(DLogBuf) / sizeof(DLogBuf[0]))) ||
	    ((MSG->Mem + MSG->Size) >
	     (&DLogBuf[0] + sizeof(DLogBuf) / sizeof(DLogBuf[0])))) {
		return -1;
	}
	return 0;
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart == DLogUart) {
		DLogRequestTransmit();
	}
}
