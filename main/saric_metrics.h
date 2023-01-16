#ifndef __SARIC_METRICS_H
#define __SARIC_METRICS_H

#include <stdlib.h>
#include <inttypes.h>
#include "saric_utils.h"
#include "pgmspace.h"
#include "saric_energy.h"
#include "cJSON.h"
#include "esp32_saric_mqtt_network.h"

void prom_metric_up(char *str);
void prom_metric_device_status(char *str);
void prom_metric_energy_status(char *str);
void prom_metric_energy_group_status(char *str);

void json_device_config(char *str);

#endif

