#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wwrite-strings"

#include "main.h"

#include "nvs.h"
#include "nvs_flash.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"

#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_netif.h"
#include "esp_eth.h"
#include "esp_event.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include <esp_system.h>

#include "driver/gpio.h"
#include "esp_eth_enc28j60.h"
#include "driver/spi_master.h"
#include "lwip/ip4_addr.h"
#include <esp_http_server.h>
#include "driver/uart.h"
#include "driver/i2c.h"
#include "driver/gptimer.h"
#include "esp_timer.h"

#include "menu.h"
#include "SSD1306.h"
#include "Font5x8.h"
#include "esp32_saric_mqtt_network.h"
#include <time.h>
#include "ezButton.h"

#include "Filter.h"
#include "saric_energy.h"
#include "saric_metrics.h"

#include "cJSON.h"


i2c_port_t i2c_num;


uint32_t uptime = 0;
uint32_t _millis = 0;

float internal_temp;

static EventGroupHandle_t s_wifi_event_group;
uint8_t s_wifi_retry_num = 0;

char *TAG = "energy-monitor";
static httpd_handle_t start_webserver(void);
static esp_err_t prom_metrics_get_handler(httpd_req_t *req);

esp_mqtt_client_handle_t mqtt_client;

esp_netif_t *eth_netif = NULL;
esp_netif_t *wifi_netif = NULL;

uint32_t eth_link = 0;
uint32_t wifi_link = 0;
uint32_t mqtt_link = 0;

uint8_t wifi_rssi = 0;
uint16_t free_heap = 0;

ezButton button_up(BUTTON_UP);
ezButton button_down(BUTTON_DOWN);
ezButton button_enter(BUTTON_ENTER);
ezButton generic_input(GENERIC_INPUT);
ezButton S0_contact(S0_CONTACK);
ezButton CrossDetector(CROSS_DETECTOR);


ExponentialFilter<long> ADCFilter1(1, 500);
ExponentialFilter<long> ADCFilter2(1, 500);
ExponentialFilter<long> ADCFilter3(1, 500);
ExponentialFilter<long> ADCFilter4(1, 500);
ExponentialFilter<long> ADCFilter5(1, 500);
ExponentialFilter<long> ADCFilter6(1, 500);

adc_cali_handle_t adc1_cali_handle = NULL;
adc_oneshot_unit_handle_t adc1_handle;

void nop(void)
{
}




esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id) {
    case HTTP_EVENT_ERROR:
        ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
        break;
    case HTTP_EVENT_REDIRECT:
        ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
        break;
    }
    return ESP_OK;
}


static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%ld", base, event_id);
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t) event_data;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
	mqtt_reconnect();
	mqtt_link = MQTT_EVENT_CONNECTED;
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
	mqtt_link = MQTT_EVENT_DISCONNECTED;
        break;
    case MQTT_EVENT_SUBSCRIBED:
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
	mqtt_callback((char *) event->topic, (uint8_t *)event->data, event->topic_len, event->data_len);
        break;
    case MQTT_EVENT_ERROR:
	mqtt_link = MQTT_EVENT_ERROR;
        ESP_LOGE(TAG, "MQTT_EVENT_ERROR, type %d", event->error_handle->error_type);
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}




/** Event handler for Ethernet events */
static void eth_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    uint8_t mac_addr[6] = {0};
    /* we can get the ethernet driver handle from event data */
    esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;
    if (event_id ==  ETHERNET_EVENT_CONNECTED)
    {
        esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
        ESP_LOGI(TAG, "Ethernet Link Up");
        ESP_LOGD(TAG, "Ethernet HW Addr %02x:%02x:%02x:%02x:%02x:%02x",
                 mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
	eth_link = ETHERNET_EVENT_CONNECTED;
    }
    if (event_id == ETHERNET_EVENT_DISCONNECTED)
    {
        ESP_LOGI(TAG, "Ethernet Link Down");
	eth_link = ETHERNET_EVENT_DISCONNECTED;
    }
    if (event_id == ETHERNET_EVENT_START)
    {
        ESP_LOGI(TAG, "Ethernet Started");
    }
    if (event_id == ETHERNET_EVENT_STOP)
    {
        ESP_LOGI(TAG, "Ethernet Stopped");
    }
}



static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
/*
    if (event_id == WIFI_EVENT_STA_START)
       ESP_LOGI(TAG, "WIFI EVENT STA START");
    if (event_id == WIFI_EVENT_WIFI_READY)
       ESP_LOGI(TAG, "WIFI EVENT WIFI READY");
    if (event_id ==  WIFI_EVENT_STA_STOP)
       ESP_LOGI(TAG,"WIFI EVENT STA STOP");
    if (event_id ==  WIFI_EVENT_STA_CONNECTED)
       ESP_LOGI(TAG,"WIFI EVENT STA CONNECTED");
    if (event_id ==  WIFI_EVENT_STA_DISCONNECTED)
       ESP_LOGI(TAG,"WIFI EVENT STA DISCONNECTED");
    if (event_id ==  WIFI_EVENT_STA_AUTHMODE_CHANGE)
       ESP_LOGI(TAG, "WIFI EVENT STA AUTHMODE CHANGE");
*/
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_wifi_retry_num < 5) {
            esp_wifi_connect();
            s_wifi_retry_num++;
            ESP_LOGI(TAG, "retry %d to connect to the AP", s_wifi_retry_num);
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
	    wifi_link = WIFI_EVENT_STA_DISCONNECTED;
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_wifi_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
	wifi_link = WIFI_EVENT_STA_CONNECTED;
    }
}



/* An HTTP GET handler */
static esp_err_t prom_metrics_get_handler(httpd_req_t *req)
{ 
    char resp_str[2048];
    strcpy(resp_str, "");
    httpd_resp_set_type(req, (const char*) "text/plain");

    prom_metric_up(resp_str);
    prom_metric_device_status(resp_str);
    prom_metric_energy_status(resp_str);
    httpd_resp_send_chunk(req, resp_str, HTTPD_RESP_USE_STRLEN);

    strcpy(resp_str, "");
    prom_metric_energy_group_status(resp_str);
    httpd_resp_send_chunk(req, resp_str, HTTPD_RESP_USE_STRLEN);

    strcpy(resp_str, "");
    httpd_resp_send_chunk(req, resp_str, 0);

    return ESP_OK;
}


/* An HTTP GET handler */
static esp_err_t configurations_get_handler(httpd_req_t *req)
{
    char resp_str[2048];
    strcpy(resp_str, "");
    json_device_config(resp_str);
    httpd_resp_set_type(req, (const char*) "application/json");
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

/* An HTTP POST handler */
static esp_err_t configurations_post_handler(httpd_req_t *req)
{
    cJSON *root; 
    char resp_str[2048];
    size_t recv_size;
    int ret;
    char str_idx[4];
    recv_size = MIN(req->content_len, sizeof(resp_str));
    ret = httpd_req_recv(req, resp_str, recv_size);
    if (ret <= 0) 
        {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) 
	    {
            httpd_resp_send_408(req);
	    }
	return ESP_FAIL;
        }

    root = cJSON_Parse(resp_str);
    //char *json_text = cJSON_Print(root);
    
    cJSON *json_network = NULL;
    cJSON *json_energy = NULL;
    cJSON *tmp_energy = NULL;
    json_network = cJSON_GetObjectItem(root, "network");
    json_energy = cJSON_GetObjectItem(root, "energy");

    if (json_network)
        {
        ret = setting_network_json(json_network);
        save_setup_network();
        }

    if (json_energy)
        {
        for (uint8_t idx = 0; idx < MAX_ADC_INPUT; idx++)
	    {
            itoa(idx, str_idx, 10);
	    tmp_energy = cJSON_GetObjectItem(json_energy, str_idx);
	    if (tmp_energy)
	        {
	        energy_set_all_from_json(idx, tmp_energy);
		}
	    }
        }

    cJSON_Delete(root);
    httpd_resp_sendstr(req, "OK\n");
    return ESP_OK;
}




