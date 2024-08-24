#include "MqttClient.h"

/* FreeRTOS */
TaskHandle_t MqttClientHandle = NULL;
TimerHandle_t MqttClientTimer = NULL;
QueueHandle_t MqttClientReportQueue = NULL;
EventGroupHandle_t MqttEvent = NULL;

/* Lwip Mqtt */
static mqtt_client_t *mqtt_client;
static struct mqtt_connect_client_info_t mqtt_client_info = {
	"plc",
	"stm", /* user */
	"root", /* pass */
	100, /* keep alive */
	NULL, /* will_topic */
	NULL, /* will_msg */
	0, /* will_msg_len */
	0, /* will_qos */
	0 /* will_retain */
#if LWIP_ALTCP && LWIP_ALTCP_TLS
	,
	NULL
#endif
};
extern ip4_addr_t ipaddr;
static ip_addr_t mqtt_ip = { 0 };
static char TopicPreffix[(MQTT_VAR_HEADER_BUFFER_LEN >> 1)] = { 0 };

void MqttClientThread(void *arg);
void MqttClientPubThread(void *arg);
void MqttTimerCallBack(TimerHandle_t xTimer);

static void mqtt_incoming_publish_cb(void *arg, const char *topic,
				     u32_t tot_len);
static void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len,
				  u8_t flags);
static void mqtt_connection_cb(mqtt_client_t *client, void *arg,
			       mqtt_connection_status_t status);
static void mqtt_request_cb(void *arg, err_t err);

int MqttClientInit(void)
{
	int ret = 0;
	if (xTaskCreate(MqttClientThread, MQTT_CLIENT_THR_NAME,
			MQTT_CLIENT_THR_STACK, NULL, MQTT_CLIENT_THR_PRIOR,
			&MqttClientHandle) != pdPASS) {
		ret = -1;
	}
	vTaskDelay(10);
	return ret;
}

void MqttClientThread(__attribute__((unused)) void *arg)
{
	/* todo: add find server */
	ipaddr_aton("192.168.0.1", &mqtt_ip);
	strcat(TopicPreffix, "plk/");
	strcat(TopicPreffix, ipaddr_ntoa(&ipaddr));
	strcat(TopicPreffix, "/");

	mqtt_client = mqtt_client_new();
	mqtt_set_inpub_callback(mqtt_client, mqtt_incoming_publish_cb,
				mqtt_incoming_data_cb,
				LWIP_CONST_CAST(void *, &mqtt_client_info));

	MqttClientTimer = xTimerCreate(
		"MqttTimer", pdMS_TO_TICKS(MQTT_CLENT_TIMER_PERIOD_MS), pdTRUE,
		NULL, MqttTimerCallBack);
	if (MqttClientTimer == NULL) {
		ErrMessage();
		xEventGroupSetBits(MainEvent, MAIN_CRITICAL_ERR);
	}
	MqttClientReportQueue = xQueueCreate(MQTT_CLIENT_REPORT_QUEUE_COUNT,
					     sizeof(MqttClientReport_t));
	vTaskDelay(10);
	if (MqttClientReportQueue == NULL) {
		ErrMessage();
		xEventGroupSetBits(MainEvent, MAIN_CRITICAL_ERR);
	}
	MqttEvent = xEventGroupCreate();
	if (MqttEvent == NULL) {
		ErrMessage();
		xEventGroupSetBits(MainEvent, MAIN_CRITICAL_ERR);
	}

	DebugMessage("Init");
	MqttClientReport_t Report = { 0 };
	while (1) {
		memset(&Report, 0, sizeof(Report));
		xQueueReceive(MqttClientReportQueue, &Report, portMAX_DELAY);
		char TopicName[MQTT_VAR_HEADER_BUFFER_LEN] = { 0 };
		strcat(TopicName, TopicPreffix);
		strcat(TopicName, Report.TopicName);

		switch (Report.Type) {
		case MQTT_PUB:
			mqtt_publish(mqtt_client, TopicName, Report.TopicData,
				     strlen(Report.TopicData), 0, 0,
				     mqtt_request_cb, NULL);
			break;
		case MQTT_SUB:
			mqtt_subscribe(mqtt_client, TopicName, 1,
				       mqtt_request_cb, NULL);
			break;
		default:
			ErrMessage();
			break;
		}

		EventBits_t Event = xEventGroupWaitBits(
			MqttEvent, MQTT_ALL, pdFALSE, pdFALSE,
			pdMS_TO_TICKS(MQTT_CONNECT_TIMOUT));
		if (Event != MQTT_OK) {
			WarningMessage("Event::ox%x", Event);
		}
		xEventGroupClearBits(MqttEvent, Event);
	}
}

