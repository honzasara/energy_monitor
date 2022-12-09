#include "saric_energy.h"

struct_energy_t energy[MAX_ADC_INPUT];


void energy_struct_default_init(void)
{
   for (uint8_t idx = 0; idx < MAX_ADC_INPUT; idx++)
       energy_struct_default_init_idx(idx);
}

void energy_dump_config(uint8_t idx)
{
   struct_energy_t energyx;
   energy_get_all_parametr(idx, &energyx);

   printf("Dump for energy %d\n", idx);
   printf("  Millis: %ld\n", millis());
   printf("  Name: %s\n", energyx.name);
   printf("  Used: %d\n", energyx.used);
   printf("  Input: %d\n", energyx.input);
   printf("  Period: %d\n", energyx.period);
   printf("  Last_Period: %ld\n", energyx.last_period);
   printf("  Samples: %d\n", energyx.samples);
   printf("  ID: %d\n", energyx.id);
   printf("  Type: %d\n", energyx.type);
   printf("  Group: %d\n", energyx.group);
   printf("  TotalWatt: %ld\n", energyx.total_watt);
   printf("  TotalSecond: %ld\n", energyx.total_second);
   printf("  NoiseLimit: %d\n", energyx.noise_limit);
   printf("  Volt: %d\n", energyx.volt);
   printf("  Milisecond: %ld\n", energyx.milisecond);
   printf("  Miliwatt: %ld\n", energyx.miliwatt);
   printf("  Checkpoint: %ld\n", energyx.checkpoint);
   printf("\n");
}

void energy_struct_default_init_idx(uint8_t idx)
{
   struct_energy_t energyx;
   char str1[8];
   sprintf(str1, "mon %d", idx);
   strcpy(energy[idx].name, str1);
   energy[idx].name[7] = 0;
   energy[idx].used = 1;
   energy[idx].input = idx + 1;
   energy[idx].period = PERIOD_1_S;
   energy[idx].last_period = millis();
   energy[idx].samples = 100;
   energy[idx].id = idx;
   energy[idx].type = ENERGY_TYPE_SCT013;
   energy[idx].group = idx;
   energy[idx].total_watt = 0;
   energy[idx].total_second = 0;
   energy[idx].noise_limit = 3;
   energy[idx].volt = 230;
   energy[idx].milisecond = 0;
   energy[idx].miliwatt = 0;
   energy[idx].checkpoint = uptime;
   energy_get_all_parametr(idx, &energyx);
   energy_set_all_parametr(idx, energyx);

   energy_set_flag(idx, flag_direction_current, current_consume);
   energy_store_update_all(idx, energyx);
}

void energy_struct_init(void)
{
  for (uint8_t idx = 0; idx < MAX_ADC_INPUT; idx++)
       {
       energy_struct_init_idx(idx);
       energy_struct_fresh_idx(idx);
       }
}

void energy_store_update_all_mess(uint32_t epoch_time)
{
  struct_energy_t energyx;
  for (uint8_t idx = 0; idx < MAX_ADC_INPUT; idx++)
     {
     energy_get_all_parametr(idx, &energyx);
     energyx.checkpoint = epoch_time;
     energy_store_update_mess(idx, energyx);
     }
}

void energy_set_flag(uint8_t idx, uint8_t flag_id, uint8_t value)
{
   struct_energy_t energyx;
   energy_get_all_parametr(idx, &energyx);
   if (value == 0)
     cbi(energyx.flags, flag_id);
   else
     sbi(energyx.flags, flag_id);

   energy_store_update_all(idx, energyx);
   energy_set_all_parametr(idx, energyx);
}

uint8_t energy_get_flag(uint8_t idx, uint8_t flag_id)
{
   struct_energy_t energyx;
   energy_get_parametr(idx, &energyx);
   if ((energyx.flags & (1<<flag_id)) == 0)
      return 0;
   else
      return 1;
}



void energy_struct_init_idx(uint8_t idx)
{
   struct_energy_t energyx;
   energy_store_get_all(idx, &energyx);
   energy_set_all_parametr(idx, energyx);
}

void energy_struct_fresh_idx(uint8_t idx)
{
   struct_energy_t energyx;
   energy_get_all_parametr(idx, &energyx);
   energyx.last_period = millis();
   energyx.current_now = 0;
   energyx.milisecond = 0;
   energyx.miliwatt = 0;
   energy_set_all_parametr(idx, energyx);
}