static const httpd_uri_t prom_metrics = {
    .uri       = "/metrics",
    .method    = HTTP_GET,
    .handler   = prom_metrics_get_handler,
    .user_ctx  = 0
};


static const httpd_uri_t get_configurations = {
    .uri       = "/conf",
    .method    = HTTP_GET,
    .handler   = configurations_get_handler,
    .user_ctx  = 0
};


static const httpd_uri_t post_configurations = {
        .uri = "/conf",
        .method = HTTP_POST,
        .handler = configurations_post_handler,
        .user_ctx = 0
    };


 
static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 16000;
    config.lru_purge_enable = true;
    config.server_port = device.http_port;
    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", device.http_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &prom_metrics);
        httpd_register_uri_handler(server, &get_configurations);
        httpd_register_uri_handler(server, &post_configurations);
        return server;
    }
    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}







/** Event handler for IP_EVENT_ETH_GOT_IP */
static void got_ip_event_handler(void *arg, esp_event_base_t event_base,
                                 int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
    const esp_netif_ip_info_t *ip_info = &event->ip_info;

    ESP_LOGI(TAG, "Ethernet Got IP Address");
    ESP_LOGI(TAG, "~~~~~~~~~~~");
    ESP_LOGI(TAG, "ETHIP:" IPSTR, IP2STR(&ip_info->ip));
    ESP_LOGI(TAG, "ETHMASK:" IPSTR, IP2STR(&ip_info->netmask));
    ESP_LOGI(TAG, "ETHGW:" IPSTR, IP2STR(&ip_info->gw));
    ESP_LOGI(TAG, "~~~~~~~~~~~");
}


void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "Notification of a time synchronization event");
}



