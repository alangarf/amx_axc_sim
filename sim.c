#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include "terminal.h"
#include "sim.h"
#include "m68k.h"
#include "scc2691.h"
#include "calendar.h"

/* Memory-mapped IO ports */
//#define INPUT_ADDRESS 0x50000
//#define OUTPUT_ADDRESS 0x60000

#define RAM1 0x20000
#define RAM2 0x30000

#define UARTS 0x40000
#define DIPS 0x70000

#define CAL_IN 0x50000
#define CAL_OUT 0x60000

/* IRQ connections */
#define IRQ_NMI_DEVICE 7
#define IRQ_INPUT_DEVICE 2
#define IRQ_OUTPUT_DEVICE 1

/* Time between characters sent to output device (seconds) */
#define OUTPUT_DEVICE_PERIOD 1

/* ROM and RAM sizes */
#define MAX_ROM 0xffff
#define MAX_RAM 0xffff

/* Prototypes */

void nmi_device_reset(void);
void nmi_device_update(void);
int nmi_device_ack(void);

// void input_device_reset(void);
// void input_device_update(void);
// int input_device_ack(void);
// unsigned int input_device_read(void);
// void input_device_write(unsigned int value);

//void output_device_reset(void);
//void output_device_update(void);
//int output_device_ack(void);
//unsigned int output_device_read(void);
//void output_device_write(unsigned int value);

void int_controller_set(unsigned int value);
void int_controller_clear(unsigned int value);

/* Data */
unsigned int g_key = 0;
unsigned int g_keyhit = 0;
unsigned int g_quit = 0;                        /* 1 if we want to quit */
unsigned int g_nmi = 0;                         /* 1 if nmi pending */

//int          g_input_device_value = -1;         /* Current value in input device */

//unsigned int g_output_device_ready = 0;         /* 1 if output device is ready */
//time_t       g_output_device_last_output;       /* Time of last char output */

unsigned int g_int_controller_pending = 0;      /* list of pending interrupts */
unsigned int g_int_controller_highest_int = 0;  /* Highest pending interrupt */

unsigned char g_rom[MAX_ROM+1];                 /* ROM */
unsigned char g_ram1[MAX_RAM+1];                /* RAM1 */
unsigned char g_ram2[MAX_RAM+1];                /* RAM2 */
unsigned char g_dips[4];                        /* DIPS */

unsigned int  g_fc;                             /* Current function code from CPU */
 
/* Exit with an error message.  Use printf syntax. */
void exit_error(char* fmt, ...)
{
    static int guard_val = 0;
    char buff[100];
    unsigned int pc;
    va_list args;

    if(guard_val)
        return;
    else
        guard_val = 1;

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
    pc = m68k_get_reg(NULL, M68K_REG_PPC);
    m68k_disassemble(buff, pc, M68K_CPU_TYPE_68000);
    fprintf(stderr, "At %04x: %s\n", pc, buff);

    exit(EXIT_FAILURE);
}

/* Read data from RAM, ROM, or a device */
unsigned int cpu_read_word_dasm(unsigned int address)
{
    if(address > MAX_ROM)
        exit_error("Disassembler attempted to read word from ROM address %08x", address);
    return READ_WORD(g_rom, address);
}

unsigned int cpu_read_long_dasm(unsigned int address)
{
    if(address > MAX_ROM)
        exit_error("Dasm attempted to read long from ROM address %08x", address);
    return READ_LONG(g_rom, address);
}

unsigned int cpu_read_byte(unsigned int address)
{
    switch(address & 0xF0000)
    {
        case 0x00000:
            return READ_BYTE(g_rom, address);
        case RAM1:
            return READ_BYTE(g_ram1, address - RAM1);
        case RAM2:
            return READ_BYTE(g_ram2, address - RAM2);
        case DIPS:
            return READ_BYTE(g_dips, address - DIPS);
        case UARTS:
            return scc2691_read(address);
        case CAL_IN:
            return calendar_read(address);
        //case OUTPUT_ADDRESS:
        //    return output_device_read();
    }

    exit_error("Attempted to read byte from RAM address %08x", address);
    return -1;
}

unsigned int cpu_read_word(unsigned int address)
{
    switch(address & 0xF0000)
    {
        case 0x00000:
            return READ_WORD(g_rom, address);
        case RAM1:
            return READ_WORD(g_ram1, address - RAM1);
        case RAM2:
            return READ_WORD(g_ram2, address - RAM2);
        case DIPS:
            return READ_WORD(g_dips, address - DIPS);
        case UARTS:
            return scc2691_read(address);
        case CAL_IN:
            return calendar_read(address);
        //case OUTPUT_ADDRESS:
        //    return output_device_read();
    }

    exit_error("Attempted to read word from RAM address %08x", address);
    return -1;
}