void energy_get_parametr(uint8_t idx, struct_energy_t *energyx)
{
   energyx->used = energy[idx].used;
   energyx->input = energy[idx].input;
   energyx->period = energy[idx].period;
   energyx->last_period = energy[idx].last_period;
   energyx->samples = energy[idx].samples;
   energyx->type = energy[idx].type;
   energyx->id = energy[idx].id;
   energyx->group = energy[idx].group;
   energyx->total_watt = energy[idx].total_watt;
   energyx->total_second = energy[idx].total_second;
   energyx->noise_limit = energy[idx].noise_limit;
   energyx->current_now = energy[idx].current_now;
   energyx->volt = energy[idx].volt;
   energyx->milisecond = energy[idx].milisecond;
   energyx->miliwatt = energy[idx].miliwatt;
   energyx->checkpoint = energy[idx].checkpoint;
   energyx->flags = energy[idx].flags;
}


void energy_set_parametr(uint8_t idx, struct_energy_t energyx)
{
   energy[idx].used = energyx.used;
   energy[idx].input = energyx.input;
   energy[idx].period = energyx.period;
   energy[idx].last_period = energyx.last_period;
   energy[idx].samples = energyx.samples;
   energy[idx].type = energyx.type;
   energy[idx].id = energyx.id;
   energy[idx].group = energyx.group;
   energy[idx].total_watt = energyx.total_watt;
   energy[idx].total_second = energyx.total_second;
   energy[idx].noise_limit = energyx.noise_limit;
   energy[idx].current_now = energyx.current_now;
   energy[idx].volt = energyx.volt;
   energy[idx].milisecond = energyx.milisecond;
   energy[idx].miliwatt = energyx.miliwatt;
   energy[idx].checkpoint = energyx.checkpoint;
   energy[idx].flags = energyx.flags;
}




void energy_get_all_parametr(uint8_t idx, struct_energy_t *energyx)
{
   strcpy(energyx->name, energy[idx].name);
   energyx->name[7] = 0;

   energyx->used = energy[idx].used;
   energyx->input = energy[idx].input;
   energyx->period = energy[idx].period;
   energyx->last_period = energy[idx].last_period;
   energyx->samples = energy[idx].samples;
   energyx->type = energy[idx].type;
   energyx->id = energy[idx].id;
   energyx->group = energy[idx].group;
   energyx->total_watt = energy[idx].total_watt;
   energyx->total_second = energy[idx].total_second;
   energyx->noise_limit = energy[idx].noise_limit;
   energyx->current_now = energy[idx].current_now;
   energyx->volt = energy[idx].volt;
   energyx->milisecond = energy[idx].milisecond;
   energyx->miliwatt = energy[idx].miliwatt;
   energyx->checkpoint = energy[idx].checkpoint;
   energyx->flags = energy[idx].flags;
}



void energy_set_all_parametr(uint8_t idx, struct_energy_t energyx)
{
   strcpy(energy[idx].name, energyx.name);
   energy[idx].name[7] = 0;
   energy[idx].used = energyx.used;
   energy[idx].input = energyx.input;
   energy[idx].period = energyx.period;
   energy[idx].last_period = energyx.last_period;
   energy[idx].samples = energyx.samples;
   energy[idx].type = energyx.type;
   energy[idx].id = energyx.id;
   energy[idx].group = energyx.group;
   energy[idx].total_watt = energyx.total_watt;
   energy[idx].total_second = energyx.total_second;
   energy[idx].noise_limit = energyx.noise_limit;
   energy[idx].current_now = energyx.current_now;
   energy[idx].volt = energyx.volt;
   energy[idx].milisecond = energyx.milisecond;
   energy[idx].miliwatt = energyx.miliwatt;
   energy[idx].checkpoint = energyx.checkpoint;
   energy[idx].flags = energyx.flags;
}