void setup(void)
{
  esp_eth_handle_t eth_handle = NULL;
  //static httpd_handle_t http_server = NULL;

  esp_mqtt_client_config_t mqtt_cfg = {};
  char hostname[10];

  char ip_uri[20];
  char complete_ip_uri[32];

  esp_event_handler_instance_t instance_any_id;
  esp_event_handler_instance_t instance_got_ip;

  twi_init(I2C_NUM_0);



  for (uint8_t init = 0;  init < 13; init++)
  {
    if (init == 0)
      {
	for (uint8_t idx = 0; idx < MAX_MENUS ;idx++)
	  {
	  menu_counter[idx] = 0;
          menu_params_1[idx] = 0;
          menu_params_2[idx] = 0;
	  }
        GLCD_Setup();
        GLCD_SetFont(Font5x8, 5, 8, GLCD_Overwrite);
        GLCD_Clear();
        GLCD_GotoXY(0, 0);
        GLCD_PrintString("... init ...");
	GLCD_GotoXY(0, 16);
        GLCD_PrintString("0. i2c bus");
        GLCD_Render();
      }
    ///  
    if (init == 1)
      {
      if (EEPROM.read(set_default_values) == 255)
         {
         ESP_LOGI(TAG, "Default values");
         EEPROM.write(set_default_values, 0);
         device.mac[0] = 2; device.mac[1] = 1; device.mac[2] = 2; device.mac[3] = 24; device.mac[4] = 25; device.mac[5] = 27;
         device.myIP[0] = 192; device.myIP[1] = 168; device.myIP[2] = 1; device.myIP[3] = 24;
         device.myMASK[0] = 255; device.myMASK[1] = 255; device.myMASK[2] = 255; device.myMASK[3] = 0;

         device.myGW[0] = 192; device.myGW[1] = 168; device.myGW[2] = 1; device.myGW[3] = 1;
         device.myDNS[0] = 192; device.myDNS[1] = 168; device.myDNS[2] = 1; device.myDNS[3] = 1;
         device.mqtt_server[0] = 192; device.mqtt_server[1] = 168; device.mqtt_server[2] = 2; device.mqtt_server[3] = 1;
         device.ntp_server[0] = 192; device.ntp_server[1] = 168; device.ntp_server[2] = 2; device.ntp_server[3] = 1;
         device.mqtt_port = 1883;
         device.http_port = 80;
	 device.via = 1 << ENABLE_CONNECT_ETH | 1 << ENABLE_CONNECT_WIFI;
	 strcpy(device.nazev, "MONITOR1");
         strcpy(device.mqtt_user, "saric");
         strcpy(device.mqtt_key, "no");
	 strcpy(device.bootloader_uri, "http://192.168.2.1/monitor/v3.bin");

	 strcpy(device.wifi_essid, "saric2g");
         strcpy(device.mqtt_key, "sarickey");
         /// po initu je vzdy dhcp aktivni
	 device.dhcp = DHCP_ENABLE;

	 save_setup_network();
         load_setup_network();
         strcpy(hostname, device.nazev);

	 strcpy(complete_ip_uri, "");
         createString(ip_uri, '.', device.mac, 6, 16, 1);
         strcat(complete_ip_uri, ip_uri);
         ESP_LOGI(TAG, "mac address: %s", complete_ip_uri);

	 strcpy(complete_ip_uri, "");
	 createString(ip_uri, '.', device.myIP, 4, 10, 1);
         strcat(complete_ip_uri, ip_uri);
         ESP_LOGI(TAG, "ip address: %s", complete_ip_uri);

	 strcpy(complete_ip_uri, "");
	 createString(ip_uri, '.', device.myMASK, 4, 10, 1);
         strcat(complete_ip_uri, ip_uri);
         ESP_LOGI(TAG, "netmask : %s", complete_ip_uri);

	 strcpy(complete_ip_uri, "");
	 createString(ip_uri, '.', device.myGW, 4, 10, 1);
         strcat(complete_ip_uri, ip_uri);
         ESP_LOGI(TAG, "default gateway: %s", complete_ip_uri);

	 strcpy(complete_ip_uri, "");
	 createString(ip_uri, '.', device.myDNS, 4, 10, 1);
         strcat(complete_ip_uri, ip_uri);
         ESP_LOGI(TAG, "dns server: %s", complete_ip_uri);

	 strcpy(complete_ip_uri, "");
	 strcpy(complete_ip_uri, "mqtt://");
         createString(ip_uri, '.', device.mqtt_server, 4, 10, 1);
         strcat(complete_ip_uri, ip_uri);
         ESP_LOGI(TAG, "mqtt uri: %s", complete_ip_uri);

         ESP_LOGI(TAG, "hostname: %s", hostname);

	 ESP_LOGI(TAG, "mqtt_user: %s", device.mqtt_user);

	 ESP_LOGI(TAG, "mqtt_key: %s", device.mqtt_key);

	 ESP_LOGI(TAG, "mqtt_port: %d", device.mqtt_port);

	 energy_struct_default_init();

	 GLCD_SetFont(Font5x8, 5, 8, GLCD_Overwrite);
         GLCD_Clear();
         GLCD_GotoXY(0, 0);
         GLCD_PrintString(".. init ...");
         GLCD_GotoXY(0, 16);
         GLCD_PrintString("1. update default");
         GLCD_Render();
         }
      else
	 {
	 ESP_LOGI(TAG, "Default values - skip");
	 GLCD_SetFont(Font5x8, 5, 8, GLCD_Overwrite);
         GLCD_Clear();
         GLCD_GotoXY(0, 0);
         GLCD_PrintString(".. init ...");
         GLCD_GotoXY(0, 16);
         GLCD_PrintString("1. default skip");
         GLCD_Render();
	 }	
      }

    if (init == 2)
      {
        uart_config_t uart_config = {
          .baud_rate = 38400,
          .data_bits = UART_DATA_8_BITS,
          .parity    = UART_PARITY_DISABLE,
          .stop_bits = UART_STOP_BITS_1,
          .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
          .rx_flow_ctrl_thresh = 122,
          .source_clk = UART_SCLK_DEFAULT,
          };

        QueueHandle_t uart_queue;
        ESP_ERROR_CHECK(uart_param_config(UART_NUM_2, &uart_config));
        ESP_ERROR_CHECK(uart_set_pin(UART_NUM_2, RS_TXD, RS_RXD, RS_RTS, RS_CTS));
        ESP_ERROR_CHECK(uart_driver_install(UART_NUM_2, 1024  , 1024, 10, &uart_queue, 0));
        ESP_ERROR_CHECK(uart_set_mode(UART_NUM_2, UART_MODE_RS485_HALF_DUPLEX));

        GLCD_SetFont(Font5x8, 5, 8, GLCD_Overwrite);
        GLCD_Clear();
        GLCD_GotoXY(0, 0);
        GLCD_PrintString(".. init ...");
        GLCD_GotoXY(0, 16);
        GLCD_PrintString("2. rs485 bus");
        GLCD_Render();
      }
    if (init == 3)
       {
	load_setup_network(); 
        GLCD_SetFont(Font5x8, 5, 8, GLCD_Overwrite);
        GLCD_Clear();
        GLCD_GotoXY(0, 0);
        GLCD_PrintString(".. init ...");
        GLCD_GotoXY(0, 16);
        GLCD_PrintString("3. EEPROM config");
        GLCD_Render();
       }

    ///
    //
    if (init == 4)
      {
      ESP_ERROR_CHECK(esp_netif_init());
      ESP_ERROR_CHECK(esp_event_loop_create_default());
      GLCD_SetFont(Font5x8, 5, 8, GLCD_Overwrite);
      GLCD_Clear();
      GLCD_GotoXY(0, 0);
      GLCD_PrintString(".. init ...");
      GLCD_GotoXY(0, 16);
      GLCD_PrintString("4. Network netif ");
      GLCD_Render();
      }
    ///
    if (init == 5)
      {
      //Initialize NVS
      esp_err_t ret = nvs_flash_init();
      if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) 
        {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
        }
      ESP_ERROR_CHECK(ret);

      ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");

      s_wifi_event_group = xEventGroupCreate();

      wifi_netif = esp_netif_create_default_wifi_sta();

      wifi_init_config_t wifi_netif_conf = WIFI_INIT_CONFIG_DEFAULT();
      ESP_ERROR_CHECK(esp_wifi_init(&wifi_netif_conf));

      wifi_config_t wifi_config = {
          .sta = {
              .ssid = "",
              .password = "",
          },
      };

      for (uint8_t idx = 0; idx < strlen(device.wifi_essid); idx++)
          wifi_config.sta.ssid[idx] = device.wifi_essid[idx];

      for (uint8_t idx = 0; idx < strlen(device.wifi_key); idx++)
          wifi_config.sta.password[idx] = device.wifi_key[idx];

      ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
      ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );

      ESP_LOGI(TAG, "wifi_init_sta finished.");

      GLCD_SetFont(Font5x8, 5, 8, GLCD_Overwrite);
      GLCD_Clear();
      GLCD_GotoXY(0, 0);
      GLCD_PrintString(".. init ...");
      GLCD_GotoXY(0, 10);
      GLCD_PrintString("5. wifi init done");
      GLCD_Render();
      }
    ///
    if (init == 6)
      {
      ESP_ERROR_CHECK(gpio_install_isr_service(0));
      esp_netif_config_t eth_netif_conf = ESP_NETIF_DEFAULT_ETH();
      eth_netif = esp_netif_new(&eth_netif_conf);
         
      spi_bus_config_t buscfg = {
          .mosi_io_num = ENC28J60_MOSI_GPIO,
          .miso_io_num = ENC28J60_MISO_GPIO,
          .sclk_io_num = ENC28J60_SCLK_GPIO,
          .quadwp_io_num = -1,
          .quadhd_io_num = -1
      };
     
      ESP_ERROR_CHECK(spi_bus_initialize(HSPI_HOST, &buscfg, SPI_DMA_CH_AUTO));
      /* ENC28J60 ethernet driver is based on spi driver */
      spi_device_interface_config_t devcfg  = {
          .command_bits = 3,
          .address_bits = 5,
          .mode = 0,
          .cs_ena_pretrans = 0,
          .cs_ena_posttrans = enc28j60_cal_spi_cs_hold_time(ENC28J60_SPI_CLOCK_MHZ),
          .clock_speed_hz = ENC28J60_SPI_CLOCK_MHZ * 1000 * 1000,
          .spics_io_num = ENC28J60_CS_GPIO,
          .queue_size = 20,
      };
    
      spi_device_handle_t spi_handle = NULL;
      ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &devcfg, &spi_handle));

      eth_enc28j60_config_t enc28j60_config = ETH_ENC28J60_DEFAULT_CONFIG(SPI2_HOST, &devcfg);

      enc28j60_config.int_gpio_num = ENC28J60_INT_GPIO;

      eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
      esp_eth_mac_t *mac = esp_eth_mac_new_enc28j60(&enc28j60_config, &mac_config);

      eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
      phy_config.autonego_timeout_ms = 0; // ENC28J60 doesn't support auto-negotiation
      phy_config.reset_gpio_num = ENC28J60_RESET; // ENC28J60 doesn't have a pin to reset internal PHY
      esp_eth_phy_t *phy = esp_eth_phy_new_enc28j60(&phy_config);

      esp_eth_config_t eth_config = ETH_DEFAULT_CONFIG(mac, phy);
      ESP_ERROR_CHECK(esp_eth_driver_install(&eth_config, &eth_handle));

      /* ENC28J60 doesn't burn any factory MAC address, we need to set it manually.
      */
      mac->set_addr(mac, device.mac);

      // ENC28J60 Errata #1 check
      if (emac_enc28j60_get_chip_info(mac) < ENC28J60_REV_B5 && ENC28J60_SPI_CLOCK_MHZ < 8) {
          ESP_LOGE(TAG, "SPI frequency must be at least 8 MHz for chip revision less than 5");
          ESP_ERROR_CHECK(ESP_FAIL);
      }

      GLCD_SetFont(Font5x8, 5, 8, GLCD_Overwrite);
      GLCD_Clear();
      GLCD_GotoXY(0, 0);
      GLCD_PrintString(".. init ...");
      GLCD_GotoXY(0, 10);
      GLCD_PrintString("6. ethernet bus");
      GLCD_Render();
      }


    if (init == 7)
      {
      ESP_ERROR_CHECK(esp_netif_set_hostname(eth_netif, device.nazev));
      ESP_ERROR_CHECK(esp_netif_set_hostname(wifi_netif, device.nazev));
      if (device.dhcp == DHCP_ENABLE)
	  ESP_LOGI(TAG, "DHCP client enable");
      if (device.dhcp == DHCP_DISABLE)
	  {
          ESP_LOGI(TAG, "DHCP client disable");
          if ((device.via & (ENABLE_CONNECT_ETH)) != 0)
	      {
              ESP_ERROR_CHECK(esp_netif_dhcpc_stop(eth_netif));
	      ESP_LOGI(TAG, "DHCP client for Ethernet not enabled");
	      }
	  if ((device.via & (ENABLE_CONNECT_WIFI)) != 0)
	     {
	     ESP_ERROR_CHECK(esp_netif_dhcpc_stop(wifi_netif));
	     ESP_LOGI(TAG, "DHCP client for Wifi not enabled");
	     }
          
          esp_netif_ip_info_t ip_info;
          IP4_ADDR(&ip_info.ip, device.myIP[0], device.myIP[1], device.myIP[2], device.myIP[3]);
          IP4_ADDR(&ip_info.gw, device.myGW[0], device.myGW[1], device.myGW[2], device.myGW[3]);
          IP4_ADDR(&ip_info.netmask, device.myMASK[0], device.myMASK[1], device.myMASK[2], device.myMASK[3]);
          esp_netif_set_ip_info(eth_netif, &ip_info);
          esp_netif_set_ip_info(wifi_netif, &ip_info);

          ESP_LOGI(TAG, "Ethernet SET IP Address");
          ESP_LOGI(TAG, "~~~~~~~~~~~");
          ESP_LOGI(TAG, "set ETHIP:" IPSTR, IP2STR(&ip_info.ip));
          ESP_LOGI(TAG, "set ETHMASK:" IPSTR, IP2STR(&ip_info.netmask));
          ESP_LOGI(TAG, "set ETHGW:" IPSTR, IP2STR(&ip_info.gw));
          ESP_LOGI(TAG, "~~~~~~~~~~~");
          }

      /* attach Ethernet driver to TCP/IP stack */
      ESP_ERROR_CHECK(esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handle)));

      // Register user defined event handers
      ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL));
      ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL));

      eth_duplex_t duplex = ETH_DUPLEX_FULL;
      ESP_ERROR_CHECK(esp_eth_ioctl(eth_handle, ETH_CMD_S_DUPLEX_MODE, &duplex));

      /* start Ethernet driver state machine */
      if ((device.via & (ENABLE_CONNECT_ETH)) != 0)
	 {
         ESP_ERROR_CHECK(esp_eth_start(eth_handle));
	 ESP_LOGI(TAG, "Started Ethernet interface");
         }
      else
	 {
	 ESP_LOGI(TAG, "Ethernet interface disabled");
	 }


      if ((device.via & (ENABLE_CONNECT_WIFI)) != 0)
         {
	  ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id));
          ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &instance_got_ip));
          ESP_ERROR_CHECK(esp_wifi_start());
          ESP_LOGI(TAG, "Starting Wifi interface");
	  ESP_LOGI(TAG, "Connecting to AP");
          /// cekam maximalne 1minutu
          const TickType_t xTicksToWait = 60000 / portTICK_PERIOD_MS;
          EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, xTicksToWait);

          if (bits & WIFI_CONNECTED_BIT) 
	     {
             ESP_LOGI(TAG, "connected to ap SSID: %s", device.wifi_essid);
             } 
	  else
             { 
             if (bits & WIFI_FAIL_BIT) 
	         {
                 ESP_LOGE(TAG, "Failed to connect to SSID: %s, password: %s", device.wifi_essid, device.wifi_key);
                 } 
	     else 
	         {
                 ESP_LOGW(TAG, "UNEXPECTED EVENT");
                 }
	     }
         }
      else
	 {
	 ESP_LOGI(TAG, "Wifi interface disabled");
	 }

      GLCD_SetFont(Font5x8, 5, 8, GLCD_Overwrite);
      GLCD_Clear();
      GLCD_GotoXY(0, 0);
      GLCD_PrintString(".. init ...");
      GLCD_GotoXY(0, 10);
      GLCD_PrintString("7. ip stack");
      
      GLCD_GotoXY(0, 22);
      vTaskDelay(2000 / portTICK_PERIOD_MS);
      if (eth_link == ETHERNET_EVENT_CONNECTED)
	{
        GLCD_PrintString("link up");
	ESP_LOGI(TAG, "ethernet link up");
	}
      else
	{
        GLCD_PrintString("link down");
	ESP_LOGI(TAG, "ethernet link down");
	}

      if (wifi_link == WIFI_EVENT_STA_CONNECTED)
	{
        ESP_LOGI(TAG, "wifi link up");
        }
      else
	{
        ESP_LOGI(TAG, "wifi link down");
        }
      GLCD_Render();
      }

  if (init == 8)
      {
      ESP_LOGI(TAG, "Starting webserver");
      start_webserver();
      GLCD_SetFont(Font5x8, 5, 8, GLCD_Overwrite);
      GLCD_Clear();
      GLCD_GotoXY(0, 0);
      GLCD_PrintString(".. init ...");
      GLCD_GotoXY(0, 16);
      GLCD_PrintString("8. http server");
      GLCD_Render();
     }	  

  if (init == 9)
     {
      GLCD_SetFont(Font5x8, 5, 8, GLCD_Overwrite);
      GLCD_Clear();
      GLCD_GotoXY(0, 0);
      GLCD_PrintString(".. init ...");
      GLCD_GotoXY(0, 11);
      GLCD_PrintString("9. MQTT start");
      GLCD_Render();

      send_mqtt_set_header(monitor_header_out);
      strcpy(complete_ip_uri, "mqtt://");
      createString(ip_uri, '.', device.mqtt_server, 4, 10, 1);
      strcat(complete_ip_uri, ip_uri);
      ESP_LOGI(TAG, "mqtt_uri: %s", complete_ip_uri);

      mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
      esp_mqtt_client_set_uri(mqtt_client, complete_ip_uri);

      esp_mqtt_client_register_event(mqtt_client, (esp_mqtt_event_id_t) ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
      esp_mqtt_client_start(mqtt_client);


      ESP_LOGI(TAG, "mqtt client start");
      GLCD_SetFont(Font5x8, 5, 8, GLCD_Overwrite);
      GLCD_Clear();
      GLCD_GotoXY(0, 0);
      GLCD_PrintString(".. init ...");
      GLCD_GotoXY(0, 10);
      GLCD_PrintString("9. mqtt client");
      GLCD_GotoXY(0, 22);
      vTaskDelay(2000 / portTICK_PERIOD_MS);
      if (mqtt_link == MQTT_EVENT_CONNECTED)
         GLCD_PrintString("mqtt connected");
      if (mqtt_link == MQTT_EVENT_DISCONNECTED)
	{
        GLCD_PrintString("mqtt disconnected");
	ESP_LOGE(TAG, "mqtt disconnected");
	}
      if (mqtt_link == MQTT_EVENT_ERROR)
	{
        GLCD_PrintString("mqtt error");
	ESP_LOGE(TAG, "mqtt error");
	}
      GLCD_Render();
     }

  if (init == 10)
     {
      adc_oneshot_unit_init_cfg_t init_config1 = {
          .unit_id = ADC_UNIT_1,
      };
      ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

      adc_oneshot_chan_cfg_t config = {
          .atten = ADC_ATTEN_DB_11,
          .bitwidth = ADC_BITWIDTH_DEFAULT
      };
      
      ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_F1, &config));
      ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_F2, &config));
      ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_F3, &config));
      ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_F4, &config));
      ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_F5, &config));
      ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_F6, &config));

      adc_calibration_init(ADC_UNIT_1, ADC_ATTEN_DB_11, &adc1_cali_handle);

      energy_struct_init();

      GLCD_SetFont(Font5x8, 5, 8, GLCD_Overwrite);
      GLCD_Clear();
      GLCD_GotoXY(0, 0);
      GLCD_PrintString(".. init ...");
      GLCD_GotoXY(0, 10);
      GLCD_PrintString("10. ADC");
      GLCD_Render();
     }


  if (init == 11)
     {
     //zero-initialize the config structure.
     gpio_config_t io_conf = {};
     //disable interrupt
     io_conf.intr_type = GPIO_INTR_DISABLE;
     //set as output mode
     io_conf.mode = GPIO_MODE_OUTPUT;
     //bit mask of the pins that you want to set,e.g.GPIO18/19
     io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
     //disable pull-down mode
     io_conf.pull_down_en = (gpio_pulldown_t) 0;
     //disable pull-up mode
     io_conf.pull_up_en = (gpio_pullup_t) 1;
     //configure GPIO with the given settings
     gpio_config(&io_conf);

     //interrupt of rising edge
     io_conf.intr_type = GPIO_INTR_DISABLE;
     //bit mask of the pins, use GPIO4/5 here
     io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
     //set as input mode
     io_conf.mode = GPIO_MODE_INPUT;
     //enable pull-up mode
     io_conf.pull_up_en = (gpio_pullup_t) 1;
     gpio_config(&io_conf);

     gpio_set_level(RELAY_OUTPUT, 1);

     button_up.setDebounceTime(10);
     button_down.setDebounceTime(10);
     button_enter.setDebounceTime(10);
     generic_input.setDebounceTime(100);
     S0_contact.setDebounceTime(100);
     CrossDetector.setDebounceTime(1);

     GLCD_SetFont(Font5x8, 5, 8, GLCD_Overwrite);
     GLCD_Clear();
     GLCD_GotoXY(0, 0);
     GLCD_PrintString(".. init ...");
     GLCD_GotoXY(0, 11);
     GLCD_PrintString("11. GPIO");
     GLCD_Render();
     }

  if (init == 12)
     {
     GLCD_SetFont(Font5x8, 5, 8, GLCD_Overwrite);
     GLCD_Clear();
     GLCD_GotoXY(0, 0);
     GLCD_PrintString(".. init ...");
     GLCD_GotoXY(0, 11);
     GLCD_PrintString("12. NTP time start");
     GLCD_Render();

     sntp_setoperatingmode(SNTP_OPMODE_POLL);
     createString(ip_uri, '.', device.ntp_server, 4, 10, 1);
     sntp_setservername(0, ip_uri);
     sntp_set_sync_interval(360000);
     sntp_set_time_sync_notification_cb(time_sync_notification_cb);
     sntp_init();
     char strftime_buf[64];
     // wait for time to be set
     time_t now = 0;
     struct tm timeinfo = { 0 };
     int retry = 0;
     const int retry_count = 10;

     ESP_LOGI(TAG, "ntp server %s", sntp_getservername(0));

     while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
         ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
         vTaskDelay(1000 / portTICK_PERIOD_MS);
     }

     setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
     tzset();
     time(&now);
     localtime_r(&now, &timeinfo);
     strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
     ESP_LOGI(TAG, "The current date/time in Prague is: %s", strftime_buf);

     GLCD_SetFont(Font5x8, 5, 8, GLCD_Overwrite);
     GLCD_Clear();
     GLCD_GotoXY(0, 0);
     GLCD_PrintString(".. init ...");
     GLCD_GotoXY(0, 11);
     if (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET)
         GLCD_PrintString("12. NTP time - ERR");
     else
	 GLCD_PrintString("12. NTP time - OK");
     GLCD_Render();
     }
  }
}



