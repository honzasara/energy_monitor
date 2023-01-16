#include "saric_metrics.h"


void prom_metric_up(char *str)
{
   strcat(str, "up 1\n");
}


void prom_metric_device_status(char *str)
{
   char str1[16];
   uint32_t time_now = 0;
   sprintf(str1, "uptime %ld\n", uptime);
   strcat(str, str1);

   sprintf(str1, "wifi_rssi %d\n", wifi_rssi);
   strcat(str, str1);

   sprintf(str1, "heap %d\n", free_heap);
   strcat(str, str1);

   time_now = DateTime(__DATE__, __TIME__).unixtime();
   sprintf(str1, "build %ld\n", time_now);
   strcat(str, str1);

}


void prom_metric_energy_status(char *str)
{
   char str1[64];
   struct_energy_t energyx;
   
   for (uint8_t idx = 0; idx < MAX_ADC_INPUT; idx++)
      {
      energy_get_all_parametr(idx, &energyx);
      if (energyx.used == 1)
         {
         sprintf(str1, "energy_active{adc_name=\"%s\",idx=\"%d\"} %d\n", energyx.name, idx, energyx.used);
         strcat(str, str1);
	 sprintf(str1, "energy_consume_now_watt{adc_name=\"%s\",idx=\"%d\"} %d\n", energyx.name, idx, (energyx.current_now * energyx.volt)/1000);
	 strcat(str, str1);
	 sprintf(str1, "energy_total_watt{adc_name=\"%s\",idx=\"%d\"} %ld\n", energyx.name, idx, energyx.total_watt);
	 strcat(str, str1);
	 sprintf(str1, "energy_total_second{adc_name=\"%s\",idx=\"%d\"} %ld\n", energyx.name, idx, energyx.total_second);
	 strcat(str, str1);
	 sprintf(str1, "energy_checkpoint_sync{adc_name=\"%s\",idx=\"%d\"} %ld\n", energyx.name, idx, energyx.checkpoint);
	 strcat(str, str1);
	 }
      else
	 {
	 sprintf(str1, "energy_active{adc_name=\"%s\",idx=\"%d\"} %d\n", energyx.name, idx, energyx.used);
         strcat(str, str1);
	 }
      }
}

void prom_metric_energy_group_status(char *str)
{
    char str1[64];
    struct_energy_t energyx;
    int32_t miliwats_now[MAX_ADC_INPUT];
    int32_t total_miliwats[MAX_ADC_INPUT];

    for (uint8_t idx = 0; idx < MAX_ADC_INPUT; idx++)
        {
	miliwats_now[idx] = 0;
	total_miliwats[idx] = 0;
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
	sprintf(str1, "energy_group_total_watt{group_idx=\"%d\"} %ld\n", idx, total_miliwats[idx]/1000);
	strcat(str, str1);
	sprintf(str1, "energy_group_consume_watt{group_idx=\"%d\"} %ld\n", idx, miliwats_now[idx]/1000);
	strcat(str, str1);
    }
}



