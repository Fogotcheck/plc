#include "Conf.h"

TaskHandle_t ConfHandle = NULL;
QueueHandle_t ConfQueue = NULL;

ConfChExecute_t Configs[EXE_THR_COUNT] = { 0 };
static lwjson_stream_parser_t LwJsonStream;

void ConfThread(void *arg);
static void prv_callback_func(lwjson_stream_parser_t *jsp,
			      lwjson_stream_type_t type);

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
