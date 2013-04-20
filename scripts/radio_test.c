/*
 * SPI testing utility (using spidev driver)
 *
 * Copyright (c) 2007  MontaVista Software, Inc.
 * Copyright (c) 2007  Anton Vorontsov <avorontsov@ru.mvista.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * Cross-compile with cross-gcc -I/path/to/cross-kernel/include
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

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define BUF_SIZE(a) (sizeof(a) / sizeof(uint8_t))

#define XFR_USE_BCM2835_LIB 1

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
			" -M --rmem		read freq from memory loc\n"
			" -m --wmem		write active freq to memory loc\n"
			" -N --qname	read device name\n"
			" -D --qdate	read datecode\n"
			" -V --qvers	read software version\n"
			" -S --qser		read serial number\n"
			" -r --qfreq	read minimum and maximum freqs"
		);
	exit(1);
}

/*	Writes data to the radio with array of length */
static uint8_t write_radio(uint8_t *data, uint8_t *rd_buf, uint8_t length) {
    uint8_t *wr_buf = malloc(MESSAGE_LENGTH * sizeof(uint8_t));
    if( wr_buf == NULL ) pabort("unable to allocate memory for write buffer");
    //  pad our message with the required dummy character
    memcpy(memset(wr_buf,0x3F,MESSAGE_LENGTH),data,2);
    
	int ret = 0;
    
    #if XFR_USE_BCM2835_LIB
        bcm2835_spi_begin();
        bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);      // The default
        bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);                   // The default
        bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_4096);
        bcm2835_spi_chipSelect(BCM2835_SPI_CS0);                      // The default
        bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);      // the default
        bcm2835_spi_transfernb(wr_buf,rd_buf,MESSAGE_LENGTH);
        bcm2835_spi_end();
        
        bcm2835_delay(20);
        bcm2835_spi_begin();
        bcm2835_spi_transfernb(wr_buf,rd_buf,MESSAGE_LENGTH);
        bcm2835_spi_end();
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
    uint8_t *rx_buf = malloc(MESSAGE_LENGTH * sizeof(uint8_t));
    write_radio(data,rx_buf,MESSAGE_LENGTH);
    print_query_results(rx_buf);
    free(rx_buf);
}

/*  set the frequency to the value specified by f */
static void radio_set_frequency(uint32_t f) {
    unsigned char *p = (unsigned char *)&f;
    uint8_t *wr_buf = malloc(5 * sizeof(uint8_t));
    if( wr_buf == NULL ) pabort("unable to allocate memory for write buffer");
    wr_buf[0] = 'B';
    memcpy(&wr_buf[1],p,sizeof(f));
    
    uint8_t *rd_buf = malloc(5 * sizeof(uint8_t));
    
    bcm2835_spi_begin();
    bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);      // The default
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);                   // The default
    bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_4096);
    bcm2835_spi_chipSelect(BCM2835_SPI_CS0);                      // The default
    bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);      // the default
    bcm2835_spi_transfernb(wr_buf,rd_buf,5);
    bcm2835_spi_end();
}

static void radio_perform_memory_operation(uint8_t channel, char op) {
    
    if( channel > 15 ) pabort("Channel out of range")
    if( (op != 'M') && (op != 'm') )
        pabort("ERROR | channel out of range");
    unsigned char *p = (unsigned char *)&channel;
    uint8_t *wr_buf = malloc(2 * sizeof(uint8_t));
    if( wr_buf == NULL ) pabort("unable to allocate memory for write buffer");
    wr_buf[0] = op;
    memcpy(&wr_buf[1],p,sizeof(f));
    
    uint8_t *rd_buf = malloc(2 * sizeof(uint8_t));
    
    bcm2835_spi_begin();
    bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);      // The default
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);                   // The default
    bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_4096);
    bcm2835_spi_chipSelect(BCM2835_SPI_CS0);                      // The default
    bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);      // the default
    bcm2835_spi_transfernb(wr_buf,rd_buf,2);
    bcm2835_spi_end();
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
				//	temperature as signed 8 bit integer
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
                radio_perform_query("QD");
                break;
            case 'r':
            {
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

	parse_opts(argc, argv);
	return ret;
}