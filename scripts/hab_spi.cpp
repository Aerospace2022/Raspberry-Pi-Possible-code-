#inlucd "hab_spi.h"
#include <bcm2835.h>

HABSPI::HABSPI(HAB_SPI_CS aCS, uint8_a A = RPI_V2_GPIO_P1_18, uint8_t B = RPI_V2_GPIO_P1_22) {
    _cs = aCS;
    _auxA = A;
    _auxB = B;
}

HABSPI::~HABSPI(void) {
    delete _cs;
    delete _auxA;
    delete _auxB;
}

HABSPI::_setup(void) {
    bcm2835_gpio_fsel(this->_auxA,BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(this->_auxB,BCM2835_GPIO_FSEL_OUTP);
}

HABSPI::lower_cs(void) {
    uint8_t a,b;
    if( this->_cs == HAB_SPI_CSA ) {
        a = b = 0;
    }
    switch( this->_cs ) {
        case HAB_SPI_CSA: {
            a = b = 0;
            break;
        }
        case HAB_SPI_CSB: {
            a = 1;
            b = 0;
            break;
        }
        case HAB_SPI_CSC: {
            a = 0;
            b = 1;
            break;
        }
        case HAB_SPI_CSD: {
            a = b = 1;
            break;
        }
    }
    bcm2835_gpio_write(this->_auxA,a);                    
    bcm2835_gpio_write(this->_auxB,b);        
}

HABSPI::raise_cs(void) {
    //  by lowering both of the aux CS control pins
    //  the 74HC139 will raise all all of the output
    //  pins once G (CS0) is raised
    bcm2835_gpio_write(this->_auxA,LOW);                    
    bcm2835_gpio_write(this->_auxB,LOW);  
}