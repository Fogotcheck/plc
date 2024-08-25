#include "Executor.h"

ExecutorHandle_t ExeHandlers[EXE_THR_COUNT] = { NULL };

void ExeThreads(void *arg);
void ExeEventHandler(EventBits_t Event, ExecutorTypes_t *Exe);
int ExeInterfaceInit(ExecutorTypes_t *Exe);
int ExeDriverInit(ExecutorTypes_t *Exe);

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
		vTaskDelay(10);
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
	char tmp[MQTT_OUTPUT_RINGBUF_SIZE] = { 0 };
	itoa(Exe->ID, tmp, 10);
	strcat(Report.TopicName, "Executor[");
	strcat(Report.TopicName, tmp);
	strcat(Report.TopicName, "]/Interface/");
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
			memset(tmp, 0, sizeof(tmp));
			itoa(Exe->ID, tmp, 10);
			strcat(ReportParam.TopicName, "Executor[");
			strcat(ReportParam.TopicName, tmp);
			strcat(ReportParam.TopicName, "]/Interface/Param/");
			memset(tmp, 0, sizeof(tmp));
			if (Exe->Interface->ParamInterpret(
				    i, &Exe->Conf.Interface.Param[i], tmp,
				    ReportParam.TopicData) == 0) {
				strcat(ReportParam.TopicName, tmp);
				if (MqttClientReportRequest(&ReportParam)) {
					ErrMessage("Interface[%d]", Exe->ID);
				}
				vTaskDelay(MQTT_CLENT_TIMER_PERIOD_MS);
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

	return 0;
}
