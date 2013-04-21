/*
 *  hab_spi_test.c
 *
 *  Tests the auxillary *CS mechanism on the SPI bus for the HAB
 *  main board.  The BCM2835 has only two chip select outputs for
 *  its SPI bus.  By using two other GPIO pins and a 74HCT139
 *  2-4 line decoder, we can have 4 'virtual' *CS lines.
 *
 *  This test program allows you to set the desired output
 *  chip select line and the inputs A and B to the 74HCT139.
 *  Then it sends a sample SPI message.  For the test, one may wish
 *  to connect MISO > MOSI so that there's something to transfer.
 *
 *  To compile:
 *  gcc hab_spi_test.c hab_spi.c -o b -lbcm2835
 *
 */

#include "hab_spi.h"
#include <stdint.h>
#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/types.h>

/*
 *  Default 74HC139 input pins
 *  A = RPI_V2_GPIO_P1_18 (GPIO24)
 *  B = RPI_V2_GPIO_P1_22 (GPIO25)
 */
static uint8_t pinA = RPI_V2_GPIO_P1_18;
static uint8_t pinB = RPI_V2_GPIO_P1_22;

/*
 *  Default virtual CS
 *  CSA
 */
static hab_spi_cs_t cs = HAB_SPI_CSA;

static void pabort(const char *s)
{
	perror(s);
	abort();
}

static void print_usage(const char *prog)
{
    printf("Tests the auxillary *CS mechanism for the SPI bus on the HAB\n");
    printf("Usage: %s [-cab]\n", prog);
    puts(   "-c --cs\t\tset chip select (A,B,C,D)\n"
            "-a --gpina\tset GPIO pin for input A\n"
            "-b --gpinb\tset GPIO pin for input B\n"
         );
    exit(1);
}

static void parse_opts(int argc, char *argv[])
{
    int ret;
    if( !bcm2835_init() )
        pabort("Unable to init BCM2835 lib");
    while(1) {
        static const struct option lopts[] = {
            { "cs",     required_argument,  NULL,   'c'},
            { "gpina",  required_argument,  NULL,   'a'},
            { "gpinb",  required_argument,  NULL,   'b'},
            {NULL,0,0,0},
        };
        int c;
        c = getopt_long(argc, argv, "cab", lopts, NULL);
        if( c == -1 ) break;
        
        switch( c )
        {
            case 'c':
            {
                //  set CS
                optarg[0] = tolower(optarg[0]);
                uint8_t c_cs = optarg[0];
                switch( c_cs )
                {
                    case 'a':
                    {
                        cs = HAB_SPI_CSA;
                        break;
                    }
                    case 'b':
                    {
                        cs = HAB_SPI_CSB;
                        break;
                    }
                    case 'c':
                    {
                        cs = HAB_SPI_CSC;
                        break;
                    }
                    case 'd':
                    {
                        cs = HAB_SPI_CSD;
                        break;
                    }
                    default:
                        pabort("invalid option for --cs");
                }
                hab_spi_set_cs(cs);
                break;
            }
            case 'a':
            {
                pinA = atoi(optarg);
                hab_spi_set_aux_gpio(pinA,pinB);
                break;
            }
            case 'b':
            {
                pinB = atoi(optarg);
                hab_spi_set_aux_gpio(pinA,pinB);
                break;
            }
            default:
                print_usage(argv[0]);
                break;
        }
    }
}

int main(int argc, char *argv[]) {
    int ret = 0;
    parse_opts(argc, argv);
    
    uint8_t *rd_buf = malloc(4 * sizeof(uint8_t) );
    if( rd_buf == NULL )
        pabort("unable to allocate memory for rd_buf");
    uint8_t *wr_buf = malloc(4 * sizeof(uint8_t) );
    if( wr_buf == NULL )
        pabort("unable to allocate memory for wr_buf");
    memset(wr_buf,'*',4);
    
    //  send a random message
    hab_spi_lower_cs();
    bcm2835_spi_begin();
    bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);      // The default
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);                   // The default
    bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_4096);
    bcm2835_spi_chipSelect(BCM2835_SPI_CS0);                      // The default
    bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);      // the default
    //  transfer
    bcm2835_spi_transfernb(wr_buf,rd_buf, 4);
    bcm2835_spi_end();
    hab_spi_raise_cs();
    
    return ret;
}