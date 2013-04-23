/*
 *  Control SRB-MX146LV via SPI on RPi
 *
 *	Hardware setup
 *
 *	The default hardware setup requires integration between CE0 and two
 *	other GPIO pins on the Raspberry Pi, namely GPIO 24 and GPIO 25,
 *	which occupy pins 18 and 22 on the rev2 board main connector
 *	GPIO 24 -> input A of a 74HC139 multiplexer (pin 2)
 *	GPIO 25 -> input B of the multiplexer (pin 3)
 *	CE0 -> G of the the multiplexer (pin 1)
 *	Radio chip select is CSB.
 *	CSB => Y1
 *
 *	So in order to send a message over SPI, we have to bracket those
 *	calls by raising and lowering the correct combination of GPIO 24 and
 *	GPIO 25.  From the datasheet, in order to lower Y1, we much raise
 *	input A (GPIO24) and lower input B (BPIO25)
 *
 *	To compile:
 *	gcc srb_mx146lv.c hab_spi.c -o srbmx145 -std=c99 -I/usr/include/glib-2.0 -I/usr/lib/arm-linux-gnueabihf/glib-2.0/include -lglib-2.0 -lbcm2835
 *
*/

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <bcm2835.h>
#include <glib.h>
#include "hab_spi.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define BUF_SIZE(a) (sizeof(a) / sizeof(uint8_t))

#define XFR_USE_BCM2835_LIB 1

/*
 *  Function prototypes
 *
 */

static void pabort(const char *s);
static void parse_opts(int argc, char *argv[]);
static void radio_spi_command_write(uint8_t *wr_buf, uint8_t *rd_buf, size_t msglen);
static uint8_t write_radio(uint8_t *data, uint8_t *rd_buf, uint8_t length);
static void print_query_results(uint8_t *data);
static void radio_perform_query(char *query);
static void radio_spi_command_write(uint8_t *wr_buf, uint8_t *rd_buf, size_t msglen);
static void radio_set_frequency(uint32_t f);
static void radio_perform_memory_operation(uint8_t channel, char op);

static void pabort(const char *s)
{
	perror(s);
	abort();
}

static const char *device = "/dev/spidev0.0";
static uint8_t mode = 0;
static uint8_t bits = 8;
static uint32_t speed = 39062;//500000;
static uint16_t delay;
static const uint8_t MESSAGE_LENGTH = 22;

static void print_usage(const char *prog)
{
	printf("Usage: %s [-FfHMmCENDVSr]\n", prog);
	puts(   " -p --aprs\t\tset transmit frequency to US APSR (144.39)\n"
            " -F --freq\t\tset freq as 32 bit binary in Hz (little endian)\n"
			" -M --rmem\tread freq from memory loc\n"
			" -m --wmem\twrite active freq to memory loc\n"
			" -N --qname\tread device name\n"
			" -D --qdate\tread datecode\n"
			" -V --qvers\tread software version\n"
			" -S --qser\tread serial number\n"
			" -r --qfreq\tread minimum and maximum freqs"
		);
	exit(1);
}