void energy_store_update_mess(uint8_t idx, struct_energy_t energyx)
{
   EEPROM.write((uint16_t)adc_input0 + (idx * adc_input_store_size_byte) + adc_input_store_total_watt + 0, energyx.total_watt>>24);
   EEPROM.write((uint16_t)adc_input0 + (idx * adc_input_store_size_byte) + adc_input_store_total_watt + 1, energyx.total_watt>>16);
   EEPROM.write((uint16_t)adc_input0 + (idx * adc_input_store_size_byte) + adc_input_store_total_watt + 2, energyx.total_watt>>8);
   EEPROM.write((uint16_t)adc_input0 + (idx * adc_input_store_size_byte) + adc_input_store_total_watt + 3, energyx.total_watt);
   EEPROM.write((uint16_t)adc_input0 + (idx * adc_input_store_size_byte) + adc_input_store_total_second + 0, energyx.total_second>>24);
   EEPROM.write((uint16_t)adc_input0 + (idx * adc_input_store_size_byte) + adc_input_store_total_second + 1, energyx.total_second>>16);
   EEPROM.write((uint16_t)adc_input0 + (idx * adc_input_store_size_byte) + adc_input_store_total_second + 2, energyx.total_second>>8);
   EEPROM.write((uint16_t)adc_input0 + (idx * adc_input_store_size_byte) + adc_input_store_total_second + 3, energyx.total_second);

   EEPROM.write((uint16_t)adc_input0 + (idx * adc_input_store_size_byte) + adc_input_store_checkpoint + 0, energyx.checkpoint>>24);
   EEPROM.write((uint16_t)adc_input0 + (idx * adc_input_store_size_byte) + adc_input_store_checkpoint + 1, energyx.checkpoint>>16);
   EEPROM.write((uint16_t)adc_input0 + (idx * adc_input_store_size_byte) + adc_input_store_checkpoint + 2, energyx.checkpoint>>8);
   EEPROM.write((uint16_t)adc_input0 + (idx * adc_input_store_size_byte) + adc_input_store_checkpoint + 3, energyx.checkpoint);
}

void energy_store_update_all(uint8_t idx, struct_energy_t energyx)
{
   EEPROM.write((uint16_t)adc_input0 + (idx * adc_input_store_size_byte) + adc_input_store_used, energyx.used);
   energyx.name[7] = 0;
   for (uint8_t id = 0; id < 8; id++) EEPROM.write((uint16_t)adc_input0 + (idx * adc_input_store_size_byte) + adc_input_store_name + id, energyx.name[id]);
   EEPROM.write((uint16_t)adc_input0 + (idx * adc_input_store_size_byte) + adc_input_store_input, energyx.input);

   EEPROM.write((uint16_t)adc_input0 + (idx * adc_input_store_size_byte) + adc_input_store_period, energyx.period>>8);
   EEPROM.write((uint16_t)adc_input0 + (idx * adc_input_store_size_byte) + adc_input_store_period + 1, energyx.period);
   EEPROM.write((uint16_t)adc_input0 + (idx * adc_input_store_size_byte) + adc_input_store_samples, energyx.samples);
   EEPROM.write((uint16_t)adc_input0 + (idx * adc_input_store_size_byte) + adc_input_store_id, energyx.id);
   EEPROM.write((uint16_t)adc_input0 + (idx * adc_input_store_size_byte) + adc_input_store_type, energyx.type);
   EEPROM.write((uint16_t)adc_input0 + (idx * adc_input_store_size_byte) + adc_input_store_group, energyx.group);
   EEPROM.write((uint16_t)adc_input0 + (idx * adc_input_store_size_byte) + adc_input_store_noise_limit, energyx.noise_limit);
   EEPROM.write((uint16_t)adc_input0 + (idx * adc_input_store_size_byte) + adc_input_store_total_watt + 0, energyx.total_watt>>24);
   EEPROM.write((uint16_t)adc_input0 + (idx * adc_input_store_size_byte) + adc_input_store_total_watt + 1, energyx.total_watt>>16);
   EEPROM.write((uint16_t)adc_input0 + (idx * adc_input_store_size_byte) + adc_input_store_total_watt + 2, energyx.total_watt>>8);
   EEPROM.write((uint16_t)adc_input0 + (idx * adc_input_store_size_byte) + adc_input_store_total_watt + 3, energyx.total_watt);
   EEPROM.write((uint16_t)adc_input0 + (idx * adc_input_store_size_byte) + adc_input_store_volt, energyx.volt);
   EEPROM.write((uint16_t)adc_input0 + (idx * adc_input_store_size_byte) + adc_input_store_flags, energyx.flags);

   EEPROM.write((uint16_t)adc_input0 + (idx * adc_input_store_size_byte) + adc_input_store_total_second + 0, energyx.total_second>>24);
   EEPROM.write((uint16_t)adc_input0 + (idx * adc_input_store_size_byte) + adc_input_store_total_second + 1, energyx.total_second>>16);
   EEPROM.write((uint16_t)adc_input0 + (idx * adc_input_store_size_byte) + adc_input_store_total_second + 2, energyx.total_second>>8);
   EEPROM.write((uint16_t)adc_input0 + (idx * adc_input_store_size_byte) + adc_input_store_total_second + 3, energyx.total_second);

   EEPROM.write((uint16_t)adc_input0 + (idx * adc_input_store_size_byte) + adc_input_store_checkpoint + 0, energyx.checkpoint>>24);
   EEPROM.write((uint16_t)adc_input0 + (idx * adc_input_store_size_byte) + adc_input_store_checkpoint + 1, energyx.checkpoint>>16);
   EEPROM.write((uint16_t)adc_input0 + (idx * adc_input_store_size_byte) + adc_input_store_checkpoint + 2, energyx.checkpoint>>8);
   EEPROM.write((uint16_t)adc_input0 + (idx * adc_input_store_size_byte) + adc_input_store_checkpoint + 3, energyx.checkpoint);
}