bool adc_calibration_init(adc_unit_t unit, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif
#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Line Fitting");
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif
    *out_handle = handle;
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Calibration Success");
    } else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
        ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
    } else {
        ESP_LOGE(TAG, "Invalid arg or no memory");
    }
    return calibrated;
}






void adc_data(void)
{
     uint32_t avg;
     uint32_t vmin, vmax, current;
     uint32_t amper1 = 0;
     uint32_t amper2 = 0;
     uint32_t amper = 0;
     int voltage = 0; 
     int raw_voltage = 0;

     uint32_t diff_period = 0;

     struct_energy_t energyx;

     for (uint8_t idx = 0; idx < MAX_ADC_INPUT; idx++)
	{
        energy_get_parametr(idx, &energyx);
        if (energyx.used == 1)
	   {
	   if ((millis() - energyx.last_period) >= energyx.period && (millis() > energyx.last_period))
	      {
	      diff_period = millis() - energyx.last_period;
	      energyx.last_period = energyx.last_period + energyx.period;
              vmax = 0;
	      vmin = 65535;
	      avg = 0;
	      current = 0;
              for (uint8_t sample = 0; sample < energyx.samples; sample++)
	         {
		 for (uint8_t sam = 0; sam < 10; sam++)
		    {
	            switch (energyx.input)
	              {
	              case 1:
			adc_oneshot_read(adc1_handle, ADC_F1, &raw_voltage);
                        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_handle, raw_voltage, &voltage));
                        ADCFilter1.Filter(voltage);
			break;
	              case 2:
			adc_oneshot_read(adc1_handle, ADC_F2, &raw_voltage);
                        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_handle, raw_voltage, &voltage));
                        ADCFilter2.Filter(voltage);
			break;
	              case 3:
                        adc_oneshot_read(adc1_handle, ADC_F3, &raw_voltage);
                        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_handle, raw_voltage, &voltage));
                        ADCFilter3.Filter(voltage);
                        break;
	              case 4:
		        adc_oneshot_read(adc1_handle, ADC_F4, &raw_voltage);
                        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_handle, raw_voltage, &voltage));
                        ADCFilter4.Filter(voltage);
                        break;
	              case 5:
			adc_oneshot_read(adc1_handle, ADC_F5, &raw_voltage);
                        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_handle, raw_voltage, &voltage));
                        ADCFilter5.Filter(voltage);
                        break;
	              case 6:
			adc_oneshot_read(adc1_handle, ADC_F6, &raw_voltage);
                        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_handle, raw_voltage, &voltage));
                        ADCFilter6.Filter(voltage);
                        break;
	              default: break; 
	              }
	            }
		 switch (energyx.input)
		   {
		   case 1: current = ADCFilter1.Current(); break;
		   case 2: current = ADCFilter2.Current(); break;
		   case 3: current = ADCFilter3.Current(); break;
		   case 4: current = ADCFilter4.Current(); break;
		   case 5: current = ADCFilter5.Current(); break;
		   case 6: current = ADCFilter6.Current(); break;
		   default: break;
		   }
                 avg = avg + current;
                 if (current < vmin)
                    vmin = current;
                 if (current > vmax)
                    vmax = current;
		 }

	      avg = avg / energyx.samples;
	      amper1 = 0;
              amper2 = 0;
              if ((vmax-avg) > energyx.noise_limit && (avg-vmin) > energyx.noise_limit)
                 {
                 amper1 = (vmax-avg)*energyx.type;
                 amper2 = (avg-vmin)*energyx.type;
                 }
              amper = (amper1 + amper2) / 2;

              energyx.miliwatt = energyx.miliwatt + (uint16_t) (float)amper*230*((float)diff_period/1000/3600);
	      energyx.milisecond = energyx.milisecond + diff_period;

	      uint32_t w = energyx.miliwatt / 1000;
	      energyx.miliwatt = energyx.miliwatt - (w * 1000);

	      uint32_t s = energyx.milisecond / 1000;
              energyx.milisecond = energyx.milisecond - (s * 1000);

	      energyx.total_watt = energyx.total_watt + w;
              energyx.total_second = energyx.total_second + s;

	      energyx.current_now = amper;
	      energy_set_parametr(idx, energyx);
	      }
	   }
	}
     /// 1A = 33mV 
     /*
      * 260mA = 8mV
      *
      * 1mV ... 33mA
     */
}