static void mqtt_incoming_publish_cb(void *arg, const char *topic,
				     u32_t tot_len)
{
	__attribute__((unused))
	const struct mqtt_connect_client_info_t *client_info =
		(const struct mqtt_connect_client_info_t *)arg;

	DebugMessage("%s\t%d", topic, (int)tot_len);
}
static void mqtt_incoming_data_cb(void *arg,
				  __attribute__((unused)) const u8_t *data,
				  u16_t len, u8_t flags)
{
	__attribute__((unused))
	const struct mqtt_connect_client_info_t *client_info =
		(const struct mqtt_connect_client_info_t *)arg;

	ConfigBuf_t Buf = { 0 };
	if (len > sizeof(Buf.data)) {
		ErrMessage();
		return;
	}
	memcpy(Buf.data, data, len);
	Buf.size = (uint16_t)len;
	Buf.flag = (uint8_t)flags;
	DebugMessage("%d\t%d", (int)len, (int)flags);

	if (ConfRequest(&Buf)) {
		ErrMessage();
	}
}

static void mqtt_connection_cb(mqtt_client_t *client, void *arg,
			       mqtt_connection_status_t status)
{
	__attribute__((unused))
	const struct mqtt_connect_client_info_t *client_info =
		(const struct mqtt_connect_client_info_t *)arg;
	LWIP_UNUSED_ARG(client);
	DebugMessage("status %d", (int)status);
	if (status == MQTT_CONNECT_ACCEPTED) {
		xQueueReset(MqttClientReportQueue);
		vTaskResume(MqttClientHandle);
		xEventGroupSetBits(MainEvent, MQTT_LINK_UP);
	}
	if (status == MQTT_CONNECT_DISCONNECTED) {
		ErrMessage();
	}
}

void MqttTimerCallBack(TimerHandle_t xTimer)
{
	if (xTimer == MqttClientTimer) {
		BaseType_t pxHigherPriorityTaskWoken = pdFALSE;
		if (mqtt_client_is_connected(mqtt_client)) {
			return;
		}
		mqtt_disconnect(mqtt_client);
		err_t ret = mqtt_client_connect(
			mqtt_client, &mqtt_ip, MQTT_PORT, mqtt_connection_cb,
			LWIP_CONST_CAST(void *, &mqtt_client_info),
			&mqtt_client_info);
		if (ret != ERR_OK) {
			WarningMessage("%d", ret);
		}
		xEventGroupSetBitsFromISR(MainEvent, MQTT_LINK_DOWN,
					  &pxHigherPriorityTaskWoken);
	}
}

void MqttClientStart(void)
{
	xTimerStart(MqttClientTimer,
		    pdMS_TO_TICKS(MQTT_CLIENT_TIMER_DELAYED_START));
}

void MqttClientStop(void)
{
	xTimerStop(MqttClientTimer, 0);
	mqtt_disconnect(mqtt_client);
	vTaskSuspend(MqttClientHandle);
}

int MqttClientReportRequest(MqttClientReport_t *Request)
{
	int ret = xQueueSend(MqttClientReportQueue, Request, 0);
	if (ret != pdTRUE) {
		vTaskDelay(MQTT_CONNECT_TIMOUT);
		ret = xQueueSend(MqttClientReportQueue, Request, 0);
	}

	return ret == pdTRUE ? 0 : -1;
}

static void mqtt_request_cb(__attribute__((unused)) void *arg, err_t err)
{
	if (err == ERR_OK) {
		xEventGroupSetBits(MqttEvent, MQTT_OK);
		return;
	}
	xEventGroupSetBits(MainEvent, MQTT_ERR);
	ErrMessage("err %d", (int)err);
}
