#ifndef __SARIC_ENERGY_H
#define __SARIC_ENERGY_H

#include <stdlib.h>
#include <inttypes.h>
#include "EEPROM.h"
#include "saric_utils.h"

#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"


#define adc_input_store_size_byte 26

#if !defined(adc_input0)
#define adc_input0  1000
#endif

#if !defined(MAX_ADC_INPUT)
#define MAX_ADC_INPUT 6
#endif

#define adc_input_store_last adc_input0 + (adc_input_store_size_byte * MAX_ADC_INPUT)

#define current_consume 0
#define current_production 1

#define flag_direction_current 0


#pragma message( "adc_inout_store_last  = " XSTR(adc_input_store_last))

#define PERIOD_1_S 1000

#define ENERGY_TYPE_SCT013 33

#define adc_input_store_used 0
#define adc_input_store_name 1 //delka 8
#define adc_input_store_input 9
#define adc_input_store_period 10 //delka 2
#define adc_input_store_samples 12
#define adc_input_store_id 13
#define adc_input_store_type 14
#define adc_input_store_group 15
#define adc_input_store_noise_limit 16
#define adc_input_store_total_watt 17 //delka 4
#define adc_input_store_volt 21
#define adc_input_store_total_second 22 //delka 4
#define adc_input_store_checkpoint 26 // delka 4
#define adc_input_store_flags 30
/*
 * 31...39 free
*/
#define adc_input_store_konec 40




typedef struct struct_energy
{
  uint8_t used;
  char name[8];
  uint8_t input;
  uint16_t  period;
  uint32_t last_period;
  uint8_t samples;
  uint8_t id;
  uint8_t type;
  uint8_t group;
  uint32_t total_watt;
  uint32_t total_second;
  uint32_t miliwatt;
  uint32_t milisecond;
  uint8_t noise_limit;
  uint16_t current_now;
  uint8_t volt;
  uint32_t checkpoint;
  uint8_t flags;
}struct_energy_t;


void energy_set_flag(uint8_t idx, uint8_t flag_id, uint8_t value);
uint8_t energy_get_flag(uint8_t idx, uint8_t flag_id);

void sample_data(void);
void energy_struct_init(void);
void energy_struct_init_idx(uint8_t idx);

void energy_struct_default_init(void);
void energy_struct_default_init_idx(uint8_t idx);

void energy_get_parametr(uint8_t idx, struct_energy_t *energyx);
void energy_set_parametr(uint8_t idx, struct_energy_t energyx);
void energy_get_all_parametr(uint8_t idx, struct_energy_t *energyx);
void energy_set_all_parametr(uint8_t idx, struct_energy_t energyx);

void energy_store_update_all(uint8_t idx, struct_energy_t energyx);
void energy_store_get_all(uint8_t idx, struct_energy_t *energyx);

void energy_store_update_mess(uint8_t idx, struct_energy_t energyx);
void energy_store_update_all_mess(uint32_t epoch_time);

#endif

