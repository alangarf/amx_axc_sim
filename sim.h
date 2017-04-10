#ifndef SIM__HEADER
#define SIM__HEADER

/* Prototypes */
unsigned int cpu_read_byte(unsigned int address);
unsigned int cpu_read_word(unsigned int address);
unsigned int cpu_read_long(unsigned int address);

void cpu_write_byte(unsigned int address, unsigned int value);
void cpu_write_word(unsigned int address, unsigned int value);
void cpu_write_long(unsigned int address, unsigned int value);

void cpu_pulse_reset(void);
void cpu_set_fc(unsigned int fc);
int  cpu_irq_ack(int level);
void cpu_instr_callback();

void exit_error(char* fmt, ...);

void int_controller_set(unsigned int value);
void int_controller_clear(unsigned int value);

/* Read/write macros */
#define READ_BYTE(BASE, ADDR) (BASE)[ADDR]
#define READ_WORD(BASE, ADDR) (((BASE)[ADDR]<<8) |			\
							  (BASE)[(ADDR)+1])
#define READ_LONG(BASE, ADDR) (((BASE)[ADDR]<<24) |			\
							  ((BASE)[(ADDR)+1]<<16) |		\
							  ((BASE)[(ADDR)+2]<<8) |		\
							  (BASE)[(ADDR)+3])

#define WRITE_BYTE(BASE, ADDR, VAL) (BASE)[ADDR] = (VAL)&0xff
#define WRITE_WORD(BASE, ADDR, VAL) (BASE)[ADDR] = ((VAL)>>8) & 0xff;		\
									(BASE)[(ADDR)+1] = (VAL)&0xff
#define WRITE_LONG(BASE, ADDR, VAL) (BASE)[ADDR] = ((VAL)>>24) & 0xff;		\
									(BASE)[(ADDR)+1] = ((VAL)>>16)&0xff;	\
									(BASE)[(ADDR)+2] = ((VAL)>>8)&0xff;		\
									(BASE)[(ADDR)+3] = (VAL)&0xff

#endif /* SIM__HEADER */
