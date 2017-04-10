#include <stdio.h>
#include <time.h>
#include "scc2691.h"
#include "sim.h"

scc2691_core uarts[2];

void scc2691_reset(void)
{
    for(int i = 0; i < 2; i++) {
        uarts[i].id = i;

        uarts[i].m1 = 0x0;
        uarts[i].m1_flag = 0x0;
        uarts[i].m2 = 0x0; 

        uarts[i].csr  = 0x0;
        uarts[i].cr   = 0xA;
        uarts[i].thr  = 0x0;
        uarts[i].acr  = 0x0;
        uarts[i].imr  = 0x0;
        uarts[i].ctur = 0x0;
        uarts[i].ctlr = 0x0;

        uarts[i].sr  = 0x0;
        uarts[i].rhr = 0x0;
        uarts[i].isr = 0x0;
        uarts[i].ctu = 0x0;
        uarts[i].ctl = 0x0;

        uarts[i].cnt_flag = 0;
        uarts[i].cnt_value = 0;

        uarts[i].tx = 0;
        uarts[i].tx_last = time(NULL);
        int_controller_clear(1);
    }
}

void scc2691_update(void) {

    // ACR set to 4MHz clk source for timer, MPO is RTSN
    // reset the RX, disable RX and flush fifo
    // reset the TX
    // TX/RX baud rate 38.4 or 19.2 (likely neither as 4MHz xtal)
    // reset MR pointer to MR1
    // 8bits no parity, RxRDY INT select, Rx RTS Control off
    // 1 stop bit, CTS Enable Tx off, Tx RTS Control off
    // enable TX and RX
    // set 0x48 into counter upper reg
    // set 0x00 into counter lower reg
    // start counter ***
    // mask interrupt RxRDY and Counter Ready

    for(int i = 0; i < 2; i++) {
        scc2691_core* uart = &uarts[i];

        // service counters
        if (uart->cnt_flag) {
            uart->cnt_value--;
        }

        if (uart->cnt_value == 0 && uart->cnt_flag) {
            // Trigger a counter interrupt if counter interrupt is masked
            if((uart->imr >> 4) & 1) {
                // set the counter ISR bit
                uart->isr |= 1 << 4;
            }

            uart->cnt_value = ((uart->ctur << 8) + uart->ctlr)&0xFFFF;
        }

        // serivce interrupts
        if (uart->isr & 1 && uart->imr & 1) {
            // TxRDY interrupt
            int_controller_set(1);
        } else {
            if ((time(NULL) - uart->tx_last) >= 1)
            {
                uart->isr |= 1;
                uart->sr &= ~(1 << 2);

                uart->tx = uart->thr;
            }
        }

        if ((uart->isr >> 4) & 1) {
            // Counter Ready interrupt
            //int_controller_set(1);
        } else {
            //int_controller_clear(1);
        }
    }
}

void scc2691_info(void)
{
    for(int i = 0; i < 2; i++) {
        scc2691_core* uart = &uarts[i];

        printf("id: %02x\n", uart->id);
        printf("m1: %02x\n", uart->m1);
        printf("m2: %02x\n", uart->m2);
        printf("\n");
    }
}

