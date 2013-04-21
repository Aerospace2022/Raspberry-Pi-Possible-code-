#include "hab_spi.h"

static hab_spi_cs_t _cs = HAB_SPI_CSA;
static uint8_t _pinA = RPI_V2_GPIO_P1_18;
static uint8_t _pinB = RPI_V2_GPIO_P1_22;

void hab_spi_set_cs(hab_spi_cs_t aCS) {
    _cs = aCS;
}

hab_spi_cs_t hab_spi_cs(void) {
    return _cs;
}

void hab_spi_set_aux_gpio(uint8_t pinA, uint8_t pinB) {
    _pinA = pinA;
    _pinB = pinB;
    
    //  make these outputs
    bcm2835_gpio_fsel(_pinA,BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(_pinB,BCM2835_GPIO_FSEL_OUTP);
}

uint8_t hab_spi_aux_gpio_A(void) {
    return _pinA;
}

uint8_t hab_spi_aux_gpio_B(void) {
    return _pinB;
}

void hab_spi_lower_cs(void) {
    uint8_t a,b;
    switch(_cs) {
        case HAB_SPI_CSA: {
            a = b = LOW;
            break;
        }
        case HAB_SPI_CSB: {
            a = HIGH;
            b = LOW;
            break;
        }
        case HAB_SPI_CSC: {
            a = LOW;
            b = HIGH;
            break;
        }
        case HAB_SPI_CSD: {
            a = b = HIGH;
            break;
        }
    }
    bcm2835_gpio_write(_pinA,a);
    bcm2835_gpio_write(_pinB,b);
}

void hab_spi_raise_cs(void) {
    bcm2835_gpio_write(_pinA,LOW);
    bcm2835_gpio_write(_pinB,LOW);
}