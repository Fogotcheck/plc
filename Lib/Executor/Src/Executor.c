#include "Executor.h"

ExecutorHandle_t ExeHandlers[EXE_THR_COUNT] = { NULL };

static void ExeThreads(void *arg);
void ExeEventHandler(EventBits_t Event, ExecutorTypes_t *Exe);
int ExeInterfaceInit(ExecutorTypes_t *Exe);
int ExeDriverInit(ExecutorTypes_t *Exe);
void ExeRequstReportData(ExecutorTypes_t *Exe);
static inline void ExeCallBack(EventGroupHandle_t EventHandle, EventBits_t Bit);
void ExeTimerCallback(TimerHandle_t xTimer);

int ExeInit(void)
{
	int ret = 0;

	for (uint32_t i = 0; i < sizeof(ExeHandlers) / sizeof(ExeHandlers[0]);
	     i++) {
		char Name[EXE_THR_MAX_NAME] = { 0 };
		itoa(i, Name, 10);
		strcat(Name, EXE_THR_SUFFIX_NAME);
		if (xTaskCreate(ExeThreads, Name, EXE_THR_STACK, (void *)i,
				EXE_THR_PRIORITIES,
				&ExeHandlers[i].Thr) != pdPASS) {
			ret = -1;
		}
		memset(Name, 0, sizeof(Name));
		itoa(i, Name, 10);
		strcat(Name, EXE_TIMER_NAME);
		ExeHandlers[i].Timer =
			xTimerCreate(Name, pdMS_TO_TICKS(EXE_TIMER_PERIOD_MS),
				     pdFALSE, NULL, ExeTimerCallback);
		if (ExeHandlers[i].Timer == NULL) {
			ErrMessage("ExeHandlers[%d]", i);
			ret = -1;
		}

		vTaskDelay(10);
	}

	return ret;
}

