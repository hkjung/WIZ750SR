#include <string.h>
#include <stdlib.h>
#include "common.h"
#include "W7500x_board.h"
#include "W7500x_wztoe.h"
#include "wizchip_conf.h"
#include "socket.h"

#include "MQTTClient.h"
#include "mqttHandler.h"
#include "thingplus.h"

/* Private define ------------------------------------------------------------*/
#define MQTT_BUF_SIZE      1024

/* Private functions ---------------------------------------------------------*/
uint16_t get_client_any_port(void);

/* Private variables ---------------------------------------------------------*/
wiz_NetInfo net;
uint8_t gWritebuf[MQTT_BUF_SIZE];
uint8_t gReadbuf[MQTT_BUF_SIZE];

static uint8_t brokerip[4] = {0, };
static uint16_t brokerport = 0;

static uint8_t mqtt_init_complete = 0;
static uint16_t client_any_port = 0;

Network n;
Client c = DefaultClient;
MQTTPacket_connectData data = MQTTPacket_connectData_initializer;

volatile uint32_t MilliTimer = 0;
static volatile uint32_t prev_MilliTimer = 0;

uint8_t isSocketOpen_MQTTclient = OFF;

/* Public functions ----------------------------------------------------------*/
void MQTT_Init(uint8_t s, uint8_t *bip, uint16_t bport)
{
	strncpy((char *)brokerip, (char *)bip, 4);
	brokerport = bport;
	
	// MQTT Socket number, callback functions init
	NewNetwork(s, &n); 
	
	// MQTT client init
	MQTTClient(&c, &n, MQTT_TIMEOUT_MS, gWritebuf, DATA_BUF_SIZE, gReadbuf, DATA_BUF_SIZE);
	
	// Flag: MQTT init complete
	mqtt_init_complete = 1;
}

int8_t MQTT_Connect(void)
{
	int8_t rc;
	rc = ConnectNetwork(&n, brokerip, brokerport);
	
	return rc;
}

int8_t MQTT_Loop(void)
{
	int8_t ret = MQTT_OK;
	int8_t rc = 0;
	
	static uint32_t prev_time;
	
	if(mqtt_init_complete)
	{
		uint16_t source_port;
		uint8_t destip[4] = {0, };
		uint16_t destport = 0;
		
		uint8_t state = getSn_SR(n.my_socket);
		switch(state)
		{
			case SOCK_INIT:
				if(get_mqtt_time() >= (prev_time + MQTT_RECONNECTION_INTERVAL_MS))
				{
					// MQTT client: TCP connect
					connect(n.my_socket, brokerip, brokerport);
					prev_time = get_mqtt_time();
					
#ifdef _MQTTHANDLER_DEBUG_
					printf(" > MQTT: Try to connect to MQTT Broker - %d.%d.%d.%d : %d\r\n", brokerip[0], brokerip[1], brokerip[2], brokerip[3], brokerport);
#endif
				}
				break;
			
			case SOCK_ESTABLISHED:
				if(getSn_IR(n.my_socket) & Sn_IR_CON)
				{
					getsockopt(n.my_socket, SO_DESTIP, &destip);
					getsockopt(n.my_socket, SO_DESTPORT, &destport);
					
#ifdef _MQTTHANDLER_DEBUG_
					printf(" > MQTT: Connected - %d.%d.%d.%d : %d\r\n",destip[0], destip[1], destip[2], destip[3], destport);
#endif
					// Debug message enable flag: TCP client sokect open 
					isSocketOpen_MQTTclient = OFF;
					
					rc = MQTTConnect(&c, &data);
					
					/*
					// for Debugging
					printf(" >> data.MQTTVersion: %d\r\n", data.MQTTVersion);
					printf(" >> data.cleansession: %d\r\n", data.cleansession);
					printf(" >> data.keepAliveInterval: %d\r\n", data.keepAliveInterval);
					printf(" >> data.willFlag: %d\r\n", data.willFlag);
					printf(" >> data.will.topicName.cstring: %s\r\n", data.will.topicName.cstring);
					printf(" >> data.will.message.cstring: %s\r\n", data.will.message.cstring);
					printf(" >> data.will.qos: %d\r\n", data.will.qos);
					printf(" >> data.will.retained: %d\r\n", data.will.retained);
					printf(" >> data.clientID.cstring: %s\r\n", data.clientID.cstring);
					printf(" >> data.username.cstring: %s\r\n", data.username.cstring);
					printf(" >> data.password.cstring: %s\r\n", data.password.cstring);
					*/
					
					if(rc == SUCCESSS)
					{
#ifdef _MQTTHANDLER_DEBUG_
						printf(" > MQTT: Connection Successful\r\n\r\n");
#endif
						ret = MQTT_CONNECTED;
					}
					else
					{
						// MQTT socket disconnect
#ifdef _MQTTHANDLER_DEBUG_
						printf(" > MQTT: Connect Failed: %d\r\n\r\n", rc);
#endif
						Network_disconnect(&n);
						return rc;
					}
					
					setSn_IR(n.my_socket, Sn_IR_CON);
				}
				MQTTYield(&c, data.keepAliveInterval);
				break;
			
			case SOCK_CLOSE_WAIT:
				// MQTT socket disconnect
				Network_disconnect(&n);
				break;
			
			case SOCK_FIN_WAIT:
			case SOCK_CLOSED:
				source_port = get_client_any_port();
#ifdef _MQTTHANDLER_DEBUG_
				printf("\r\n");
				printf(" > MQTT: Client source port = %d\r\n", source_port);
#endif		
				if(socket(n.my_socket, Sn_MR_TCP, source_port, 0) == n.my_socket)
				{
					prev_time = get_mqtt_time();
					if(!(isSocketOpen_MQTTclient))
					{
#ifdef _MQTTHANDLER_DEBUG_
						printf(" > MQTT: TCP Sockopen\r\n");
#endif
						isSocketOpen_MQTTclient = ON;
					}
				}
				break;
				
			default:
				break;
		}
	}
	else
	{
		ret = MQTTERR_INIT_FAILED;
	}
	
	return ret;
}