void json_device_config(char *str)
{
    char ip_uri[32];
    char itm_list[4];
    uint32_t time_now = 0;
    struct_energy_t energyx;
    cJSON *root, *network, *energy;
    cJSON *energy_data[MAX_ADC_INPUT];

    root = cJSON_CreateObject();

    
    cJSON_AddItemToObject(root, "network", network = cJSON_CreateObject());
    
    cJSON_AddItemToObject(root, "energy", energy = cJSON_CreateObject());

    time_now = DateTime(__DATE__, __TIME__).unixtime();
    sprintf(ip_uri, "%ld", time_now);
    cJSON_AddItemToObject(network, "version", cJSON_CreateString(ip_uri));

    cJSON_AddItemToObject(network, "name", cJSON_CreateString(device.nazev));
    cJSON_AddItemToObject(network, "wifi_essid", cJSON_CreateString(device.wifi_essid));
    cJSON_AddItemToObject(network, "wifi_key", cJSON_CreateString(device.wifi_key));
    
    strcpy(ip_uri, "");
    createString(ip_uri, '.', device.mac, 6, 16, 1);
    cJSON_AddItemToObject(network, "ETH_MAC", cJSON_CreateString(ip_uri));

    strcpy(ip_uri, "");
    createString(ip_uri, '.', device.myIP, 4, 10, 1);
    cJSON_AddItemToObject(network, "IP", cJSON_CreateString(ip_uri));

    strcpy(ip_uri, "");
    createString(ip_uri, '.', device.myMASK, 4, 10, 1);
    cJSON_AddItemToObject(network, "Mask", cJSON_CreateString(ip_uri));

    strcpy(ip_uri, "");
    createString(ip_uri, '.', device.myGW, 4, 10, 1);
    cJSON_AddItemToObject(network, "GW", cJSON_CreateString(ip_uri));

    strcpy(ip_uri, "");
    createString(ip_uri, '.', device.myDNS, 4, 10, 1);
    cJSON_AddItemToObject(network, "DNS", cJSON_CreateString(ip_uri));

    strcpy(ip_uri, "");
    createString(ip_uri, '.', device.mqtt_server, 4, 10, 1);
    cJSON_AddItemToObject(network, "MQTT_IP", cJSON_CreateString(ip_uri));

    sprintf(ip_uri, "%d", device.mqtt_port);
    cJSON_AddItemToObject(network, "MQTT_Port", cJSON_CreateString(ip_uri));

    cJSON_AddItemToObject(network, "MQTT_User", cJSON_CreateString(device.mqtt_user));
    cJSON_AddItemToObject(network, "MQTT_Key", cJSON_CreateString(device.mqtt_key));

    strcpy(ip_uri, "");
    createString(ip_uri, '.', device.ntp_server, 4, 10, 1);
    cJSON_AddItemToObject(network, "NTP_IP", cJSON_CreateString(ip_uri));

    sprintf(ip_uri, "%d", device.http_port);
    cJSON_AddItemToObject(network, "HTTP_Port", cJSON_CreateString(ip_uri));

    cJSON_AddItemToObject(network, "Bootloader_URI", cJSON_CreateString(device.bootloader_uri));

    if (device.dhcp == DHCP_ENABLE)
        cJSON_AddItemToObject(network, "DHCP", cJSON_CreateString("Enable"));
    else
        cJSON_AddItemToObject(network, "DHCP", cJSON_CreateString("Disable"));

    switch (device.via)
	{
        case ENABLE_CONNECT_DISABLE:
	    cJSON_AddItemToObject(network, "via", cJSON_CreateString("ConnectDisable"));
	    break;
        case ENABLE_CONNECT_ETH:
	    cJSON_AddItemToObject(network, "via", cJSON_CreateString("ConnectEthernet"));
            break;
	case ENABLE_CONNECT_WIFI:
	    cJSON_AddItemToObject(network, "via", cJSON_CreateString("ConnectWifi"));
            break;
	case ENABLE_CONNECT_BOTH:
	    cJSON_AddItemToObject(network, "via", cJSON_CreateString("ConnectBoth"));
            break;
	default:
	    cJSON_AddItemToObject(network, "via", cJSON_CreateString("ConnectUnknow"));
            break;
	}


    for (uint8_t idx = 0; idx < MAX_ADC_INPUT; idx++)
      {
      energy_get_all_parametr(idx, &energyx);
      sprintf(itm_list, "%d", idx);
      cJSON_AddItemToObject(energy, itm_list, energy_data[idx] = cJSON_CreateObject());
      cJSON_AddItemToObject(energy_data[idx], "name", cJSON_CreateString(energyx.name));
      cJSON_AddItemToObject(energy_data[idx], "input", cJSON_CreateNumber(energyx.input));
      cJSON_AddItemToObject(energy_data[idx], "period", cJSON_CreateNumber(energyx.period));
      cJSON_AddItemToObject(energy_data[idx], "samples", cJSON_CreateNumber(energyx.samples));
      cJSON_AddItemToObject(energy_data[idx], "id", cJSON_CreateNumber(energyx.id));
      cJSON_AddItemToObject(energy_data[idx], "type", cJSON_CreateNumber(energyx.type));
      cJSON_AddItemToObject(energy_data[idx], "group", cJSON_CreateNumber(energyx.group));
      cJSON_AddItemToObject(energy_data[idx], "noise_limit", cJSON_CreateNumber(energyx.noise_limit));
      cJSON_AddItemToObject(energy_data[idx], "volt", cJSON_CreateNumber(energyx.volt));
      cJSON_AddItemToObject(energy_data[idx], "flags", cJSON_CreateNumber(energyx.flags));
      cJSON_AddItemToObject(energy_data[idx], "used", cJSON_CreateNumber(energyx.used));
      }

    char *json = cJSON_Print(root);
    cJSON_Delete(root);
    strcpy(str, json);
}




