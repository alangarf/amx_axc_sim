#ifndef CALENDAR__HEADER
#define CALENDAR__HEADER

void calendar_reset(void);
void calendar_update(void);
int calendar_ack(void);
unsigned int calendar_read(unsigned int address);
void calendar_write(unsigned int address, unsigned int value);

#endif /* CALENDAR__HEADER */
