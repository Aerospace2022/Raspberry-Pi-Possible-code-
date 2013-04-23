#include "hab_spi.h"
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

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define BUF_SIZE(a) (sizeof(a) / sizeof(uint8_t))

static void pabort(const char *s)
{
	perror(s);
	abort();
}

static void print_usage(const char *prog)
{
    printf("Usage: %s [-c]\n", prog);
    puts(   "-c --chan\tRead from specified channel\n");
    exit(1);
}