/*	Writes data to the radio with array of length */
static uint8_t write_radio(uint8_t *data, uint8_t *rd_buf, uint8_t length) {
    uint8_t *wr_buf = malloc(MESSAGE_LENGTH * sizeof(uint8_t));
    if( wr_buf == NULL )
        pabort("unable to allocate memory for write buffer");
        
    //  pad our message with the required dummy character '?'
    memcpy(memset(wr_buf,0x3F,MESSAGE_LENGTH),data,2);
    
	int ret = 0;
    
    #if XFR_USE_BCM2835_LIB
        //  set the aux pins on the 74139 before dropping the RPi *CS
        hab_spi_lower_cs();
        
        bcm2835_spi_begin();
        bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);      // The default
        bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);                   // The default
        bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_4096);
        bcm2835_spi_chipSelect(BCM2835_SPI_CS0);                      // The default
        bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);      // the default
        //  transfer
        bcm2835_spi_transfernb(wr_buf,rd_buf,MESSAGE_LENGTH);
        bcm2835_spi_end();
        //  for unclear reasons we must execute the transfer twice
        bcm2835_delay(20);
        bcm2835_spi_begin();
        bcm2835_spi_transfernb(wr_buf,rd_buf,MESSAGE_LENGTH);
        bcm2835_spi_end();
        
        //  restore our aux pins
        hab_spi_raise_cs();
    #else
        int fd = open(device, O_RDWR);
        if( fd < 0 ) pabort("unable to open device");
        //	set SPI mode
        ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
        if( ret == -1 ) pabort("unable to set SPI write mode");
        ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);
        if( ret == -1) pabort("unable to set SPI read mode");
        //	set SPI word size
        ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
        if( ret == -1) pabort("unable to set SPI word length");
        //	set SPI max speed
        ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
        if (ret == -1) pabort("unable to set SPI max speed");
        
        struct spi_ioc_transfer tr = {
            .tx_buf = (unsigned long)wr_buf,
            .rx_buf = (unsigned long)rd_buf,
            .len = MESSAGE_LENGTH,
            .delay_usecs = delay,
            .speed_hz = speed,
            .bits_per_word = bits,
        };
    
        ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
        //	for some reason, the message must be sent twice
        ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
        close(fd);
        free(wr_buf);
        if (ret < 1) pabort("can't send spi message");
    #endif
    return ret;
}

static void print_query_results(uint8_t *data) {
    uint8_t err_code = data[0];
    uint8_t length = data[1];
    char str[length+1];
    memcpy(&str,&data[2],length);
    str[length] = '\0';
    printf("%s\n",str);
}

//  
static void radio_perform_query(char *query) {
    uint8_t data[2];
    memcpy(&data,query,2);      //  all queries are 2 bytes in length
    uint8_t *rx_buf = (uint8_t *)malloc(MESSAGE_LENGTH * sizeof(uint8_t));
    if( rx_buf == NULL )
        pabort("unable to allocate memory for read buffer");
    write_radio(data,rx_buf,MESSAGE_LENGTH);
    print_query_results(rx_buf);
    
    free(rx_buf);
}

//  write to radio without padding message with '?' as we do with queries
static void radio_spi_command_write(uint8_t *wr_buf, uint8_t *rd_buf, size_t msglen)
{
    //  set the aux pins on the 74139 before dropping the RPi *CS
    hab_spi_lower_cs();
    
    bcm2835_spi_begin();
    bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);                   
    bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_4096);
    bcm2835_spi_chipSelect(BCM2835_SPI_CS0);                      
    bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);
    //  transfer
    bcm2835_spi_transfernb(wr_buf,rd_buf,MESSAGE_LENGTH);
    bcm2835_spi_end();
    //  for unclear reasons we must execute the transfer twice
    bcm2835_delay(20);
    bcm2835_spi_begin();
    bcm2835_spi_transfernb(wr_buf,rd_buf,MESSAGE_LENGTH);
    bcm2835_spi_end();
    
    //  restore our aux pins
    hab_spi_raise_cs();
}

/*  set the frequency to the value specified by f */
static void radio_set_frequency(uint32_t f) {
    unsigned char *p = (unsigned char *)&f;
    uint8_t *wr_buf = (uint8_t *)malloc(5 * sizeof(uint8_t));
    if( wr_buf == NULL )
        pabort("unable to allocate memory for write buffer");
    wr_buf[0] = 'B';
    memcpy(&wr_buf[1],p,sizeof(f));
    
    uint8_t *rd_buf = (uint8_t *)malloc(5 * sizeof(uint8_t));
    if( rd_buf == NULL )
        pabort("unable to allocate memory for read buffer");
    
    radio_spi_command_write(wr_buf, rd_buf, 5);
    
    free(wr_buf);
    free(rd_buf);
}

