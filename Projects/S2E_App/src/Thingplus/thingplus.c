#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "timerHandler.h"
#include "MQTTClient.h"
#include "mqtthandler.h"
#include "cJSON.h"
#include "thingplus.h"

/* Private define ------------------------------------------------------------*/
#ifndef SOCK_MQTT
	#define SOCK_MQTT 5 // MQTT socket number for WIZnet platforms
#endif

#define THINGPLUS_VALUE_MESSAGE_LEN  36
#define THINGPLUS_TOPIC_LEN          80
#define THINGPLUS_SHORT_TOPIC_LEN    32

/* Private functions ---------------------------------------------------------*/
uint8_t *(*actuatorCallback)(const uint8_t *id, const uint8_t *cmd, cJSON *options);
uint8_t *_actuatorDo(const uint8_t *id, const uint8_t *cmd, cJSON *options);
void _actuatorResultPublish(const uint8_t *messageId, uint8_t *result);

uint8_t statusPublish(const uint8_t *topic, uint8_t on, time_t durationSec);
uint8_t mqttPublish(const uint8_t *topic, const uint8_t *msg);

/* Private variables ---------------------------------------------------------*/
uint8_t gw_id[13];
const uint8_t *gw_apikey;
const uint8_t *gw_mac;

uint16_t gReportInterval = DEFAULT_REPORT_INTERVAL;

static uint8_t willTopic[32] = {0, };
static uint8_t subscribeTopic[32] = {0, };

/* Public variables ----------------------------------------------------------*/
extern Network n;
extern Client c;
extern MQTTPacket_connectData data;

