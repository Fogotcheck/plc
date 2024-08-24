#ifndef __ConfDef_h__
#define __ConfDef_h__

#define CONF_THR_NAME "ConfTask"
#define CONF_THR_STACK 512
#define CONF_THR_PRIOR (osPriorityHigh7)

#define CONF_QUEUE_SIZE 50

#define CONF_TOPIC_SUFFIX "Configurator"
#define CONF_DATA_HELLO "Configurator is wait"

enum ConfSwitchState {
	CONF_DIS,
	CONF_EN,
};

#endif //__ConfDef_h__