static void radio_perform_memory_operation(uint8_t channel, char op) {
    
    if( channel > 15 )
        pabort("Channel out of range");
    if( (op != 'M') && (op != 'm') )
        pabort("ERROR | channel out of range");
    unsigned char *p = (unsigned char *)&channel;
    uint8_t *wr_buf = (uint8_t *)malloc(2 * sizeof(uint8_t));
    if( wr_buf == NULL )
        pabort("unable to allocate memory for write buffer");
    wr_buf[0] = op;
    memcpy(&wr_buf[1],p,sizeof(unsigned char));
    
    uint8_t *rd_buf = (uint8_t *)malloc(2 * sizeof(uint8_t));
    if( rd_buf == NULL )
        pabort("unable to allocate memory for read buffer");
        
    radio_spi_command_write(wr_buf,rd_buf,2);
        
    free(wr_buf);
    free(rd_buf);
}

static void parse_opts(int argc, char *argv[])
{
	int ret;
	if(!bcm2835_init())
		pabort("Unable to init BCM2835 lib");
	while (1) {
		static const struct option lopts[] = {
            { "aprs",       no_argument,        NULL, 'p'},
			{ "freq32",		required_argument, 	NULL, 'F'},
			{ "rmem",		required_argument, 	NULL, 'M'},
			{ "wmem",		required_argument, 	NULL, 'm'},
			{ "qname",		no_argument, 		NULL, 'N'},
			{ "qdate",		no_argument, 		NULL, 'D'},
			{ "qvers",		no_argument, 		NULL, 'V'},
			{ "qser",		no_argument, 		NULL, 'S'},
            { "qtemp",      no_argument,        NULL, 'T'},
			{ "qfreq",		no_argument, 		NULL, 'r'},
			{ NULL, 0, 0, 0 },
		};
		int c;

		c = getopt_long(argc, argv, "pF:M:m:NDVSTr", lopts, NULL);
		if (c == -1) break;

		switch (c) {
            case 'M':
            {
                //  read frequency from memory
                uint8_t channel = atoi(optarg);
                radio_perform_memory_operation(channel,'M');
            }
            case 'm':
            {
                //  write active frequency to memory location
                uint8_t channel = atoi(optarg);
                radio_perform_memory_operation(channel,'M');
            }
            case 'p':
            {
                //  set frequency to 144.390MHz
                uint32_t freq = 144390000;
                radio_set_frequency(144390000UL);
                break;
            }
            case 'F':
            {
                uint32_t freq = atol(optarg);
                radio_set_frequency(freq);
                break;
            }
			case 'N':
                radio_perform_query("QN");      //  device name
				break;
			case 'S':
                radio_perform_query("Q#");
                break;
			case 'V':
                radio_perform_query("QV");
                break;
            case 'T':
            {
				//	temperature as signed 8 bit integer (command 'QT')
                uint8_t data[2] = {0x51, 0x54};
                uint8_t *rx_buf = malloc(22 * sizeof(uint8_t));
                write_radio(data,rx_buf,22);
                for(uint8_t i = 0; i < 22; i++ ) {
                    printf("%.2X ",rx_buf[i]);
                }
                puts("");
                free(rx_buf);
                break;
            }
            case 'D':
                //  date code
                radio_perform_query("QD");
                break;
            case 'r':
            {
                //  'QF' Fmin, Fmax, Fstep as 32 bit numbers
                uint8_t data[2] = {0x51, 0x46};
                uint8_t *rx_buf = malloc(22 * sizeof(uint8_t));
                write_radio(data,rx_buf,22);
                uint32_t freql,freqh,freqs;
                memcpy(&freql,&data[2],sizeof(freql));
                memcpy(&freqh,&data[6],sizeof(freqh));
                memcpy(&freqs,&data[10],sizeof(freqs));
                printf("freq = %d,%d,%d",freql,freqh,freqs);
                free(rx_buf);
                break;
            }
		default:
			print_usage(argv[0]);
			break;
		}
	}
}

int main(int argc, char *argv[])
{
	int ret = 0;
	int fd;

    //  radio is on CSB, set up the RPi GPIO pins that
    //  control the aux CS
    hab_spi_set_cs(HAB_SPI_CSB);
    hab_spi_set_aux_gpio(RPI_V2_GPIO_P1_18, RPI_V2_GPIO_P1_22);
    
	parse_opts(argc, argv);
	return ret;
}