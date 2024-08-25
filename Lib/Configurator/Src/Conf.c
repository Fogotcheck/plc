#include "Conf.h"

TaskHandle_t ConfHandle = NULL;
QueueHandle_t ConfQueue = NULL;

ConfChExecute_t Configs[EXE_THR_COUNT] = { 0 };
static lwjson_stream_parser_t LwJsonStream;

void ConfThread(void *arg);
static void prv_callback_func(lwjson_stream_parser_t *jsp,
			      lwjson_stream_type_t type);
void ConfSetCh(uint32_t num, ConfChExecute_t **ch);

void ConfInterfaceParse(lwjson_stream_parser_t *jsp, lwjson_stream_type_t type,
			ConfChExecute_t *Ch);
void ConfDriverParse(lwjson_stream_parser_t *jsp, lwjson_stream_type_t type,
		     ConfChExecute_t *Ch);

int ConfInit(void)
{
	int ret = 0;
	if (xTaskCreate(ConfThread, CONF_THR_NAME, CONF_THR_STACK, NULL,
			CONF_THR_PRIOR, &ConfHandle) != pdPASS) {
		ret = -1;
	}
	vTaskDelay(10);
	return ret;
}

void ConfThread(__attribute__((unused)) void *arg)
{
	ConfigBuf_t Buf = { 0 };
	ConfQueue = xQueueCreate(CONF_QUEUE_SIZE, sizeof(Buf));
	if (ConfQueue == NULL) {
		ErrMessage();
		xEventGroupSetBits(MainEvent, MAIN_CRITICAL_ERR);
	}
	DebugMessage("init");
	lwjson_stream_init(&LwJsonStream, prv_callback_func);

	vTaskSuspend(ConfHandle);
	lwjsonr_t res;
	while (1) {
		memset(&Buf, 0, sizeof(Buf));
		xQueueReceive(ConfQueue, &Buf, portMAX_DELAY);

		for (uint16_t i = 0; i < Buf.size; i++) {
			res = lwjson_stream_parse(&LwJsonStream, Buf.data[i]);
			switch (res) {
			case lwjsonSTREAMINPROG:
				break;
			case lwjsonSTREAMWAITFIRSTCHAR:
				break;
			case lwjsonSTREAMDONE:
				for (uint16_t j = 0;
				     j < sizeof(Configs) / sizeof(Configs[0]);
				     j++) {
					if ((strlen(Configs[j].Interface.Name)) &&
					    (strlen(Configs[j].Driver.Name))) {
						if (ExeConfigure(&Configs[j])) {
							ErrMessage();
						}
						memset(Configs[j].Interface.Name,
						       0,
						       sizeof(Configs[j]
								      .Interface
								      .Name));
						memset(Configs[j].Driver.Name,
						       0,
						       sizeof(Configs[j]
								      .Driver
								      .Name));
					}
				}

				break;
			default:
				ErrMessage();
				break;
			}
		}
	}
}

void ConfSwitch(uint8_t State)
{
	MqttClientReport_t Report = { 0 };

	strcat(Report.TopicName, CONF_TOPIC_SUFFIX);
	strcat(Report.TopicData, CONF_DATA_HELLO);
	switch (State) {
	case CONF_DIS:
		vTaskSuspend(ConfHandle);
		break;
	case CONF_EN: {
		vTaskResume(ConfHandle);
		Report.Type = MQTT_PUB;
		if (MqttClientReportRequest(&Report)) {
			ErrMessage();
		}
		Report.Type = MQTT_SUB;
		if (MqttClientReportRequest(&Report)) {
			ErrMessage();
		}
		break;
	}
	default:
		break;
	}
}

