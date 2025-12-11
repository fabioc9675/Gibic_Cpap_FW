#ifndef WIFI_DRV_H
#define WIFI_DRV_H

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif_net_stack.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "lwip/inet.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_system.h"

#include "esp_err.h"

esp_err_t wifi_init_ap(void);
esp_err_t wifi_stop(void);

// /**
//   * @brief  Disconnect from the currently connected Wi-Fi network.
//   * @return
//   *   - ESP_OK on success
//   */
// esp_err_t wifi_shutdown(void);

// /**
//   * @brief  configuracion y conexionWi-Fi network.
//   * @param 
//   *   - wlan: the name of the Wi-Fi network to connect
//   *   - pwd: the password of the Wi-Fi network to connect
//   * @return
//   *   - ESP_OK on success
//   *   - ESP_FAIL en caso de fallo
//   */
// esp_err_t wifi_connect(char *wlan, char *pwd);

// /**
//   * @brief  Get the status of the Wi-Fi network.
//   * @return
//   *   - ESP_OK on success
//   *   - ESP_FAIL en caso de fallo
//   */
// esp_err_t wifi_status();

// /**
//   * @brief  Get the status of the Wi-Fi network.
//   * @return
//   *   - ESP_OK on success
//   *   - ESP_FAIL en caso de fallo
//   */
// esp_err_t wifi_reconnect(void);

// /**
//   * @brief  Get the MAC address of the Wi-Fi network.
//   * @param 
//   *   - mac: pointer to the MAC address or serial number
//   *   - type: 1 for MAC address, 12 char
//   *           0 for serial number, 6 char
//   */
// void wifi_get_mac(uint8_t *mac, uint8_t type);




#endif // WIFI_DRV_H