uint32_t millis(void)
{
  return _millis;
}

void callback_1_milisec(void)
{
  _millis++;
}





void callback_1_sec(void* arg)
{
  ESP_LOGD(TAG, "timer 1 sec");
  uptime++;
  display_redraw = true;
}




void core0Task( void * pvParameters )
{
  struct timeval tv_now;

  char str1[32];
  char str2[32];
  uint8_t button_up_state = BUTTON_RELEASED;
  uint8_t button_down_state = BUTTON_RELEASED;
  uint8_t button_enter_state = BUTTON_RELEASED;

  uint8_t _button_up_state = BUTTON_RELEASED;
  uint8_t _button_down_state = BUTTON_RELEASED;
  uint8_t _button_enter_state = BUTTON_RELEASED;
  
  uint8_t _generic_input_state = BUTTON_RELEASED;
  uint8_t _S0_contact_state = BUTTON_RELEASED;
  uint8_t _CrossDetecot_state = BUTTON_RELEASED;


  uint32_t next_10s = uptime;
  uint32_t next_20s = uptime;
  uint32_t next_60s = uptime;
  uint32_t next_12h = uptime;

  while(1)
    {
    button_up.loop();
    button_down.loop();
    button_enter.loop();

    generic_input.loop();
    S0_contact.loop();
    CrossDetector.loop();

    if (generic_input.isPressed() && _generic_input_state == BUTTON_RELEASED)
       {
       _generic_input_state = BUTTON_PRESSED;
       send_mqtt_general_payload(mqtt_client, "input", "on"); 
       }

    if (generic_input.isReleased() && _generic_input_state == BUTTON_PRESSED)
       {
       _generic_input_state = BUTTON_RELEASED;
       send_mqtt_general_payload(mqtt_client, "input", "off");
       }

    if (S0_contact.isPressed() && _S0_contact_state == BUTTON_RELEASED)
       {
       _S0_contact_state = BUTTON_PRESSED;
       send_mqtt_general_payload(mqtt_client, "S0_contact", "on"); 
       }

    if (S0_contact.isReleased() && _S0_contact_state == BUTTON_PRESSED)
       {
       _S0_contact_state = BUTTON_RELEASED;
       send_mqtt_general_payload(mqtt_client, "S0_contact", "off");
       }




    if (button_up.isPressed() && _button_up_state == BUTTON_RELEASED)
       {
       _button_up_state = BUTTON_PRESSED;
       button_up_state = BUTTON_PRESSED;
       display_redraw = true;
       }

    if (button_down.isPressed() && _button_down_state == BUTTON_RELEASED) 
       {
       _button_down_state = BUTTON_PRESSED;
       button_down_state = BUTTON_PRESSED;
       display_redraw = true;
       }

    if (button_enter.isPressed() && _button_enter_state == BUTTON_RELEASED)
       {
       _button_enter_state = BUTTON_PRESSED; 
       button_enter_state = BUTTON_PRESSED; 
       display_redraw = true;
       }

    if (button_up.isReleased()) 
       {
       _button_up_state = BUTTON_RELEASED;
       button_up_state = BUTTON_RELEASED;
       display_redraw = true;
       }
    
    if (button_down.isReleased()) 
       {
       _button_down_state = BUTTON_RELEASED;
       button_down_state = BUTTON_RELEASED;
       display_redraw = true;
       }

    if (button_enter.isReleased()) 
       {
       _button_enter_state = BUTTON_RELEASED; 
       button_enter_state = BUTTON_RELEASED; 
       display_redraw = true;
       }




    display_redraw = display_menu(button_up_state, button_down_state, button_enter_state, display_redraw);
    button_up_state = BUTTON_RELEASED; 
    button_down_state = BUTTON_RELEASED; 
    button_enter_state = BUTTON_RELEASED; 

    if ((uptime - next_60s) > 60)
      {
      next_60s += 60;
      if (wifi_link == WIFI_EVENT_STA_DISCONNECTED)
         {
         s_wifi_retry_num = 0;
	 esp_wifi_connect();
	 }	     
      }

    if ((uptime - next_10s) > 10)
       {
       next_10s += 10;
       if (mqtt_link == MQTT_EVENT_CONNECTED)
          {
	  get_device_status();
          send_mqtt_status(mqtt_client);
          update_know_mqtt_device();
          send_know_device();
          send_device_status();
	  send_energy_status();
          }
       }   

    if ((uptime - next_20s) > 20)
       {
       next_20s += 20;
       strcpy_P(str2, monitor_subscribe);
       device_get_name(str1);
       if (mqtt_link == MQTT_EVENT_CONNECTED)
          {
          send_mqtt_payload(mqtt_client, str2, str1);
          }
       }

    if ((uptime - next_12h) > 43200)
       {
       next_12h += 43200;
       gettimeofday(&tv_now, NULL);
       energy_store_update_all_mess(tv_now.tv_sec);
       }
    }
}



