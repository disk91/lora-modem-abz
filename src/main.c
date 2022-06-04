#include <LoRaWAN/Utilities/timeServer.h>
#include <loramac-node/src/radio/radio.h>
#include "atci.h"
#include "cmd.h"
#include "adc.h"
#include "sx1276io.h"
#include "lrw.h"
#include "system.h"
#include "log.h"
#include "lpuart.h"
#include "spi.h"
#include "gpio.h"
#include "rtc.h"
#include "usart.h"
#include "irq.h"
#include "part.h"
#include "eeprom.h"
#include "halt.h"
#include "nvm.h"


int main(void)
{
    system_init();

#ifdef DEBUG
    log_init(LOG_LEVEL_DUMP, LOG_TIMESTAMP_ABS);
#else
    log_init(LOG_LEVEL_OFF, LOG_TIMESTAMP_ABS);
#endif
    log_info("LoRa Module %s [LoRaMac %s] built on %s", VERSION, LIB_VERSION, BUILD_DATE);

    nvm_init();
    cmd_init(sysconf.uart_baudrate);

    adc_init();
    spi_init(10000000);
    sx1276io_init();

    lrw_init();
    log_debug("LoRaMac: Starting");
    LoRaMacStart();
    cmd_event(CMD_EVENT_MODULE, CMD_MODULE_BOOT);

    while (1) {
        cmd_process();
        lrw_process();
        sysconf_process();

        disable_irq();

        // If the application scheduled a reset, perform it as soon as the MCU
        // is allowed to sleep, which indicates that there is no more work to be
        // done (e.g., NVM updates).
        if (schedule_reset && system_is_sleep_allowed())
            system_reset();
        else
            system_sleep();

        enable_irq();

        // Invoke lrw_process as the first thing after waking up to give the MAC
        // a chance to timestamp incoming downlink as quickly as possible.
        lrw_process();
    }
}


void system_on_enter_stop_mode(void)
{
    spi_io_deinit();
    sx1276io_deinit();
    adc_deinit();
    lpuart_enter_stop_mode();
}


void system_on_exit_stop_mode(void)
{
    lpuart_leave_stop_mode();
    spi_io_init();
    sx1276io_init();
}
