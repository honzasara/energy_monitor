#include "driver/i2c.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_netif.h"

#ifndef GLOBALH_INCLUDED
#define GLOBALH_INCLUDED

#define HASH_LEN 32

#define BUTTON_PRESSED 1
#define BUTTON_RELEASED 0
#define BUTTON_HOLD 2
#define BUTTON_NONE 0


#define ENC28J60_RESET (GPIO_NUM_4)
#define ENC28J60_SCLK_GPIO 14
#define ENC28J60_MOSI_GPIO 13
#define ENC28J60_MISO_GPIO 12
#define ENC28J60_CS_GPIO 15
#define ENC28J60_SPI_CLOCK_MHZ 20
#define ENC28J60_INT_GPIO 23

#define ADC_F1 ADC_CHANNEL_0
#define ADC_F2 ADC_CHANNEL_3
#define ADC_F3 ADC_CHANNEL_6
#define ADC_F4 ADC_CHANNEL_7
#define ADC_F5 ADC_CHANNEL_4
#define ADC_F6 ADC_CHANNEL_5

#define BUTTON_UP GPIO_NUM_5
#define BUTTON_DOWN GPIO_NUM_18
#define BUTTON_ENTER GPIO_NUM_19

#define CROSS_DETECTOR GPIO_NUM_27
#define S0_CONTACK GPIO_NUM_25
#define GENERIC_INPUT GPIO_NUM_26

#define RELAY_OUTPUT GPIO_NUM_2

#define GPIO_OUTPUT_PIN_SEL  (1ULL<<RELAY_OUTPUT)
#define GPIO_INPUT_PIN_SEL  ((1ULL<<BUTTON_UP) | (1ULL<<BUTTON_DOWN) | (1ULL<<BUTTON_ENTER) | (1ULL<<CROSS_DETECTOR) | (1ULL<<S0_CONTACK) | (1ULL<<GENERIC_INPUT))
#define ESP_INTR_FLAG_DEFAULT 0

#define portTICK_RATE_MS portTICK_PERIOD_MS

#define I2C_MASTER_SCL_IO GPIO_NUM_22               /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO GPIO_NUM_21               /*!< gpio number for I2C master data  */

#define ACK_CHECK_EN 0x1                        /*!< I2C master will check ack from slave*/
#define WRITE_BIT I2C_MASTER_WRITE              /*!< I2C master write */
#define READ_BIT I2C_MASTER_READ                /*!< I2C master read */
#define I2C_MASTER_FREQ_HZ 400000
#define ACK_VAL I2C_MASTER_ACK                             /*!< I2C ack value */
#define NACK_VAL I2C_MASTER_NACK                             /*!< I2C nack value */



#define I2C_MEMORY_ADDR 0x50

#define RS_TXD  (GPIO_NUM_17)
#define RS_RXD  (GPIO_NUM_16)
#define RS_RTS  (GPIO_NUM_4)
#define RS_CTS  (UART_PIN_NO_CHANGE)




#define RS_BUF_SIZE 32



#define eeprom_wire_know_rom 250 /// konci 954


#define bootloader_tag 0
#define set_default_values 90

extern uint32_t wifi_link;
extern uint32_t eth_link;
extern uint32_t mqtt_link;
extern uint32_t uptime;

extern esp_netif_t *eth_netif;
extern esp_netif_t *wifi_netif;

extern i2c_port_t i2c_num;
esp_err_t twi_init(i2c_port_t t_i2c_num);

void shiftout(uint8_t data);

void send_know_device(void);
void send_device_status(void);
void send_energy_status(void);

void mqtt_callback(char* topic, uint8_t * payload, unsigned int length_topic, unsigned int length_data);
bool mqtt_reconnect(void);

long int millis(void);


void get_sha256_of_partitions(void);
void print_sha256(const uint8_t *image_hash, const char *label);
void ota_task(void *pvParameter);

uint8_t display_menu(uint8_t button_up, uint8_t button_down, uint8_t button_enter, uint8_t redraw);
uint8_t display_menu_0(uint8_t button_up, uint8_t button_down, uint8_t button_enter);
uint8_t display_menu_1(uint8_t button_up, uint8_t button_down, uint8_t button_enter);

typedef struct
  {
    uint64_t event_count;
  } timer_queue_element_t;


void nop(void);

bool adc_calibration_init(adc_unit_t unit, adc_atten_t atten, adc_cali_handle_t *out_handle);

#endif