void core1Task( void * pvParameters )
{
  while(1)
  {
     adc_data();
  }
}



static bool IRAM_ATTR timer_on_alarm_1ms(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_data)
{
  callback_1_milisec();
  return true;
}




extern "C" void app_main(void)
{
  esp_log_level_set("*", ESP_LOG_INFO); 

  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(err);

  get_sha256_of_partitions();


  const esp_timer_create_args_t periodic_timer_args_1_sec = {.callback = &callback_1_sec, .name = "1_sec" };

  setup();

  xTaskCreatePinnedToCore(core0Task, "core0Task", 10000, NULL, 0, NULL, 0);   
  xTaskCreatePinnedToCore(core1Task, "core1Task", 10000, NULL, 0, NULL, 1);
  
  
  esp_timer_handle_t periodic_timer_1_sec;
  esp_timer_create(&periodic_timer_args_1_sec, &periodic_timer_1_sec);
  esp_timer_start_periodic(periodic_timer_1_sec, 1000000);


  //timer_queue_element_t ele;
  QueueHandle_t timer_queue = xQueueCreate(10, sizeof(timer_queue_element_t));
  if (!timer_queue) 
  {
        ESP_LOGE(TAG, "Creating timer_queue failed");
        return;
  }

  gptimer_handle_t gptimer_1ms = NULL;
  gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_APB,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000, // 1MHz, 1 tick=1us
  };
  ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer_1ms));

  gptimer_event_callbacks_t timer_event_define_callback = {
        .on_alarm = timer_on_alarm_1ms,
    };
  ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer_1ms, &timer_event_define_callback, timer_queue));

  gptimer_alarm_config_t timer_alarm_config_1ms = {
        .alarm_count = 1000, // period = 1ms
	.reload_count = 0
    };
  timer_alarm_config_1ms.flags.auto_reload_on_alarm = true;

  ESP_LOGI(TAG, "Enable timer");
  ESP_ERROR_CHECK(gptimer_enable(gptimer_1ms));
  ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer_1ms, &timer_alarm_config_1ms));
  ESP_ERROR_CHECK(gptimer_start(gptimer_1ms));

}