unsigned int cpu_read_long(unsigned int address)
{
    switch(address & 0xF0000)
    {
        case 0x00000:
            return READ_LONG(g_rom, address);
        case RAM1:
            return READ_LONG(g_ram1, address - RAM1);
        case RAM2:
            return READ_LONG(g_ram2, address - RAM2);
        case DIPS:
            return READ_LONG(g_dips, address - DIPS);
        case UARTS:
            return scc2691_read(address);
        case CAL_IN:
            return calendar_read(address);
        //case OUTPUT_ADDRESS:
        //    return output_device_read();
    }

    exit_error("Attempted to read long from RAM address %08x", address);
    return -1;
}


/* Write data to RAM or a device */
void cpu_write_byte(unsigned int address, unsigned int value)
{
    switch(address & 0xF0000)
    {
        case RAM1:
            WRITE_BYTE(g_ram1, address - RAM1, value);
            return;
        case RAM2:
            WRITE_BYTE(g_ram2, address - RAM2, value);
            return;
        case DIPS:
            WRITE_BYTE(g_dips, address - DIPS, value);
            return;
        case UARTS:
            scc2691_write(address, value);
            return;
        case CAL_OUT:
            calendar_write(address, value);
            return;
    }

    exit_error("Attempted to write byte %02x to RAM address %08x", value&0xff, address);
}

void cpu_write_word(unsigned int address, unsigned int value)
{
    switch(address & 0xF0000)
    {
        case RAM1:
            WRITE_WORD(g_ram1, address - RAM1, value);
            return;
        case RAM2:
            WRITE_WORD(g_ram2, address - RAM2, value);
            return;
        case DIPS:
            WRITE_WORD(g_dips, address - DIPS, value);
            return;
        case UARTS:
            scc2691_write(address, value);
            return;
        case CAL_OUT:
            calendar_write(address, value);
            return;
    }

    exit_error("Attempted to write word %04x to RAM address %08x", value&0xffff, address);
}

void cpu_write_long(unsigned int address, unsigned int value)
{
    switch(address & 0xF0000)
    {
        case RAM1:
            WRITE_LONG(g_ram1, address - RAM1, value);
            return;
        case RAM2:
            WRITE_LONG(g_ram2, address - RAM2, value);
            return;
        case DIPS:
            WRITE_LONG(g_dips, address - DIPS, value);
            return;
        case UARTS:
            scc2691_write(address, value);
            return;
        case CAL_OUT:
            calendar_write(address, value);
            return;
    }

    exit_error("Attempted to write long %08x to RAM address %08x", value, address);
}


/* Called when the CPU pulses the RESET line */
void cpu_pulse_reset(void)
{
    //nmi_device_reset();
    //output_device_reset();
    //input_device_reset();
}


/* Called when the CPU changes the function code pins */
void cpu_set_fc(unsigned int fc)
{
    g_fc = fc;
}


/* Called when the CPU acknowledges an interrupt */
int cpu_irq_ack(int level)
{
    switch(level)
    {
        case 1:
            printf("ACK!\n");
            fflush(stdout);
            return M68K_INT_ACK_AUTOVECTOR;
            break;
        //case IRQ_NMI_DEVICE:
        //    return nmi_device_ack();
        //case IRQ_INPUT_DEVICE:
        //	return input_device_ack();
        //case IRQ_OUTPUT_DEVICE:
        //  return output_device_ack();
    }
    return M68K_INT_ACK_SPURIOUS;
}


/* Implementation for the NMI device */
/*
void nmi_device_reset(void)
{
    g_nmi = 0;
}

void nmi_device_update(void)
{
    if(g_nmi)
    {
        g_nmi = 0;
        int_controller_set(IRQ_NMI_DEVICE);
    }
}

int nmi_device_ack(void)
{
    //printf("\nNMI\n");fflush(stdout);
    //int_controller_clear(IRQ_NMI_DEVICE);
    return M68K_INT_ACK_AUTOVECTOR;
}
*/


/* Implementation for the input device */
/*
   void input_device_reset(void)
   {
   g_input_device_value = -1;
   int_controller_clear(IRQ_INPUT_DEVICE);
   }

   void input_device_update(void)
   {
   if(g_input_device_value >= 0)
   int_controller_set(IRQ_INPUT_DEVICE);
   }

   int input_device_ack(void)
   {
   return M68K_INT_ACK_AUTOVECTOR;
   }

   unsigned int input_device_read(void)
   {
   int value = g_input_device_value > 0 ? g_input_device_value : 0;
   int_controller_clear(IRQ_INPUT_DEVICE);
   g_input_device_value = -1;
   return value;
   }

   void input_device_write(unsigned int value)
   {
   }
   */


