#include "atci.h"
#include "cmd.h"
#include "adc.h"
#include "sx1276io.h"
#include "config.h"
#include "io.h"
#include "lora.h"
#include "system.h"
#include "timeServer.h"
#include "log.h"
#include "lpuart.h"
#include "spi.h"
#include "gpio.h"
#include "usart.h"
#include "irq.h"

static void lora_join_status(bool status);
static void tx_needed(void);
static uint8_t get_battery_level(void);
static float lora_get_temperature(void);
static void lora_mac_process_notify(void);
static void lora_on_rx_data(uint8_t port, uint8_t *buffer, uint8_t length, lora_rx_params_t *params);
static void lora_on_tx_data(lora_tx_params_t *params);

static lora_callback_t lora_callbacks = {
    .config_save = config_save,
    .get_battery_level = get_battery_level,
    .get_temperature_level = lora_get_temperature,
    .get_unique_id = system_get_unique_id,
    .get_random_seed = system_get_random_seed,
    .join_status = lora_join_status,
    .tx_needed = tx_needed,
    .mac_process_notify = lora_mac_process_notify,
    .on_rx_data = lora_on_rx_data,
    .on_tx_data = lora_on_tx_data
};

bool lora_process_request = false;

#pragma pack(push, 4)
typedef struct
{
    lora_configuration_t lora;
} configuration_t;
#pragma pack(pop)

static const configuration_t configuration_default = {
    .lora = {
        .region = LORAMAC_REGION_EU868,
        .public_network = LORA_PUBLIC_NETWORK,
        .otaa = LORA_ENABLE,
        .duty_cycle = LORA_ENABLE, // Enable for EU868
        .class = LORA_CLASS,
        .devaddr = LORA_DEVICE_ADDRESS,
        .deveui = LORA_DEVICE_EUI,
        .appeui = LORA_JOIN_EUI,
        .appkey = LORA_DEFAULT_KEY,
        .nwkkey = LORA_DEFAULT_KEY,
        .nwksenckey = LORA_DEFAULT_KEY,
        .appskey = LORA_DEFAULT_KEY,
        .fnwksIntkey = LORA_DEFAULT_KEY,
        .snwksintkey = LORA_DEFAULT_KEY,
        .chmask = {0xff,0,0,0,0,0},
        .tx_datarate = DR_0,
        .adr = LORA_ADR_ON,
        .tx_repeats = 1,
    }
};

configuration_t configuration;

int main(void)
{
    system_init();

    log_init(LOG_LEVEL_DUMP, LOG_TIMESTAMP_ABS);

    log_debug("configuration %d", sizeof(configuration));

    config_init(&configuration, sizeof(configuration), &configuration_default);

    adc_init();

    spi_init(10000000);

    sx1276io_init();

    lora_init(&configuration.lora, &lora_callbacks);

    cmd_init();

    cmd_event(0, 0);

    while (1)
    {
        cmd_process();
        
        lora_process();

        // low power section
        CRITICAL_SECTION_BEGIN();
        if (lora_process_request)
        {
            lora_process_request = false; // reset notification flag
        }
        else
        {
#ifndef LOW_POWER_DISABLE
            system_low_power();
#endif
        }
        CRITICAL_SECTION_END();
    }
}

void lora_mac_process_notify(void)
{
    lora_process_request = true;
}

static void lora_join_status(bool status)
{
    cmd_event(1, status);
}

static void tx_needed(void)
{
    log_debug("tx_needed");
}

static uint8_t get_battery_level(void)
{
    // callback to get the battery level in % of full charge (254 full charge, 0 no charge)
    return LORA_MAX_BAT;
}

static float lora_get_temperature(void)
{
    return adc_get_temperature_celsius();
}

static void lora_on_rx_data(uint8_t port, uint8_t *buffer, uint8_t length, lora_rx_params_t *params)
{
    atci_printf('+RECV=%d,%d\r\n\r\n', port, length);
    atci_write(buffer, length);
    // atci_print_buffer_as_hex(buffer, length);
}

static void lora_on_tx_data(lora_tx_params_t *params)
{
    if (params->ack_received)
        cmd_print("+ACK\r\n\r\n");
}

void system_on_enter_stop_mode(void)
{
    spi_io_deinit();
    sx1276io_deinit();
    adc_deinit();
    usart_io_deinit();
}

void system_on_exit_stop_mode(void)
{
    spi_io_init();
    sx1276io_init();
    lpuart_io_init();
    usart_io_init();
}
