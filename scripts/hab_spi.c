#include "hab_spi.h"

static hab_spi_cs_t _cs = HAB_SPI_CSA;
static uint8_t _pinA = RPI_V2_GPIO_P1_18;
static uint8_t _pinB = RPI_V2_GPIO_P1_22;

aux_cs_t aux = { .cs = HAB_SPI_CSA, .pinA = RPI_V2_GPIO_P1_18, .pinB = RPI_V2_GPIO_P1_22 };

void hab_spi_set_cs(hab_spi_cs_t aCS) {
    aux.cs = aCS;
}

hab_spi_cs_t hab_spi_cs(void) {
    return aux.cs;
}

void hab_spi_set_aux_gpio(uint8_t pinA, uint8_t pinB) {
    aux.pinA = pinA;
    aux.pinB = pinB;
    
    //  make these outputs
    bcm2835_gpio_fsel(aux.pinA, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(aux.pinB, BCM2835_GPIO_FSEL_OUTP);
}

uint8_t hab_spi_aux_gpio_A(void) {
    return aux.pinA;
}

uint8_t hab_spi_aux_gpio_B(void) {
    return aux.pinB;
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
    bcm2835_gpio_write(aux.pinA, a);
    bcm2835_gpio_write(aux.pinB, b);
}

void hab_spi_raise_cs(void) {
    bcm2835_gpio_write(aux.pinA, LOW);
    bcm2835_gpio_write(aux.pinB, LOW);
}