static void ExeThreads(void *arg)
{
	ExecutorTypes_t Exe = { 0 };
	Exe.ID = (uint32_t)arg;
	Exe.Handle = &ExeHandlers[Exe.ID];
	Exe.Handle->Queue = xQueueCreate(1, sizeof(ConfChExecute_t));
	if (Exe.Handle->Queue == NULL) {
		ErrMessage("[%d]", Exe.ID);
		Exe.Handle->Thr = NULL;
		vTaskDelete(NULL);
	}

	Exe.Handle->Event = xEventGroupCreate();
	if (Exe.Handle->Event == NULL) {
		ErrMessage("[%d]", Exe.ID);
		Exe.Handle->Thr = NULL;
		vTaskDelete(NULL);
	}

	char tmp[MQTT_OUTPUT_RINGBUF_SIZE] = { 0 };
	strcat(Exe.Preffix, "Executor[");
	itoa(Exe.ID, tmp, 10);
	strcat(Exe.Preffix, tmp);
	strcat(Exe.Preffix, "]/");
	ExecutorBufs_t Buf = { 0 };
	Exe.Buf = &Buf;

	DebugMessage("Init::%d", Exe.ID);
	vTaskSuspend(NULL);
	EventBits_t Event = 0;
	EventBits_t Mask = 1;
	while (1) {
		Event = xEventGroupWaitBits(Exe.Handle->Event, EXE_ALL_EVENTS,
					    pdFALSE, pdFALSE, portMAX_DELAY);
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
		xQueueReceive(Exe->Handle->Queue, &Exe->Conf, 0);
		if (ExeInterfaceInit(Exe)) {
			break;
		}
		if (ExeDriverInit(Exe)) {
			break;
		}
		Exe->Interface->CallBack.BitRawData = EXE_HANDLE_DATA;
		Exe->Interface->CallBack.EventHandle = Exe->Handle->Event;
		Exe->Interface->CallBack.Func = ExeCallBack;
		xTimerStart(Exe->Handle->Timer, 0);
		break;
	}

	case EXE_REQUEST_DATA: {
		if (Exe->Driver->Request == NULL) {
			ErrMessage("Exe[%d]", Exe->ID);
			break;
		}
		if (Exe->Driver->Request(Exe->Interface, Exe->Conf.Driver.Param,
					 Exe->Buf->tmp)) {
			ErrMessage("Exe[%d]", Exe->ID);
		}
		break;
	}

	case EXE_HANDLE_DATA: {
		if (Exe->Driver->RawDataHandle != NULL) {
			Exe->Driver->RawDataHandle(Exe->Conf.Driver.Param,
						   Exe->Buf->tmp,
						   Exe->Buf->raw);
		}
		ExeRequstReportData(Exe);
		xTimerStart(Exe->Handle->Timer, 0);
		break;
	}

	default:
		WarningMessage("Exe[%d]", Exe->ID);
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

int ExeConfigure(ConfChExecute_t *Conf)
{
	if (Conf->ch >= sizeof(ExeHandlers) / sizeof(ExeHandlers[0])) {
		return -1;
	}

	if (xQueueOverwrite(ExeHandlers[Conf->ch].Queue, Conf) != pdPASS) {
		return -1;
	}
	xEventGroupSetBits(ExeHandlers[Conf->ch].Event, EXE_INIT);
	return 0;
}

int ExeInterfaceInit(ExecutorTypes_t *Exe)
{
	if (SupportGetInterface(Exe->Conf.Interface.Name, &Exe->Interface)) {
		WarningMessage("Interface[%d]::\"%s\" is n't support", Exe->ID,
			       Exe->Conf.Interface.Name);
		return -1;
	}
	if (Exe->Interface == NULL) {
		ErrMessage("Interface[%d]", Exe->ID);
		return -1;
	}
	if (Exe->Conf.Interface.Param[0] == 0) {
		if (Exe->Interface->SetDefault == NULL) {
			WarningMessage("Interface[%d]::No default param");
		} else {
			Exe->Interface->SetDefault(Exe->Interface,
						   Exe->Conf.Interface.Param);
		}
	}
	if ((Exe->Interface->Init == NULL) ||
	    (Exe->Interface->DeInit == NULL)) {
		ErrMessage("Interface[%d]", Exe->ID);
		return -1;
	}

	if (Exe->Interface->DeInit(Exe->Interface)) {
		ErrMessage("Interface[%d]", Exe->ID);
	}
	if (Exe->Interface->Init(Exe->Interface, Exe->Conf.Interface.Param)) {
		ErrMessage("Interface[%d]", Exe->ID);
	}

	MqttClientReport_t Report = { 0 };
	Report.Type = MQTT_PUB;
	strcat(Report.TopicName, Exe->Preffix);
	strcat(Report.TopicName, "Interface/");
	strcat(Report.TopicData, Exe->Interface->Name);
	if (MqttClientReportRequest(&Report)) {
		ErrMessage("Interface[%d]", Exe->ID);
	}

	if (Exe->Interface->ParamInterpret != NULL) {
		for (uint16_t i = 0;
		     i < sizeof(Exe->Conf.Interface.Param) /
				 sizeof(Exe->Conf.Interface.Param[0]);
		     i++) {
			MqttClientReport_t ReportParam = { 0 };
			ReportParam.Type = MQTT_PUB;
			strcat(ReportParam.TopicName, Exe->Preffix);
			strcat(ReportParam.TopicName, "Interface/Param/");

			if (Exe->Interface->ParamInterpret(
				    i, &Exe->Conf.Interface.Param[i],
				    ReportParam.TopicName,
				    ReportParam.TopicData) == 0) {
				if (MqttClientReportRequest(&ReportParam)) {
					ErrMessage("Interface[%d]", Exe->ID);
				}
				vTaskDelay(MQTT_CONNECT_TIMOUT);
			}
		}
	}

	return 0;
}

int ExeDriverInit(ExecutorTypes_t *Exe)
{
	if (SupportGetDriver(Exe->Conf.Driver.Name, &Exe->Driver)) {
		WarningMessage("Driver[%d]::\"%s\" is n't support", Exe->ID,
			       Exe->Conf.Interface.Name);
		return -1;
	}
	if (Exe->Driver == NULL) {
		ErrMessage("Driver[%d]", Exe->ID);
		return -1;
	}
	if (Exe->Conf.Driver.Param[0] == 0) {
		if (Exe->Driver->SetDefault == NULL) {
			WarningMessage("Driver[%d]::No default param");
		} else {
			Exe->Driver->SetDefault(Exe->Interface,
						Exe->Conf.Driver.Param);
		}
	}
	if (Exe->Driver->Init == NULL) {
		ErrMessage("Driver[%d]", Exe->ID);
		return -1;
	}
	if (Exe->Driver->Init(Exe->Interface, Exe->Conf.Driver.Param)) {
		ErrMessage("Driver[%d]", Exe->ID);
		return -1;
	}

	MqttClientReport_t Report = { 0 };
	Report.Type = MQTT_PUB;
	strcat(Report.TopicName, Exe->Preffix);
	strcat(Report.TopicName, "Driver/");
	strcat(Report.TopicData, Exe->Driver->Name);
	if (MqttClientReportRequest(&Report)) {
		ErrMessage("Driver[%d]", Exe->ID);
	}

	if (Exe->Driver->ParamInterpret != NULL) {
		for (uint16_t i = 0;
		     i < sizeof(Exe->Conf.Driver.Param) /
				 sizeof(Exe->Conf.Driver.Param[0]);
		     i++) {
			MqttClientReport_t ReportParam = { 0 };
			ReportParam.Type = MQTT_PUB;
			strcat(ReportParam.TopicName, Exe->Preffix);
			strcat(ReportParam.TopicName, "Driver/Param/");

			if (Exe->Driver->ParamInterpret(
				    &Exe->Conf.Driver.Param[i],
				    ReportParam.TopicName,
				    ReportParam.TopicData) == 0) {
				if (MqttClientReportRequest(&ReportParam)) {
					ErrMessage("Driver[%d]", Exe->ID);
				}
				vTaskDelay(MQTT_CONNECT_TIMOUT);
			}
		}
	}

	return 0;
}

void ExeRequstReportData(ExecutorTypes_t *Exe)
{
	if (Exe->Driver->DataInterpret == NULL) {
		WarningMessage("Exe[%d]", Exe->ID);
		return;
	}
	for (uint16_t j = 0;
	     j < sizeof(Exe->Buf->cplt) / sizeof(Exe->Buf->cplt[0]); j++) {
		if (Exe->Buf->raw[j] != Exe->Buf->cplt[j]) {
			MqttClientReport_t Report = { 0 };
			Report.Type = MQTT_PUB;
			strcat(Report.TopicName, Exe->Preffix);
			strcat(Report.TopicName, "Data_");
			strcat(Report.TopicName, Exe->Driver->Name);
			strcat(Report.TopicName, "/");

			if (Exe->Driver->DataInterpret(j, &Exe->Buf->raw[j],
						       Report.TopicName,
						       Report.TopicData) == 0) {
				if (MqttClientReportRequest(&Report)) {
					ErrMessage("[%d]", Exe->ID);
				}
			}
			Exe->Buf->cplt[j] = Exe->Buf->raw[j];
		}
	}
}

void ExeTimerCallback(TimerHandle_t xTimer)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	for (uint16_t i = 0; i < sizeof(ExeHandlers) / sizeof(ExeHandlers[0]);
	     i++) {
		if (xTimer == ExeHandlers[i].Timer) {
			if (ExeHandlers[i].Event != NULL) {
				xEventGroupSetBitsFromISR(
					ExeHandlers[i].Event, EXE_REQUEST_DATA,
					&xHigherPriorityTaskWoken);
			}
		}
	}
}

static inline void ExeCallBack(EventGroupHandle_t EventHandle, EventBits_t Bit)
{
	osEventFlagsSet(EventHandle, Bit);
}