esp_err_t twi_init(i2c_port_t t_i2c_num)
{
  #define I2C_MASTER_TX_BUF_DISABLE 0                           /*!< I2C master doesn't need buffer */
  #define I2C_MASTER_RX_BUF_DISABLE 0                           /*!< I2C master doesn't need buffer */
  esp_err_t ret;
  i2c_num = t_i2c_num;
  i2c_config_t conf;
  conf.mode = I2C_MODE_MASTER;
  conf.sda_io_num = I2C_MASTER_SDA_IO;
  conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
  conf.scl_io_num = I2C_MASTER_SCL_IO;
  conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
  conf.master.clk_speed = 400000L;
  conf.clk_flags = 0;
  i2c_param_config(i2c_num, &conf);
  ret = i2c_driver_install(i2c_num, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
  if (!ret)
	  ESP_LOGI(TAG, "I2C init OK");
  else
	  ESP_LOGE(TAG, "I2C init ERR");
  return ret;
}




void send_know_device(void)
{
  char payload[64];
  char str2[18];
  strcpy_P(str2, text_know_mqtt_device);
  for (uint8_t idx = 0; idx < MAX_KNOW_MQTT_INTERNAL_RAM; idx++)
  {
    if (know_mqtt[idx].type != TYPE_FREE)
    {
      itoa(know_mqtt[idx].type, payload, 10);
      send_mqtt_message_prefix_id_topic_payload(mqtt_client, str2, idx, "type", payload);
      itoa(know_mqtt[idx].last_update, payload, 10);
      send_mqtt_message_prefix_id_topic_payload(mqtt_client, str2, idx, "last_update", payload);
      strcpy(payload, know_mqtt[idx].device);
      send_mqtt_message_prefix_id_topic_payload(mqtt_client, str2, idx, "device", payload);
    }
  }
}


void get_device_status(void)
{
   wifi_ap_record_t wifidata;
   if (esp_wifi_sta_get_ap_info(&wifidata)==0)
     {
     wifi_rssi = 255 - wifidata.rssi;
     }

   free_heap = esp_get_free_heap_size();
}

void send_device_status(void)
{
  char str_topic[32];
  char payload[16];

  strcpy(str_topic, "status/uptime");
  sprintf(payload, "%ld", uptime);
  send_mqtt_general_payload(mqtt_client, str_topic, payload);

  if (wifi_link == WIFI_EVENT_STA_CONNECTED)
      send_mqtt_general_payload(mqtt_client, "status/wifi/link", "link");
  else
      send_mqtt_general_payload(mqtt_client, "status/wifi/link", "no-link");

  if (eth_link == ETHERNET_EVENT_CONNECTED)
      send_mqtt_general_payload(mqtt_client, "status/eth/link", "link");  
  else
      send_mqtt_general_payload(mqtt_client, "status/eth/link", "no-link");

  itoa(free_heap, payload, 10);
  send_mqtt_general_payload(mqtt_client, "status/heap", payload);
}


void send_energy_status(void)
{
    char payload[16];
    struct_energy_t energyx;
    int32_t miliwats_now[MAX_ADC_INPUT];
    int32_t total_miliwats[MAX_ADC_INPUT];
    for (uint8_t idx = 0; idx < MAX_ADC_INPUT; idx++)
    {
       energy_get_all_parametr(idx, &energyx);
       if (energyx.used == 1)
         {
         send_mqtt_message_prefix_id_topic_payload(mqtt_client, "energy", idx, "name", energyx.name);

         itoa(energyx.total_watt, payload, 10);
         send_mqtt_message_prefix_id_topic_payload(mqtt_client, "energy", idx, "total_watt", payload);

         itoa(energyx.current_now, payload, 10);
         send_mqtt_message_prefix_id_topic_payload(mqtt_client, "energy", idx, "current_now", payload);

         itoa((energyx.current_now * energyx.volt)/1000, payload, 10);
         send_mqtt_message_prefix_id_topic_payload(mqtt_client, "energy", idx, "consume_watt", payload);
         }
    }

    for (uint8_t idx = 0; idx < MAX_ADC_INPUT; idx++)
    {
        total_miliwats[idx] = 0;
	miliwats_now[idx] = 0;
        energy_get_all_parametr(idx, &energyx);
        if ((energyx.used == 1) && (energyx.group < MAX_ADC_INPUT))
	{
	    if ((energyx.flags & (1 << flag_direction_current)) == 0)
                {
                miliwats_now[energyx.group] = miliwats_now[energyx.group] + (energyx.current_now * energyx.volt);
                total_miliwats[energyx.group] = total_miliwats[energyx.group] + energyx.total_watt;
                }
            else
                {
                miliwats_now[energyx.group] = miliwats_now[energyx.group] - (energyx.current_now * energyx.volt);
                total_miliwats[energyx.group] = total_miliwats[energyx.group] - energyx.total_watt;
                }
	}
    }

    for (uint8_t idx = 0; idx < MAX_ADC_INPUT; idx++)
    {
        ltoa(total_miliwats[idx]/1000, payload, 10);
        send_mqtt_message_prefix_id_topic_payload(mqtt_client, "energy-group", idx, "total_watt", payload);
	ltoa(miliwats_now[idx]/1000, payload, 10);
        send_mqtt_message_prefix_id_topic_payload(mqtt_client, "energy-group", idx, "consume_watt", payload);
    }
}



/*************************************************************************************************************************/
///SEKCE MQTT ///

void mqtt_callback_prepare_topic_array(char *out_str, char *in_topic)
{
  uint8_t cnt = 0;
  cnt = strlen(in_topic) - strlen(out_str);
  memcpy(out_str, in_topic + strlen(out_str), cnt);
  out_str[cnt] = 0;
}

void mqtt_callback(char* topic, uint8_t * payload, unsigned int length_topic, unsigned int length_data)
{
  char str1[64];
  char my_payload[128];
  char my_topic[128];
  uint8_t id = 0;
  struct_energy_t energyx;

  memset(my_payload, 0, 128);
  memset(my_topic, 0, 128);
  ////
  mqtt_receive_message++; /// inkrementuji promenou celkovy pocet prijatych zprav
  strncpy(my_payload, (char*) payload, length_data);
  strncpy(my_topic, (char*) topic, length_topic);
  ////
  /// pridam mqqt kamarada typ termbig
  strcpy_P(str1, termbig_subscribe);
  if (strcmp(str1, my_topic) == 0)
  {
    if (strcmp(device.nazev, my_payload) != 0) /// sam sebe ignoruj
    {
      mqtt_process_message++; /// inkrementuji promenou celkovy pocet zpracovanych zprav
      know_mqtt_create_or_update(my_payload, TYPE_TERMBIG);
    }
  }
  ////
  //// pridam mqtt kamarada typ room controler
  strcpy_P(str1, thermctl_subscribe);
  if (strcmp(str1, my_topic) == 0)
  {
    if (strcmp(device.nazev, my_payload) != 0) /// sam sebe ignoruj
    {
      mqtt_process_message++;
      know_mqtt_create_or_update(my_payload, TYPE_THERMCTL);
    }
  }
  //// pridam mqtt kamarada typ room controler
  strcpy_P(str1, monitor_subscribe);
  if (strcmp(str1, my_topic) == 0)
  {
    if (strcmp(device.nazev, my_payload) != 0) /// sam sebe ignoruj
    {
      mqtt_process_message++;
      know_mqtt_create_or_update(my_payload, TYPE_MONITOR);
    }
  }

  // /monitor-in/NAZEV/energy/store
  strcpy_P(str1, monitor_header_in);
  strcat(str1, device.nazev);
  strcat(str1, "/energy/store");
  if (strncmp(str1, my_topic, strlen(str1)) == 0)
  {
    mqtt_process_message++;
    mqtt_callback_prepare_topic_array(str1, my_topic);
    id = atoi(my_payload);
    energy_get_all_parametr(id, &energyx);
    energy_store_update_all(id, energyx);
    ESP_LOGI(TAG, "Persist energy store idx=%d\n", id);
  }

  // /monitor-in/NAZEV/energy/store
  strcpy_P(str1, monitor_header_in);
  strcat(str1, device.nazev);
  strcat(str1, "/energy/reset");
  if (strncmp(str1, my_topic, strlen(str1)) == 0)
  {
    mqtt_process_message++;
    mqtt_callback_prepare_topic_array(str1, my_topic);
    id = atoi(my_payload);
    energy_struct_default_init_idx(id);
    ESP_LOGI(TAG, "Reset energy store idx=%d\n", id);
  }


  // /monitor-in/NAZEV/energy/store
  strcpy_P(str1, monitor_header_in);
  strcat(str1, device.nazev);
  strcat(str1, "/relay/state");
  if (strncmp(str1, my_topic, strlen(str1)) == 0)
  {
    mqtt_process_message++;
    mqtt_callback_prepare_topic_array(str1, my_topic);
    id = atoi(my_payload);
    if (id == 0) gpio_set_level(RELAY_OUTPUT, 0);
    if (id == 1) gpio_set_level(RELAY_OUTPUT, 1);
  }


  ////
  //// ziskani nastaveni site
  strcpy_P(str1, monitor_header_in);
  strcat(str1, device.nazev);
  strcat(str1, "/network/get/config");
  if (strncmp(str1, my_topic, strlen(str1)) == 0)
  {
    mqtt_process_message++;
    send_network_config(mqtt_client);
  }
  //// /monitor-in/XXXXX/reload
  strcpy_P(str1, monitor_header_in);
  strcat(str1, device.nazev);
  strcat(str1, "/reload");
  if (strcmp(str1, my_topic) == 0)
  {
    mqtt_process_message++;
    esp_restart();
  }
  //// /monitor-in/XXXXX/bootloader
  strcpy_P(str1, monitor_header_in);
  strcat(str1, device.nazev);
  strcat(str1, "/bootloader");
  if (strcmp(str1, my_topic) == 0)
  {
    mqtt_process_message++;
    EEPROM.write(bootloader_tag, 255);
    xTaskCreate(&ota_task, "ota_task", 8192, NULL, 5, NULL);
  }
  //// /thermctl-in/XXXXX/default
  strcpy_P(str1, monitor_header_in);
  strcat(str1, device.nazev);
  strcat(str1, "/default");
  if (strcmp(str1, my_topic) == 0)
  {
    mqtt_process_message++;
    EEPROM.write(set_default_values, atoi(my_payload));
  }
}



bool mqtt_reconnect(void)
{
  char nazev[10];
  char topic[26];
  bool ret = false;
  device_get_name(nazev);
  strcpy_P(topic, monitor_header_in);
  strcat(topic, nazev);
  strcat(topic, "/#");

  esp_mqtt_client_subscribe(mqtt_client, topic, 0);

  strcpy_P(topic, monitor_header_in);
  strcat(topic, "global/#");
  esp_mqtt_client_subscribe(mqtt_client, topic, 0);

  strcpy_P(topic, monitor_subscribe);
  esp_mqtt_client_subscribe(mqtt_client, topic, 0);

  ret = true;
  return ret;
}




void print_sha256(const uint8_t *image_hash, const char *label)
{
  char hash_print[HASH_LEN * 2 + 1];
  hash_print[HASH_LEN * 2] = 0;
  for (int i = 0; i < HASH_LEN; ++i) {
      sprintf(&hash_print[i * 2], "%02x", image_hash[i]);
  }
  ESP_LOGI(TAG, "%s %s", label, hash_print);
}


void get_sha256_of_partitions(void)
{
  uint8_t sha_256[HASH_LEN] = { 0 };
  esp_partition_t partition;
  //
  // get sha256 digest for bootloader
  partition.address   = ESP_BOOTLOADER_OFFSET;
  partition.size      = ESP_PARTITION_TABLE_OFFSET;
  partition.type      = ESP_PARTITION_TYPE_APP;
  esp_partition_get_sha256(&partition, sha_256);
  print_sha256(sha_256, "SHA-256 for bootloader: ");
  //
  // get sha256 digest for running partition
  esp_partition_get_sha256(esp_ota_get_running_partition(), sha_256);
  print_sha256(sha_256, "SHA-256 for current firmware: ");
}




void ota_task(void *pvParameter)
{
  ESP_LOGI(TAG, "Starting OTA bootloader");
  ESP_LOGI(TAG, "Only ethernet");
  struct ifreq ifr;
  esp_netif_get_netif_impl_name(eth_netif, ifr.ifr_name);
  ESP_LOGI(TAG, "Bind interface name is %s", ifr.ifr_name);
  //
  esp_http_client_config_t config = {
  .url = device.bootloader_uri,
  .cert_pem = "",
  .event_handler = http_event_handler,
  .skip_cert_common_name_check = true,
  .if_name = &ifr,
  };
  //
  esp_https_ota_config_t ota_config = {
        .http_config = &config,
    };
  ESP_LOGI(TAG, "Attempting to download update from %s", config.url);
  esp_err_t ret = esp_https_ota(&ota_config);
  if (ret == ESP_OK) 
      {
      ESP_LOGI(TAG, "OTA Succeed, Rebooting...");
      esp_restart();
      } 
     else 
     {
     ESP_LOGE(TAG, "Firmware upgrade failed");
     }
  while (1) 
      {
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      }
}

