#include <stdlib.h>
#include "sx1276-board.h"
#include "radio.h"

void GpioWrite( Gpio_t *obj, uint32_t value )
{
    gpio_write(RADIO_NSS_PORT, RADIO_NSS_PIN, value);
}

uint16_t SpiInOut( Spi_t *obj, uint16_t outData )
{
    return spi_transfer(outData);
}