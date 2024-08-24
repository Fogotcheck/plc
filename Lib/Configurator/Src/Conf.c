#include "Conf.h"

TaskHandle_t ConfHandle = NULL;
QueueHandle_t ConfQueue = NULL;

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
	char data[MQTT_VAR_HEADER_BUFFER_LEN] = { 0 };
	ConfQueue = xQueueCreate(MQTT_VAR_HEADER_BUFFER_LEN, CONF_QUEUE_SIZE);
	if (ConfQueue == NULL) {
		ErrMessage();
		xEventGroupSetBits(MainEvent, MAIN_CRITICAL_ERR);
	}
	DebugMessage("init");
	lwjson_stream_init(&LwJsonStream, prv_callback_func);
	vTaskSuspend(ConfHandle);

	while (1) {
		memset(data, 0, sizeof(data));
		xQueueReceive(ConfQueue, data, portMAX_DELAY);
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
	case CONF_EN:
		vTaskResume(ConfHandle);
		Report.Type = MQTT_PUB;
		MqttClientReportRequest(&Report);
		Report.Type = MQTT_SUB;
		MqttClientReportRequest(&Report);
		break;
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
