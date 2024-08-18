#include "Executor.h"

ExecutorHandle_t ExeHandlers[EXE_THR_COUNT] = { NULL };

void ExeThreads(void *arg);
void ExeEventHandler(EventBits_t Event, ExecutorTypes_t *Exe);

int ExeInit(void)
{
	int ret = 0;

	for (uint32_t i = 0; i < sizeof(ExeHandlers) / sizeof(ExeHandlers[0]);
	     i++) {
		char ThrName[EXE_THR_MAX_NAME] = { 0 };
		itoa(i, ThrName, 10);
		strcat(ThrName, EXE_THR_SUFFIX_NAME);
		if (xTaskCreate(ExeThreads, ThrName, EXE_THR_STACK, (void *)i,
				EXE_THR_PRIORITIES,
				&ExeHandlers[i].Thr) != pdPASS) {
			ret = -1;
		}
		vTaskDelay(10);
	}

	return ret;
}

void ExeThreads(void *arg)
{
	ExecutorTypes_t Exe = { 0 };
	Exe.ID = (uint32_t)arg;
	Exe.Handle = &ExeHandlers[Exe.ID];
	Exe.Handle->Event = xEventGroupCreate();
	if (Exe.Handle->Event == NULL) {
		ErrMessage("[%d]", Exe.ID);
		Exe.Handle->Thr = NULL;
		vTaskDelete(NULL);
	}
	DebugMessage("Init::%d", Exe.ID);
	vTaskSuspend(NULL);
	EventBits_t Event = 0;
	EventBits_t Mask = 1;
	while (1) {
		Event = xEventGroupWaitBits(Exe.Handle->Event, 1, pdFALSE,
					    pdFALSE, portMAX_DELAY);
		Mask = 1;
		for (uint8_t i = 0; i < configUSE_16_BIT_TICKS; i++) {
			if (Event & Mask) {
				ExeEventHandler(Event & Mask, &Exe);
			}
			Mask <<= 1;
		}
	}
}

void ExeEventHandler(EventBits_t Event, ExecutorTypes_t *Exe)
{
	xEventGroupClearBits(Exe->Handle->Event, Event);
	DebugMessage("Event[%d]::0x%x", Exe->ID, Event);

	switch (Event) {
	case EXE_INIT: {
		/* code */
		break;
	}

	case EXE_REQUEST_DATA: {
		/* code */
		break;
	}

	case EXE_HANDLE_DATA: {
		/* code */
		break;
	}

	case EXE_REPORT_DATA: {
		/* code */
		break;
	}

	default:
		break;
	}
}

void ExeStartAll(void)
{
	for (uint8_t i = 0; i < sizeof(ExeHandlers) / sizeof(ExeHandlers[0]);
	     i++) {
		if (ExeHandlers[i].Thr != NULL) {
			vTaskResume(ExeHandlers[i].Thr);
		}
	}
}

void ExeStopAll(void)
{
	for (uint8_t i = 0; i < sizeof(ExeHandlers) / sizeof(ExeHandlers[0]);
	     i++) {
		if (ExeHandlers[i].Thr != NULL) {
			vTaskSuspend(ExeHandlers[i].Thr);
		}
	}
}
