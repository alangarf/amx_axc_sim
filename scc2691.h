#ifndef SCC2691__HEADER
#define SCC2691__HEADER

void scc2691_reset(void);
void scc2691_update(void);
int scc2691_ack(void);

unsigned int scc2691_read(unsigned int reg);
void scc2691_write(unsigned int address, unsigned int value);

void scc2691_info(void);

unsigned int uarts_device_read(unsigned int address);

typedef struct
{
    unsigned char id;       // The id of the UART

    unsigned char m1;       // M1 register
    unsigned char m1_flag;  // Flag to indicate M1 has been written to
    unsigned char m2;       // M2 register

    unsigned char csr;      // CSR register
    unsigned char cr;       // CR register
    unsigned char thr;      // THR register
    unsigned char acr;      // ACR register
    unsigned char imr;      // IMR register
    unsigned char ctur;     // CTUR register
    unsigned char ctlr;     // CTLR register

    unsigned char sr;       // SR register
    unsigned char rhr;      // RHR register
    unsigned char isr;      // ISR register
    unsigned char ctu;      // CTU register
    unsigned char ctl;      // CTL register

    unsigned char cnt_flag; // Counter start/stop flag
    unsigned int cnt_value; // Counter value

    unsigned int tx_last;   // tx last output time
    unsigned char tx;       // tx character

} scc2691_core;

#endif /* SCC2691__HEADER */
