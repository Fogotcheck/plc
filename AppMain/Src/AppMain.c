#include "AppMain.h"

EventGroupHandle_t MainEvent = NULL;

void MainThread(void *arg);
void MainEventHandler(EventBits_t Event);

void AppMain(void)
{
	BaseType_t ret =
		xTaskCreate(MainThread, "MainTask", 500, NULL, 1, NULL);
	if (ret != pdPASS) {
		Error_Handler();
	}
	vTaskStartScheduler();
}

void MainThread(__attribute__((unused)) void *arg)
{
	if (osKernelInitialize()) {
		Error_Handler();
	}

	MainEvent = xEventGroupCreate();
	if (MainEvent == NULL) {
		Error_Handler();
	}

	if (DLogInit()) {
		Error_Handler();
	}
	if (ExeInit()) {
		ErrMessage();
		vTaskDelay(10);
		Error_Handler();
	}
	if (MX_LWIP_Init()) {
		ErrMessage();
		vTaskDelay(10);
		Error_Handler();
	}
	if (SupportModuleInit()) {
		ErrMessage();
		vTaskDelay(10);
		Error_Handler();
	}
	if (MqttClientInit()) {
		ErrMessage();
		vTaskDelay(10);
		Error_Handler();
	}
	if (ConfInit()) {
		ErrMessage();
		vTaskDelay(10);
		Error_Handler();
	}

	vTaskDelay(1000);
	DebugMessage("Init::OK");

	EventBits_t Event = 0;
	EventBits_t Mask = 1;
	while (1) {
		Event = xEventGroupWaitBits(MainEvent, MAIN_ALL_EVENTS, pdFALSE,
					    pdFALSE, portMAX_DELAY);
		Mask = 1;
		for (uint8_t i = 0; i < configUSE_16_BIT_TICKS; i++) {
			if (Event & Mask) {
				MainEventHandler(Event & Mask);
			}
			Mask <<= 1;
		}
	}
}

void MainEventHandler(EventBits_t Event)
{
	xEventGroupClearBits(MainEvent, Event);
	DebugMessage("event::0x%x", Event);

	switch (Event) {
	case ETH_LINK_UP:
		MqttClientStart();
		break;
	case ETH_LINK_DOWN:
		MqttClientStop();
		ExeStopAll();
		break;
	case MQTT_LINK_UP:
		ConfSwitch(CONF_EN);
		ExeStartAll();
		break;
	case MQTT_LINK_DOWN:
		ConfSwitch(CONF_DIS);
		ExeStopAll();
		break;

	case MAIN_CRITICAL_ERR:
		ErrMessage();
		vTaskDelay(10);
		Error_Handler();
		break;
	default:
		break;
	}
}