void scc2691_write(unsigned int address, unsigned int value)
{
    scc2691_core* uart = &uarts[address & 0x1];
    int reg = (address >> 1) & 0x07;

    switch(reg)
    {
        case 0x00:
            if(uart->m1_flag == 0) {
                uart->m1 = value&0xff;
                uart->m1_flag = 1;
                printf("UART%d WRITE 0x%02x to M1\n", uart->id, uart->m1);
            }
            else {
                uart->m2 = value&0xff;
                printf("UART%d WRITE 0x%02x to M2\n", uart->id, uart->m2);
            }
            break;

        case 0x01:
            uart->csr = value&0xff;
            printf("UART%d WRITE 0x%02x to CSR\n", uart->id, value&0xff);
            break;

        case 0x02:
            uart->cr = value&0xff;

            printf("UART%d ", uart->id);

            printf("( TX:%d%d ", (value >> 3)&0x1, (value >> 2)&0x1);
            printf("RX:%d%d ) ", (value >> 1)&0x1, value&0x1);

            // disable TX
            if((value >> 3) & 1) {
                // clear the TxRDY, TxEMT flag on TX disable
                uart->isr &= ~(1);      // TxRDY
                uart->isr &= ~(1 << 1); // TxEMT

                uart->sr &= ~(1 << 2);  // TxRDY
                uart->sr &= ~(1 << 3);  // TxEMT
            }

            // enable TX
            if((value >> 2) & 1) {
                // on TX enable TxRDY and TxEMT are set
                uart->isr |= 1;         // TxRDY
                uart->sr |= 1 << 2;     // TxRDY

                uart->isr |= 1 << 1;    // TxEMT
                uart->sr |= 1 << 3;     // TxEMT
                
                //int_controller_set(1);
            }

            switch((value >> 4)&0x0f)
            {
                case 0x0:
                    // no command
                    printf("NOP\n");
                    break;
                case 0x1:
                    printf("RESET MR POINTER\n");
                    break;
                case 0x2:
                    printf("RESET RX\n");
                    break;
                case 0x3:
                    printf("RESET TX\n");
                    break;
                case 0x4:
                    printf("RESET ERROR STATUS\n");
                    break;
                case 0x5:
                    printf("RESET BREAK CHANGE INT\n");
                    break;
                case 0x6:
                    printf("START BREAK\n");
                    break;
                case 0x7:
                    printf("STOP BREAK\n");
                    break;
                case 0x8:
                    printf("START COUNTER\n");
                    // set the counter value
                    uart->cnt_value = ((uart->ctur << 8) + uart->ctlr)&0xFFFF;
                    // set the counter flag
                    uart->cnt_flag = 1;
                    break;
                case 0x9:
                    printf("STOP COUNTER\n");
                    // clear the isr bit for the counter interrupt
                    uart->isr &= ~(1 << 4);
                    // reset the counter flag
                    uart->cnt_flag = 0;
                    break;
                case 0xa:
                    printf("ASSERT RTSN\n");
                    break;
                case 0xb:
                    printf("NEGATE RTSN\n");
                    break;
                case 0xc:
                    printf("RESET MPI CHANGE INT\n");
                    break;
                default:
                    printf("ERRRRRRRRRRR SOMETHING WEIRD!!\n");
                    break;

            }

            break;

        case 0x03:
            uart->thr = value&0xff;

            // clear TxRDY, character received
            uart->isr &= ~(1);
            uart->sr &= ~(1 << 2);

            // set tx_last
            uart->tx_last = time(NULL);

            printf("UART%d WRITE 0x%02x to THR\n", uart->id, value&0xff);
            break;

        case 0x04:
            uart->acr = value&0xff;
            printf("UART%d WRITE 0x%02x to ACR\n", uart->id, value&0xff);
            break;

        case 0x05:
            uart->imr = value&0xff;
            printf("UART%d WRITE 0x%02x to IMR\n", uart->id, value&0xff);
            break;

        case 0x06:
            uart->ctur = value&0xff;
            printf("UART%d WRITE 0x%02x to CTUR\n", uart->id, value&0xff);
            break;

        case 0x07:
            uart->ctlr = value&0xff;
            printf("UART%d WRITE 0x%02x to CTLR\n", uart->id, value&0xff);
            break;

        default:
            exit_error("Attempted to write %04x to UART address %08x", value&0xff, address);
            break;
    }

    fflush(stdout);
    return;
}

unsigned int scc2691_read(unsigned int address)
{
    scc2691_core* uart = &uarts[address & 0x1];
    int reg = (address >> 1) & 0x07;

    switch(reg)
    {
        case 0x00:
            if(uart->m1_flag == 0) {
                printf("UART%d READ 0x%02x FROM M1\n", uart->id, uart->m1);
                uart->m1_flag = 1;
                return uart->m1;
            }
            else {
                printf("UART%d READ 0x%02x FROM M2\n", uart->id, uart->m2);
                return uart->m2;
            }

        case 0x01:
            printf("UART%d READ 0x%02x FROM SR\n", uart->id, uart->sr);
            return uart->sr;

        case 0x02:
            // BRG Test function not implemented
            return 0;

        case 0x03:
            printf("UART%d READ 0x%02x FROM RHR\n", uart->id, uart->rhr);
            return uart->rhr;

        case 0x04:
            // 1X/16X Test function not implemented
            return 0;

        case 0x05:
            printf("UART%d READ 0x%02x FROM ISR\n", uart->id, uart->isr);
            return uart->isr;

        case 0x06:
            printf("UART%d READ 0x%02x FROM CTU\n", uart->id, uart->ctu);
            return uart->ctu;

        case 0x07:
            printf("UART%d READ 0x%02x FROM CTL\n", uart->id, uart->ctl);
            return uart->ctl;

        default:
            exit_error("Attempted to read UART address %08x", address);
            break;
    }

    return 0;
}
