#include "AppMain.h"

void MainThread(void *arg);

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
	if (DLogInit()) {
		Error_Handler();
	}
	if (ExeInit()) {
		ErrMessage();
		Error_Handler();
	}

	while (1) {
		vTaskDelay(1000);
	}
}