void energy_store_get_all(uint8_t idx, struct_energy_t *energyx)
{
   energyx->used = EEPROM.read((uint16_t)adc_input0 + (idx * adc_input_store_size_byte) + adc_input_store_used);
   for (uint8_t id = 0; id < 8; id++) energyx->name[id] = EEPROM.read((uint16_t)adc_input0 + (idx * adc_input_store_size_byte) + adc_input_store_name + id);
   energyx->input = EEPROM.read((uint16_t)adc_input0 + (idx * adc_input_store_size_byte) + adc_input_store_input);
   energyx->period = ((uint16_t)EEPROM.read(adc_input0 + (idx * adc_input_store_size_byte) + adc_input_store_period) << 8)  + EEPROM.read(adc_input0 + (idx * adc_input_store_size_byte) + adc_input_store_period + 1);
   energyx->samples = EEPROM.read((uint16_t)adc_input0 + (idx * adc_input_store_size_byte) + adc_input_store_samples);
   energyx->id = EEPROM.read((uint16_t)adc_input0 + (idx * adc_input_store_size_byte) + adc_input_store_id);
   energyx->type = EEPROM.read((uint16_t)adc_input0 + (idx * adc_input_store_size_byte) + adc_input_store_type);
   energyx->group = EEPROM.read((uint16_t)adc_input0 + (idx * adc_input_store_size_byte) + adc_input_store_group);
   energyx->noise_limit = EEPROM.read((uint16_t)adc_input0 + (idx * adc_input_store_size_byte) + adc_input_store_noise_limit);
   energyx->volt = EEPROM.read((uint16_t)adc_input0 + (idx * adc_input_store_size_byte) + adc_input_store_volt);
   energyx->flags = EEPROM.read((uint16_t)adc_input0 + (idx * adc_input_store_size_byte) + adc_input_store_flags);

   energyx->total_watt = (EEPROM.read((uint16_t)adc_input0 + (idx * adc_input_store_size_byte) + adc_input_store_total_watt + 0) << 24) + (EEPROM.read((uint16_t)adc_input0 + (idx * adc_input_store_size_byte) + adc_input_store_total_watt + 1) << 16) + (EEPROM.read((uint16_t)adc_input0 + (idx * adc_input_store_size_byte) + adc_input_store_total_watt + 2) << 8) + EEPROM.read((uint16_t)adc_input0 + (idx * adc_input_store_size_byte) + adc_input_store_total_watt + 3);

   energyx->total_second = (EEPROM.read((uint16_t)adc_input0 + (idx * adc_input_store_size_byte) + adc_input_store_total_second + 0) << 24) + (EEPROM.read((uint16_t)adc_input0 + (idx * adc_input_store_size_byte) + adc_input_store_total_second + 1) << 16) + (EEPROM.read((uint16_t)adc_input0 + (idx * adc_input_store_size_byte) + adc_input_store_total_second + 2) << 8) + EEPROM.read((uint16_t)adc_input0 + (idx * adc_input_store_size_byte) + adc_input_store_total_second + 3);
   
   energyx->checkpoint = (EEPROM.read((uint16_t)adc_input0 + (idx * adc_input_store_size_byte) + adc_input_store_checkpoint + 0) << 24) + (EEPROM.read((uint16_t)adc_input0 + (idx * adc_input_store_size_byte) + adc_input_store_checkpoint + 1) << 16) + (EEPROM.read((uint16_t)adc_input0 + (idx * adc_input_store_size_byte) + adc_input_store_checkpoint + 2) << 8) + EEPROM.read((uint16_t)adc_input0 + (idx * adc_input_store_size_byte) + adc_input_store_checkpoint + 3);
}