static void prv_callback_func(lwjson_stream_parser_t *jsp,
			      lwjson_stream_type_t type)
{
	if ((jsp->stack_pos < 2) || (type < LWJSON_STREAM_TYPE_STRING)) {
		return;
	}
	static ConfChExecute_t *CurCh = 0;

	if ((lwjson_stack_seq_5(jsp, 0, OBJECT, KEY, ARRAY, OBJECT, KEY)) &&
	    (type == LWJSON_STREAM_TYPE_NUMBER)) {
		if ((strcmp(jsp->stack[1].meta.name, "ConfCH") == 0) &&
		    (strcmp(jsp->stack[4].meta.name, "NumCH") == 0)) {
			ConfSetCh(atoi(jsp->data.str.buff), &CurCh);
		}
	}
	if (CurCh == NULL) {
		return;
	}
	if (lwjson_stack_seq_3(jsp, 4, KEY, OBJECT, KEY)) {
		ConfInterfaceParse(jsp, type, CurCh);
		ConfDriverParse(jsp, type, CurCh);
		// ConfPostHandleParse(jsp, type, CurCh);
	}
}

void ConfSetCh(uint32_t num, ConfChExecute_t **ch)
{
	*ch = NULL;
	if (num >= sizeof(Configs) / sizeof(Configs[0])) {
		return;
	}
	Configs[num].ch = num;
	*ch = &Configs[num];
}

int ConfRequest(ConfigBuf_t *Buf)
{
	int ret = xQueueSend(ConfQueue, Buf, 0);
	if (ret != pdTRUE) {
		vTaskDelay(10);
		ret = xQueueSend(ConfQueue, Buf, 0);
	}
	return ret == pdTRUE ? 0 : -1;
}

void ConfInterfaceParse(lwjson_stream_parser_t *jsp, lwjson_stream_type_t type,
			ConfChExecute_t *Ch)
{
	static uint8_t InterfaceParamCount = 0;
	if (strcmp(jsp->stack[4].meta.name, "Interface") == 0) {
		if (strcmp(jsp->stack[6].meta.name, "Type") == 0) {
			size_t len = strlen(jsp->data.str.buff);
			if (len >= sizeof(Ch->Interface.Name)) {
				return;
			}
			memcpy(Ch->Interface.Name, jsp->data.str.buff, len);
		}
		if ((strcmp(jsp->stack[6].meta.name, "Param") == 0) &&
		    (type == LWJSON_STREAM_TYPE_NUMBER)) {
			if (InterfaceParamCount >=
			    SUPPORT_INTERFACE_PARAM_SIZE) {
				return;
			}
			Ch->Interface.Param[InterfaceParamCount++] =
				atoi(jsp->data.str.buff);
		} else {
			InterfaceParamCount = 0;
		}
	} else {
		InterfaceParamCount = 0;
	}
}

void ConfDriverParse(lwjson_stream_parser_t *jsp, lwjson_stream_type_t type,
		     ConfChExecute_t *Ch)
{
	static uint8_t DriverParamCount = 0;
	if (strcmp(jsp->stack[4].meta.name, "Driver") == 0) {
		if (strcmp(jsp->stack[6].meta.name, "Type") == 0) {
			size_t len = strlen(jsp->data.str.buff);
			if (len >= sizeof(Ch->Driver.Name)) {
				return;
			}
			memcpy(Ch->Driver.Name, jsp->data.str.buff, len);
		}

		if ((strcmp(jsp->stack[6].meta.name, "Param") == 0) &&
		    (type == LWJSON_STREAM_TYPE_NUMBER)) {
			if (DriverParamCount >= SUPPORT_DRIVER_PARAM_SIZE) {
				return;
			}
			Ch->Driver.Param[DriverParamCount++] =
				atoi(jsp->data.str.buff);
		} else {
			DriverParamCount = 0;
		}
	} else {
		DriverParamCount = 0;
	}
}

// void ConfPostHandleParse(lwjson_stream_parser_t *jsp, lwjson_stream_type_t type,
// 			 ConfChExecute_t *Ch)
// {
// if (strcmp(jsp->stack[4].meta.name, "Posthandle") == 0) {
// 	if (strcmp(jsp->stack[6].meta.name, "filter") == 0) {
// 		if (type == LWJSON_STREAM_TYPE_TRUE) {
// 			Ch->PostHandle.filter = 1;
// 		}
// 	}
// }
// }
