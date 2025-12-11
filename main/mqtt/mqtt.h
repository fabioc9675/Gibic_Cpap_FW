#ifndef __MQTT_H__
#define __MQTT_H__

#include <stdint.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"

#include "wifi/wifiDrv.h"




/**
  * @brief  Initialize the variables for the MQTT client and the Wi-Fi network.
  *         in te future must include the credentials for the broker.
  * @return
  *   - ESP_OK on success
  */
void mqtt_initVars(char *wlan, char *pwd, char *broker, uint32_t port);

void send_mqtt_message(char *topic, char *message);
void MSmqtt(void);

typedef enum{
    stConnectWifi,
    stReconnectWifi,
    stConnectMqtt,
    stSendInit,
    stSendRecord,
    stSendEnd
}msMqtt;

#endif // __MQTT_H__