// Time counter function for MQTT timer
// This function should be located in Timer IRQ handler (Have to call this function every millisecond)
void mqtt_timer_msec(void) {
	MilliTimer++; // mqtt timer
}

uint32_t get_mqtt_time(void) {
	return MilliTimer;
}

char expired(Timer* timer) {
	long left = timer->end_time - MilliTimer;
	return (left < 0);
}


void countdown_ms(Timer* timer, unsigned int timeout) {
	timer->end_time = MilliTimer + timeout;
}


void countdown(Timer* timer, unsigned int timeout) {
	timer->end_time = MilliTimer + (timeout * 1000);
}


int left_ms(Timer* timer) {
	long left = timer->end_time - MilliTimer;
	return (left < 0) ? 0 : left;
}


void InitTimer(Timer* timer) {
	timer->end_time = 0;
}


/* Private functions ---------------------------------------------------------*/
void NewNetwork(uint8_t s, Network* n) {
	n->my_socket = s;
	n->mqttread = Network_read;
	n->mqttwrite = Network_write;
	n->disconnect = Network_disconnect;
}


int ConnectNetwork(Network* n, uint8_t* ip, uint16_t port)
{
	int8_t ret;
	socket(n->my_socket, Sn_MR_TCP, get_client_any_port(), 0);
	ret = connect(n->my_socket, ip, port);
	
	return ret;
}


int Network_read(Network* n, unsigned char* buffer, int len, int timeout_ms)
{

	if((getSn_SR(n->my_socket) == SOCK_ESTABLISHED) && (getSn_RX_RSR(n->my_socket)>0))
	{
		return recv(n->my_socket, buffer, len);
	}
	
	return SOCK_ERROR;
}

int Network_write(Network* n, unsigned char* buffer, int len, int timeout_ms)
{
	if(getSn_SR(n->my_socket) == SOCK_ESTABLISHED)
	{
		return send(n->my_socket, buffer, len);
	}
	
	return SOCK_ERROR;
}

void Network_disconnect(Network* n)
{
	disconnect(n->my_socket);
}

uint16_t get_client_any_port(void)
{
	if(client_any_port)
	{
		if(client_any_port < 0xffff) 	client_any_port++;
		else							client_any_port = 0;
	}
	
	if(client_any_port == 0)
	{
		srand(1);
		client_any_port = (rand() % 10000) + 45000; // 45000 ~ 54999
	}
	
	return client_any_port;
}
