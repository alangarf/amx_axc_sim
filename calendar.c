#include "calendar.h"
#include "sim.h"
#include <stdio.h>

void calendar_reset(void)
{
}

void calendar_update(void) {
}

int calendar_ack(void) {
    return 0;
}

unsigned int calendar_read(unsigned int address) {
    //printf("CAL READ 0x%08x\n", address);
    return 0;
}

void calendar_write(unsigned int address, unsigned int value) {
    printf("CAL WRITE 0x%02x | DATA 0x%01x REG: 0%02x\n", value&0xff, value&0x01, (value >> 1)&0xff);
}