/* Public functions ----------------------------------------------------------*/
void thingplus_begin(uint8_t *mac, uint8_t *apikey)
{
	//const char *server = "dmqtt.thingplus.net";
	//const int port = 1883;

	uint8_t serverip[4] = {54, 92, 68, 73}; // Thingplus server ip address
	uint16_t serverport = 1883;             // Thingplus server port number
	
	snprintf((char *)gw_id, sizeof(gw_id), (const char *)"%02x%02x%02x%02x%02x%02x", 
		mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	
	snprintf((char *)willTopic, sizeof(willTopic), (const char *)"%s%s%s", 
		TOPIC_GATEWAY, gw_id, TOPIC_MQTT_STATUS);
	
	gw_apikey = apikey;
	gw_mac = mac;
	
	// MQTT init
	MQTT_Init(SOCK_MQTT, serverip, serverport);
	
	// MQTT Connect data init
	data.MQTTVersion = 3;
	data.cleansession = TRUE; //clean true
	data.keepAliveInterval = (gReportInterval/1000) * 2; // in secs
	data.willFlag = TRUE;
	data.will.topicName.cstring = (char *)willTopic;
	data.will.message.cstring   = STATUS_ERR;
	data.will.qos = QOS1;
	data.will.retained = TRUE;
	
	data.clientID.cstring = (char *)gw_id;
	data.username.cstring = (char *)gw_id;
	data.password.cstring = (char *)gw_apikey;
}

void thingplus_connect(void)
{
	int8_t rc;
	
	MQTT_Connect();
	
	do {
		rc = MQTT_Loop();
	} while(rc != MQTT_CONNECTED);
	
	// MQTT Connect
	if(rc == MQTT_CONNECTED)
	{
#ifdef _THINGPLUS_DEBUG_
		printf(" > INFO: MQTT Connected\r\n");
#endif
		snprintf((char *)subscribeTopic, sizeof(subscribeTopic), (const char *)"%s%s%s", 
			TOPIC_GATEWAY, gw_id, TOPIC_REQ);
		
		// MQTT Topic subscribing: Request topic
		rc = MQTTSubscribe(&c, (char *)subscribeTopic, QOS1, thingplus_mqttSubscribeCallback);
		
#ifdef _THINGPLUS_DEBUG_
		printf(" > INFO: Subscribing to %s rc=%d\r\n", subscribeTopic, rc);
#endif
		
		rc = thingplus_mqttStatusPublish(TRUE);
	}
}


void thingplus_disconnect(void)
{
	Network_disconnect(&n);
}


uint8_t thingplus_loop(void)
{
	int8_t rc;
	rc = MQTT_Loop();
	
	if((rc != MQTT_OK) || (rc != MQTT_CONNECTED))
		return 0;
	else
		return 1;
}


int8_t thingplus_mqttStatusPublish(uint8_t on)
{
	int8_t rc;
	uint8_t topic[THINGPLUS_TOPIC_LEN] = {0,};
	
	MQTTMessage msg = {QOS1, 0, 0, 0, NULL, 0};
	
	char *mqttstatus;
	
	// Topic: "v/a/g/GWID/mqtt/status"
	snprintf((char *)topic, sizeof(topic), "%s%s%s", TOPIC_GATEWAY, gw_id, TOPIC_MQTT_STATUS);
	
	// Publish "on" to mqtt/status on "connected"
	if(on) {
		mqttstatus = STATUS_ON;
	} else {
		mqttstatus = STATUS_OFF;
	}
	
	msg.retained = TRUE;
	msg.payload = mqttstatus;
	msg.payloadlen = strlen(mqttstatus);
	
	rc = MQTTPublish(&c, (char *)topic, &msg);
	
#ifdef _THINGPLUS_DEBUG_
	printf(" > INFO: [%.d] MQTTPublish %s rc=%d\r\n", getNow(), topic, rc);
#endif
	
	return rc;
}


uint8_t thingplus_gatewayStatusPublish(uint8_t on, time_t durationSec)
{
	uint8_t statusPublishTopic[THINGPLUS_TOPIC_LEN] = {0,};
	snprintf((char *)statusPublishTopic, sizeof(statusPublishTopic), "%s%s%s", TOPIC_GATEWAY, gw_id, TOPIC_STATUS);
	
	return statusPublish(statusPublishTopic, on, durationSec);
}


uint8_t thingplus_sensorStatusPublish(const uint8_t *id, uint8_t on, time_t durationSec)
{
	uint8_t statusPublishTopic[THINGPLUS_TOPIC_LEN] = {0,};
	snprintf((char *)statusPublishTopic, sizeof(statusPublishTopic), "%s%s/s/%s%s", TOPIC_GATEWAY, gw_id, id, TOPIC_STATUS);
	
	return statusPublish(statusPublishTopic, on, durationSec);
}


uint8_t thingplus_valuePublish_str(const uint8_t *id, char *value)
{
	uint8_t valuePublishTopic[THINGPLUS_TOPIC_LEN] = {0,};
	uint8_t v[THINGPLUS_VALUE_MESSAGE_LEN] = {0,};
	
	// Topic: "v/a/g/GWID/s/SensorOrActuatorID"
	snprintf((char *)valuePublishTopic, sizeof(valuePublishTopic), "%s%s/s/%s", TOPIC_GATEWAY, gw_id, id);
	
	// Value: Char(string)
	snprintf((char *)v, sizeof(v), "%d000,%s", getNow(), value);
	
#ifdef _THINGPLUS_DEBUG_
	printf(" > INFO: [%.d] valuePublish_str %s [%s]\r\n", getNow(), valuePublishTopic, v);
#endif

	return mqttPublish(valuePublishTopic, v);
}


uint8_t thingplus_valuePublish_int(const uint8_t *id, int value)
{
	uint8_t valuePublishTopic[THINGPLUS_TOPIC_LEN] = {0,};
	uint8_t v[THINGPLUS_VALUE_MESSAGE_LEN] = {0,};
	
	// Topic: "v/a/g/GWID/s/SensorOrActuatorID"
	snprintf((char *)valuePublishTopic, sizeof(valuePublishTopic), "%s%s/s/%s", TOPIC_GATEWAY, gw_id, id);
	
	// Value: Integer
	snprintf((char *)v, sizeof(v), "%d000,%d", getNow(), value);
	
#ifdef _THINGPLUS_DEBUG_
	printf(" > INFO: [%.d] valuePublish_int %s [%s]\r\n", getNow(), valuePublishTopic, v);
#endif
	
	return mqttPublish(valuePublishTopic, v);
}


uint8_t thingplus_valuePublish_float(const uint8_t *id, float value)
{
	uint8_t valuePublishTopic[THINGPLUS_TOPIC_LEN] = {0,};
	uint8_t v[THINGPLUS_VALUE_MESSAGE_LEN] = {0,};
	
	// Topic: "v/a/g/GWID/s/SensorOrActuatorID"
	snprintf((char *)valuePublishTopic, sizeof(valuePublishTopic), "%s%s/s/%s", TOPIC_GATEWAY, gw_id, id);
	
	// Value: Float
	snprintf((char *)v, sizeof(v), "%d000,%.2f", getNow(), value);
	
#ifdef _THINGPLUS_DEBUG_
	printf(" > INFO: [%.d] valuePublish_float %s [%s]\r\n", getNow(), valuePublishTopic, v);
#endif
	
	return mqttPublish(valuePublishTopic, v);
}


//void actuatorCallbackSet(char* (*cb)(const char* id, const char* cmd, JsonObject& options));
void thingplus_actuatorCallbackSet(uint8_t* (*cb)(const uint8_t* id, const uint8_t* cmd, cJSON *options))
{
	actuatorCallback = cb;
}

void serverTimeSync(uint32_t serverTime)
{
	setDevtime(serverTime);
	
#ifdef _THINGPLUS_DEBUG_
	printf(" > INFO: Time Synced. NOW: %d\r\n", getNow());
#endif
}


void thingplus_mqttSubscribeCallback(MessageData* md)
{
	unsigned char recvbuffer[250];
	MQTTMessage* message = md->message;
	char topic[50];
	
	cJSON * root;
	char *id = NULL;
	char *method = NULL;
	cJSON *params = NULL;
	char *actuatorId = NULL;
	char *actuatorCmd = NULL;
	cJSON *actuatorOptions = NULL;
	
	uint64_t remoteTime;
	uint8_t *result;

	sprintf(topic, "%.*s", md->topicName->lenstring.len, md->topicName->lenstring.data);
		
	strncpy((char*)recvbuffer, (char*)message->payload, (int)message->payloadlen);
	recvbuffer[(int)message->payloadlen] = '\0';
	
#ifdef _THINGPLUS_DEBUG_
	printf(" >> Message arrived on topic [%s]: (%d) %s\r\n", topic, message->payloadlen, recvbuffer);
	printf("\r\n");
#endif
	
	root = cJSON_Parse((char *)recvbuffer);
	
	if (NULL != cJSON_GetObjectItem(root,"id") &&
		(cJSON_String == cJSON_GetObjectItem(root,"id")->type ||
		cJSON_Number == cJSON_GetObjectItem(root,"id")->type)) 
	{
		id = cJSON_GetObjectItem(root,"id")->valuestring;
	}
	
	if (NULL != cJSON_GetObjectItem(root,"method") &&
		cJSON_String == cJSON_GetObjectItem(root,"method")->type) 
	{
		method = cJSON_GetObjectItem(root,"method")->valuestring;
	}
	
	if(method != NULL)
	{
		params = cJSON_GetObjectItem(root,"params");
		if(params != NULL)
		{
			if(strcmp(method, "controlActuator") == 0)
			{
				if(NULL != cJSON_GetObjectItem(params,"id") &&
					cJSON_String == cJSON_GetObjectItem(params,"id")->type) 
				{
					actuatorId = cJSON_GetObjectItem(params,"id")->valuestring;
				}
				
				if(NULL != cJSON_GetObjectItem(params,"cmd") &&
					cJSON_String == cJSON_GetObjectItem(params,"cmd")->type)
				{
					actuatorCmd = cJSON_GetObjectItem(params,"cmd")->valuestring;
				}
				
				if((params != NULL) && (actuatorId != NULL) && (actuatorCmd != NULL))
				{
					if (NULL != cJSON_GetObjectItem(params,"options") &&
						cJSON_Object == cJSON_GetObjectItem(params,"options")->type)
					{
						actuatorOptions = cJSON_GetObjectItem(params,"options");
					}
					
					result = _actuatorDo((uint8_t *)actuatorId, (uint8_t *)actuatorCmd, actuatorOptions);
					_actuatorResultPublish((uint8_t *)id, (uint8_t *)result);
				}
				else
				{
					// Err: Invalid params
					// -32602, "Invalid params"
#ifdef _THINGPLUS_DEBUG_
					printf(" > ERR: -32602, Invalid params\r\n");
#endif
				}
			}
			else if(strcmp(method, "timeSync") == 0)
			{
				if (NULL != cJSON_GetObjectItem(params,"time") &&
					cJSON_Number == cJSON_GetObjectItem(params,"time")->type)
				{
					remoteTime = cJSON_GetObjectItem(params,"time")->valuedouble;
					// Time sync
					serverTimeSync((uint32_t)(remoteTime/1000)); 
				}
			}
		}
	}
	cJSON_Delete(root);
}


uint8_t *_actuatorDo(const uint8_t *id, const uint8_t *cmd, cJSON *options)
{
	if(!actuatorCallback)
	{
#ifdef _THINGPLUS_DEBUG_
		printf(" > ERR: [actuatorCallback] function cannot be found\r\n");
#endif
		return NULL;
	}
	
	return actuatorCallback(id, cmd, options);
}


void _actuatorResultPublish(const uint8_t *messageId, uint8_t *result)
{
	uint8_t responseTopic[THINGPLUS_SHORT_TOPIC_LEN] = {0,};
	uint8_t responseMessage[64] = {0,};
	
	snprintf((char *)responseTopic, sizeof(responseTopic), "%s%s%s", TOPIC_GATEWAY, gw_id, TOPIC_RES);
	
	if(result)
	{
		snprintf((char *)responseMessage, sizeof(responseMessage), "{\"id\":\"%s\",\"result\":\"%s\"}", messageId, result);
	}
	else
	{
		snprintf((char *)responseMessage, sizeof(responseMessage), "{\"id\":\"%s\",\"error\": {\"code\": -32000}}", messageId);
	}
	
	mqttPublish(responseTopic, responseMessage);
}


uint8_t statusPublish(const uint8_t *topic, uint8_t on, time_t durationSec)
{
	uint8_t status[20] = {0,};
	snprintf((char *)status, sizeof(status), "%s,%d000", on ? "on" : "off", (getNow() + durationSec));
	
#ifdef _THINGPLUS_DEBUG_
	printf(" > INFO: [%.d] statusPublish %s [%s]\r\n", getNow(), topic, status);
#endif
	
	return mqttPublish(topic, status);
}


uint8_t mqttPublish(const uint8_t *topic, const uint8_t *payload)
{
	MQTTMessage msg = {QOS1, 0, 0, 0, NULL, 0};
	
	msg.retained = FALSE;
	msg.payload = (char *)payload;
	msg.payloadlen = strlen((char *)payload);
	
	if(MQTTPublish(&c, (char *)topic, &msg) == FAILURE)
	{
#ifdef _THINGPLUS_DEBUG_
		printf("\r\n");
		printf(" > ERR: Publish failed\r\n");
		printf(" ## TOPIC: %s\r\n", topic);
		printf(" ## PAYLOAD: %s\r\n", payload);
		printf("\r\n");
#endif
		return FALSE;
	}
	
	return TRUE;
}

