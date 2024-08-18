#include "MqttClient.h"

/* FreeRTOS */
TaskHandle_t MqttClientHandle = NULL;
TimerHandle_t MqttClientTimer = NULL;
QueueHandle_t MqttClientReportQueue = NULL;

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

	DebugMessage("Init");
	MqttClientReport_t Report = { 0 };
	while (1) {
		xQueueReceive(MqttClientReportQueue, &Report, portMAX_DELAY);
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
static void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len,
				  u8_t flags)
{
	__attribute__((unused))
	const struct mqtt_connect_client_info_t *client_info =
		(const struct mqtt_connect_client_info_t *)arg;

	LWIP_UNUSED_ARG(data);

	DebugMessage("%d\t%d", (int)len, (int)flags);
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