/* Implementation for the output device */
/*
void output_device_reset(void)
{
    g_output_device_last_output = time(NULL);
    g_output_device_ready = 0;
    int_controller_clear(IRQ_OUTPUT_DEVICE);
}

void output_device_update(void)
{
    if(!g_output_device_ready)
    {
        if((time(NULL) - g_output_device_last_output) >= OUTPUT_DEVICE_PERIOD)
        {
            g_output_device_ready = 1;
            int_controller_set(IRQ_OUTPUT_DEVICE);
        }
    }
}

int output_device_ack(void)
{
    return M68K_INT_ACK_AUTOVECTOR;
}

unsigned int output_device_read(void)
{
    int_controller_clear(IRQ_OUTPUT_DEVICE);
    return 0;
}

void output_device_write(unsigned int value)
{
    char ch;
    if(g_output_device_ready)
    {
        ch = value & 0xff;
        printf("%c", ch);
        g_output_device_last_output = time(NULL);
        g_output_device_ready = 0;
        int_controller_clear(IRQ_OUTPUT_DEVICE);
    }
}
*/

/* Implementation for the interrupt controller */
void int_controller_set(unsigned int value)
{
    unsigned int old_pending = g_int_controller_pending;

    g_int_controller_pending |= (1<<value);

    if(old_pending != g_int_controller_pending && value > g_int_controller_highest_int)
    {
        g_int_controller_highest_int = value;
        m68k_set_irq(g_int_controller_highest_int);
    }
}

void int_controller_clear(unsigned int value)
{
    g_int_controller_pending &= ~(1<<value);

    for(g_int_controller_highest_int = 7;g_int_controller_highest_int > 0;g_int_controller_highest_int--)
        if(g_int_controller_pending & (1<<g_int_controller_highest_int))
            break;

    m68k_set_irq(g_int_controller_highest_int);
}

/* Disassembler */
void make_hex(char* buff, unsigned int pc, unsigned int length)
{
    char* ptr = buff;

    for(;length>0;length -= 2)
    {
        sprintf(ptr, "%04x", cpu_read_word(pc));
        pc += 2;
        ptr += 4;
        if(length > 2)
            *ptr++ = ' ';
    }
}

void disassemble_program()
{
    unsigned int pc;
    unsigned int instr_size;
    char buff[100];
    char buff2[100];

    pc = cpu_read_long(4);

    while(pc <= MAX_ROM)
    {
        instr_size = m68k_disassemble(buff, pc, M68K_CPU_TYPE_68000);
        make_hex(buff2, pc, instr_size);
        printf("%03x: %-20s: %s\n", pc, buff2, buff);
        pc += instr_size;
    }
    fflush(stdout);
}

void cpu_instr_callback()
{
    return;
    /* The following code would print out instructions as they are executed */
    static char buff[100];
    static char buff2[100];
    static unsigned int pc;
    static unsigned int instr_size;

    pc = m68k_get_reg(NULL, M68K_REG_PC);
    instr_size = m68k_disassemble(buff, pc, M68K_CPU_TYPE_68000);
    make_hex(buff2, pc, instr_size);
    printf("E %03x: %-20s: %s\n", pc, buff2, buff);
    fflush(stdout);
}



/* The main loop */
int main(int argc, char* argv[])
{
    FILE* fhandle;

    if(argc != 2)
    {
        printf("Usage: sim <program file>\n");
        exit(-1);
    }

    if((fhandle = fopen(argv[1], "rb")) == NULL)
        exit_error("Unable to open %s", argv[1]);

    if(fread(g_rom, 1, MAX_ROM+1, fhandle) <= 0)
        exit_error("Error reading %s", argv[1]);

    /*disassemble_program();*/
    /*return 0;*/

    m68k_init();
    m68k_set_cpu_type(M68K_CPU_TYPE_68000);
    m68k_pulse_reset();

    scc2691_reset();
    calendar_reset();
    //input_device_reset();
    //output_device_reset();
    //nmi_device_reset();

    g_quit = 0;

    nonblock(NB_ENABLE);
    while(!g_quit)
    {
        g_keyhit=kbhit();
        if (g_keyhit != 0)
        {
            g_key=fgetc(stdin);
            g_quit = (g_key=='') ? 1 : 0;
        }

        // Our loop requires some interleaving to allow us to update the
        // input, output, and nmi devices.

        // Values to execute determine the interleave rate.
        // Smaller values allow for more accurate interleaving with multiple
        // devices/CPUs but is more processor intensive.
        // 100000 is usually a good value to start at, then work from there.

        // Note that I am not emulating the correct clock speed!
        m68k_execute(10);

        scc2691_update();
        calendar_update();

        //output_device_update();
        //input_device_update();
        
        //nmi_device_update();
    }
    nonblock(NB_DISABLE);

    return 0;
}

