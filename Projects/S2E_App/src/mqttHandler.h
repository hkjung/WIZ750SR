/* 
 * mqttHandler.h
 * Paho MQTT Client for WIZnet W7500 Interface
 * 
 * http://www.eclipse.org/paho/
 */

#ifndef MQTTHANDLER_H_
#define MQTTHANDLER_H_

#include <stdint.h>

#define _MQTTHANDLER_DEBUG_

#define MQTT_TIMEOUT_MS                1000 // unit: ms
#define MQTT_RECONNECTION_INTERVAL_MS  3000 // unit: ms

#define MQTT_CONNECTED          2
#define MQTT_OK                 1        ///< Result is OK about MQTT client process.
#define MQTT_BUSY               0        ///< MQTT client is busy on processing the operation. Not used. (for non-blocking IO mode)
#define MQTT_ERROR              -1
#define MQTTERR_INIT_FAILED     (MQTT_ERROR - 1)     ///< MQTT client init failed
#define MQTTERR_CONNECT_FAILED  (MQTT_ERROR - 2)     ///< MQTT client connection failed

typedef struct Timer Timer;

struct Timer {
	unsigned long systick_period;
	unsigned long end_time;
};

typedef struct Network Network;

struct Network
{
	int my_socket;
	int (*mqttread) (Network*, unsigned char*, int, int);
	int (*mqttwrite) (Network*, unsigned char*, int, int);
	void (*disconnect) (Network*);
};

void MQTT_Init(uint8_t s, uint8_t *bip, uint16_t bport);
int8_t MQTT_Connect(void);
int8_t MQTT_Loop(void);

void mqtt_timer_msec(void);
uint32_t get_mqtt_time(void);

void InitTimer(Timer*);
char expired(Timer*);
void countdown_ms(Timer*, unsigned int);
void countdown(Timer*, unsigned int);
int left_ms(Timer*);

void NewNetwork(uint8_t s, Network* n);
int ConnectNetwork(Network*, uint8_t*, uint16_t);
int Network_read(Network*, unsigned char*, int, int);
int Network_write(Network*, unsigned char*, int, int);
void Network_disconnect(Network*);

#endif
