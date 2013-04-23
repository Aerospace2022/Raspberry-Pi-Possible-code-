#include <bcm2835.h>
#include <inttypes.h>

typedef enum {
    HAB_SPI_CSA,
    HAB_SPI_CSB,
    HAB_SPI_CSC,
    HAB_SPI_CSD
} hab_spi_cs_t;

typedef struct {
    hab_spi_cs_t cs;
    uint8_t pinA;
    uint8_t pinB;
} aux_cs_t;

void hab_spi_set_cs(hab_spi_cs_t aCS);
void hab_spi_set_aux_gpio(uint8_t pinA, uint8_t pinB);
void hab_spi_lower_cs(void);
void hab_spi_raise_cs(void);

hab_spi_cs_t hab_spi_cs(void);
uint8_t hab_spi_aux_gpio_A(void);
uint8_t hab_spi_aux_gpio_B(void);