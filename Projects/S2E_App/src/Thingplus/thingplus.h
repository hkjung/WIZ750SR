#ifndef THINGPLUS_H
#define THINGPLUS_H

#include <stdint.h>
#include <Time.h>
#include "MQTTClient.h"
#include "cJSON.h"

#define _THINGPLUS_DEBUG_

#define THINGPLUS_SERVER      "dmqtt.thingplus.net"
#define THINGPLUS_PORT        1883

#define TOPIC_GATEWAY         "v/a/g/"
#define TOPIC_STATUS          "/status"
#define TOPIC_MQTT_STATUS     "/mqtt/status"
#define TOPIC_REQ             "/req"
#define TOPIC_RES             "/res"

#define DEFAULT_REPORT_INTERVAL  60000
#define SENSING_INTERVAL         60000

#define STATUS_ON  "on"
#define STATUS_OFF "off"
#define STATUS_ERR "err"

#define TRUE 1
#define FALSE 0

enum PUBTYPE {
	PUBTYPE_CHAR,
	PUBTYPE_INT,
	PUBTYPE_FLOAT
};

void thingplus_begin(uint8_t *mac, uint8_t *apikey);

void thingplus_connect(void);
void thingplus_disconnect(void);

uint8_t thingplus_loop(void);

int8_t thingplus_mqttStatusPublish(uint8_t on);

uint8_t thingplus_gatewayStatusPublish(uint8_t on, time_t durationSec);
uint8_t thingplus_sensorStatusPublish(const uint8_t *id, uint8_t on, time_t durationSec);

uint8_t thingplus_valuePublish_str(const uint8_t *id, char *value);
uint8_t thingplus_valuePublish_int(const uint8_t *id, int value);
uint8_t thingplus_valuePublish_float(const uint8_t *id, float value);

void thingplus_actuatorCallbackSet(uint8_t* (*cb)(const uint8_t* id, const uint8_t* cmd, cJSON *options));
void thingplus_mqttSubscribeCallback(MessageData* md);

#endif // THINGPLUS_H