void energy_set_all_from_json(uint8_t idx, cJSON *json_energy)
{
    struct_energy_t energyx;
    uint8_t ret = 0;
    cJSON *tmp = NULL;
    energy_get_all_parametr(idx, &energyx);

    /// nastaveni nazvu ADC vstupu
    tmp = cJSON_GetObjectItem(json_energy, "name");
    if (tmp)
    {
        ret = 1;
        strncpy(energyx.name, tmp->valuestring, 8);
	energy_set_all_parametr(idx, energyx);
	ESP_LOGI(TAG, "Set new name idx=%d, value=%s", idx, energyx.name);
    }
    /// nastaveni indexu ADC prevodniku
    tmp = cJSON_GetObjectItem(json_energy, "input");
    if (tmp)
    {
        ret = 1;
        energyx.input = tmp->valueint;
	energy_set_all_parametr(idx, energyx);
	ESP_LOGI(TAG, "Set new input idx=%d, value=%d", idx, energyx.input);
    }
    /// nastaveni merici periody, jak casto se ma merit
    tmp = cJSON_GetObjectItem(json_energy, "period");
    if (tmp)
    {
        ret = 1;
        energyx.period = tmp->valueint;
	energy_set_all_parametr(idx, energyx);
	ESP_LOGI(TAG, "Set new period idx=%d, value=%d", idx, energyx.period);
    }
    /// nastaveni poctu vzorku pro kazdy mereny usek, jedno mereni trva 100usec
    tmp = cJSON_GetObjectItem(json_energy, "samples");
    if (tmp)
    {
        ret = 1;
        energyx.samples = tmp->valueint;
	energy_set_all_parametr(idx, energyx);
	ESP_LOGI(TAG, "Set new samples idx=%d, value=%d", idx, energyx.samples);
    }
    /// nastaveni globalniho ID
    tmp = cJSON_GetObjectItem(json_energy, "id");
    if (tmp)
    {
        ret = 1;
        energyx.id = tmp->valueint;
	energy_set_all_parametr(idx, energyx);
	ESP_LOGI(TAG, "Set new id idx=%d, value=%d", idx, energyx.id);
    }
    /// nastaveni converzniho typu civky
    tmp = cJSON_GetObjectItem(json_energy, "type");
    if (tmp)
    {
        ret = 1;
        energyx.type = tmp->valueint;
	energy_set_all_parametr(idx, energyx);
	ESP_LOGI(TAG, "Set new type idx=%d, value=%d", idx, energyx.type);
    }
    /// nastaveni groupovani mericich ADC
    tmp = cJSON_GetObjectItem(json_energy, "group");
    if (tmp)
    {
        ret = 1;
        energyx.group = tmp->valueint;
	energy_set_all_parametr(idx, energyx);
	ESP_LOGI(TAG, "Set new group idx=%d, value=%d", idx, energyx.group);
    }
    /// nastaveni hranice detekce sumu mereni
    tmp = cJSON_GetObjectItem(json_energy, "noise_limit");
    if (tmp)
    {
        ret = 1;
        energyx.noise_limit = tmp->valueint;
	energy_set_all_parametr(idx, energyx);
	ESP_LOGI(TAG, "Set new noise_limit idx=%d, value=%d", idx, energyx.noise_limit);
    }
    /// nastaveni napeti v siti pro vypocet spotreby 
    tmp = cJSON_GetObjectItem(json_energy, "volt");
    if (tmp)
    {
        ret = 1;
        energyx.volt = tmp->valueint;
	energy_set_all_parametr(idx, energyx);
	ESP_LOGI(TAG, "Set new volt idx=%d, value=%d", idx, energyx.volt);
    }
    /// nastaveni konfiguracnich flagu merice
    tmp = cJSON_GetObjectItem(json_energy, "flags");
    if (tmp)
    {
        ret = 1;
        energyx.flags = tmp->valueint;
	energy_set_all_parametr(idx, energyx);
	ESP_LOGI(TAG, "Set new flags idx=%d, value=%d", idx, energyx.flags);
    }
    /// povoleni tohoto vstupu
    tmp = cJSON_GetObjectItem(json_energy, "used");
    if (tmp)
    {
        ret = 1;
        ESP_LOGI(TAG, "Fresh energy struct %d", idx);
        energy_struct_fresh_idx(idx);
        energy_get_all_parametr(idx, &energyx);
	energyx.used = tmp->valueint;
	energy_set_all_parametr(idx, energyx);
	ESP_LOGI(TAG, "Set new used idx=%d, value=%d", idx, energyx.used);

    }
    /// pri zmene si ulozim data
    if (ret == 1)
	{
	energy_store_update_all(idx, energyx);
	energy_dump_config(idx);
	}
}
