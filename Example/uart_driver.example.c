/*
 * SCI.c
 *
 *  Created on: 2 мая 2017 г.
 *      Author: p.borisenko
 *      Hardware: tms320f2803x
 */

#include "SCI.h"
#include <stdlib.h>

#define SCI_LSPCLK  15000000

void (*tx_cb)(void);
void (*rx_cb)(void);
void (*rx_err_cb)(uint16_t errors);

__interrupt void tx_isr(void) {
    PieCtrlRegs.PIEACK.all= PIEACK_GROUP9;   // Acknowledge interrupt to PIE
    IER |= M_INT9;
    EINT;
#if (MODE_SCI == 1)
    if (SciaRegs.SCICTL2.bit.TXRDY) {
#elif (MODE_LIN == 1)
    if (LinaRegs.SCIFLR.bit.TXRDY) {
#endif
        if (tx_cb != NULL) {
            tx_cb();
        }
        //SciaRegs.SCIFFTX.bit.TXFFINTCLR= 1; // Clear TX FIFO interrupt flag
    }
}

__interrupt void rx_isr(void) {
    uint16_t temp;
    PieCtrlRegs.PIEACK.all= PIEACK_GROUP9;   // Acknowledge interrupt to PIE
    IER |= M_INT9;
    EINT;
#if (MODE_SCI == 1)
    if (SciaRegs.SCIRXST.bit.RXERROR) {
        temp= ((SciaRegs.SCIRXST.all)&0b00111100);
        if (rx_err_cb != NULL) {
            rx_err_cb(temp);
        }
    }
#elif (MODE_LIN == 1)
    // Error handle
#endif
#if (MODE_SCI == 1)
    if (SciaRegs.SCIRXST.bit.RXRDY) {
        //SciaRegs.SCIFFRX.bit.RXFFINTCLR= 1; // Clear RX FIFO interrupt flag
#elif (MODE_LIN == 1)
    if (LinaRegs.SCIFLR.bit.RXRDY) {
#endif
        if (rx_cb != NULL) {
            rx_cb();
        }
    }
#if (MODE_SCI == 1)
    if (SciaRegs.SCIRXST.bit.BRKDT) {

    }
#elif (MODE_LIN == 1)
    // Break handle
#endif
}

uint16_t sciGetRxReg(void) {
    return SCI_RX_REG;
}

void sciSetTxReg(uint16_t dat) {
    SCI_TX_REG= dat;
}

void sciInit(uint32_t baudrate, void (*tx_callback)(void), void (*rx_callback)(void)) {
    char dummy;
    uint32_t brr;
    tx_cb= tx_callback;
    rx_cb= rx_callback;

#if (MODE_SCI == 1)
    EALLOW;
    // Init GPIO
/* Enable internal pull-up for the selected pins */
// Pull-ups can be enabled or disabled disabled by the user.
// This will enable the pullups for the specified pins.

    GpioCtrlRegs.GPAPUD.bit.GPIO28 = 0;    // Enable pull-up for GPIO28 (SCIRXDA)
//  GpioCtrlRegs.GPAPUD.bit.GPIO7 = 0;     // Enable pull-up for GPIO7  (SCIRXDA)

    GpioCtrlRegs.GPAPUD.bit.GPIO29 = 0;    // Enable pull-up for GPIO29 (SCITXDA)
//  GpioCtrlRegs.GPAPUD.bit.GPIO12 = 0;    // Enable pull-up for GPIO12 (SCITXDA)

/* Set qualification for selected pins to asynch only */
// Inputs are synchronized to SYSCLKOUT by default.
// This will select asynch (no qualification) for the selected pins.

    GpioCtrlRegs.GPAQSEL2.bit.GPIO28 = 3;  // Asynch input GPIO28 (SCIRXDA)
//  GpioCtrlRegs.GPAQSEL1.bit.GPIO7 = 3;   // Asynch input GPIO7 (SCIRXDA)

/* Configure SCI-A pins using GPIO regs*/
// This specifies which of the possible GPIO pins will be SCI functional pins.

    GpioCtrlRegs.GPAMUX2.bit.GPIO28 = 1;   // Configure GPIO28 for SCIRXDA operation
//  GpioCtrlRegs.GPAMUX1.bit.GPIO7 = 2;    // Configure GPIO7  for SCIRXDA operation

    GpioCtrlRegs.GPAMUX2.bit.GPIO29 = 1;   // Configure GPIO29 for SCITXDA operation
//  GpioCtrlRegs.GPAMUX1.bit.GPIO12 = 2;   // Configure GPIO12 for SCITXDA operation

    // Init UART Module
    SciaRegs.SCIFFTX.bit.SCIRST= 1;
    DELAY_US(5);
    SciaRegs.SCIFFTX.bit.SCIRST= 0;

    // Put SCI into software reset mode
    SciaRegs.SCICTL1.bit.SWRESET= 0;

    // SCI Communication mode
    SciaRegs.SCICCR.bit.ADDRIDLE_MODE= 1;   // Address bit wakeup mode
    SciaRegs.SCICCR.bit.LOOPBKENA= 0;       // External communication mode
    SciaRegs.SCICCR.bit.PARITY= 0;          // Odd
    SciaRegs.SCICCR.bit.PARITYENA= 0;       // No parity
    SciaRegs.SCICCR.bit.SCICHAR= 0b111;     // 8-bit char
    SciaRegs.SCICCR.bit.STOPBITS= 0;        // 1 stop bit

    SciaRegs.SCICTL1.bit.RXERRINTENA= 1;    // Enable RX ERROR interrupt
    SciaRegs.SCICTL1.bit.SLEEP= 0;          // Sleep mode disabled
    SciaRegs.SCICTL1.bit.TXWAKE= 0;         // Wakeup method not selected
    SciaRegs.SCICTL1.bit.RXENA= 1;          // Enable RX pin
    SciaRegs.SCICTL1.bit.TXENA= 1;          // Enable TX pin

    SciaRegs.SCICTL2.bit.RXBKINTENA= 1;     // Enable RX/BK interrupt
    SciaRegs.SCICTL2.bit.TXINTENA= 1;       // Enable TX interrupt

    SciaRegs.SCIFFTX.bit.SCIFFENA= 0;       // Disable FIFO
    SciaRegs.SCIFFTX.bit.TXFFINT= 0;        // Disable TX FIFO interrupt
    SciaRegs.SCIFFTX.bit.TXFFIL= 1;         // Interrupt level is 1
    SciaRegs.SCIFFRX.bit.RXFFIENA= 0;       // Disable RX FIFO interrupt
    SciaRegs.SCIFFRX.bit.RXFFIL= 1;         // Interrupt level is 1

    SciaRegs.SCIFFCT.bit.FFTXDLY= 0;        // FIFO transfer delay is 0 baud cycles

    SciaRegs.SCIPRI.bit.FREE= 0;            // Complete current receive/transmit sequence before stopping
    SciaRegs.SCIPRI.bit.SOFT= 1;

    // Baud Rate Settings - 60MHz device
    brr= (SCI_LSPCLK/(baudrate<<3))-1;      // TODO: Check LSPCLK prescaler value
    SciaRegs.SCILBAUD= brr&0xFFFF;
    SciaRegs.SCIHBAUD= (brr>>16)&0xFFFF;

    // Release SCI from software reset state - End of Config
    SciaRegs.SCICTL1.bit.SWRESET= 1;

    // Attach interrupt handlers
    PieVectTable.SCIRXINTA= rx_isr;
    PieVectTable.SCITXINTA= tx_isr;

    EDIS;
    // Flush RX buffer
    while (SciaRegs.SCIRXST.bit.RXRDY) {
        dummy= SciaRegs.SCIRXBUF;
    }
#elif (MODE_LIN == 1)
    EALLOW;
    /* Enable internal pull-up for the selected pins */
    // Pull-ups can be enabled or disabled by the user.
    // This will enable the pullups for the specified pins.

    //  GpioCtrlRegs.GPAPUD.bit.GPIO9 = 0;      // Enable pull-up for GPIO9 (LIN TX)
    //  GpioCtrlRegs.GPAPUD.bit.GPIO14 = 0;     // Enable pull-up for GPIO14 (LIN TX)
    //  GpioCtrlRegs.GPAPUD.bit.GPIO18 = 0;     // Enable pull-up for GPIO18 (LIN TX)
    GpioCtrlRegs.GPAPUD.bit.GPIO22 = 0;     // Enable pull-up for GPIO22 (LIN TX)

    //  GpioCtrlRegs.GPADIR.bit.GPIO9 = 0;
    //  GpioCtrlRegs.GPADIR.bit.GPIO14 = 0;
    //  GpioCtrlRegs.GPADIR.bit.GPIO18 = 0;
    GpioCtrlRegs.GPADIR.bit.GPIO22 = 1;     // 1=OUTput,  0=INput

    //  GpioCtrlRegs.GPAPUD.bit.GPIO11 = 0;     // Enable pull-up for GPIO11 (LIN RX)
    //  GpioCtrlRegs.GPAPUD.bit.GPIO15 = 0;     // Enable pull-up for GPIO15 (LIN RX)
    //  GpioCtrlRegs.GPAPUD.bit.GPIO19 = 0;     // Enable pull-up for GPIO19 (LIN RX)
    GpioCtrlRegs.GPAPUD.bit.GPIO23 = 0;     // Enable pull-up for GPIO23 (LIN RX)

    //  GpioCtrlRegs.GPADIR.bit.GPIO11 = 0;
    //  GpioCtrlRegs.GPADIR.bit.GPIO15 = 0;
    //  GpioCtrlRegs.GPADIR.bit.GPIO19 = 0;
    GpioCtrlRegs.GPADIR.bit.GPIO23 = 0;     // 1=OUTput,  0=INput

    /* Set qualification for selected pins to asynch only */
    // Inputs are synchronized to SYSCLKOUT by default.
    // This will select asynch (no qualification) for the selected pins.

    //  GpioCtrlRegs.GPAQSEL1.bit.GPIO11 = 3;  // Asynch input GPIO11 (LINRXA)
    //  GpioCtrlRegs.GPAQSEL1.bit.GPIO15 = 3;  // Asynch input GPIO15 (LINRXA)
    //  GpioCtrlRegs.GPAQSEL2.bit.GPIO19 = 3;  // Asynch input GPIO19 (LINRXA)
    GpioCtrlRegs.GPAQSEL2.bit.GPIO23 = 3;  // Asynch input GPIO23 (LINRXA)


    /* Configure LIN-A pins using GPIO regs*/
    // This specifies which of the possible GPIO pins will be LIN pins.
    // Only one set of pins should be enabled at any time for LIN operation.

    //  GpioCtrlRegs.GPAMUX1.bit.GPIO9 = 2;    // Configure GPIO9 for LIN TX operation (2-Enable,0-Disable)
    //  GpioCtrlRegs.GPAMUX1.bit.GPIO14 = 2;   // Configure GPIO14 for LIN TX operation (2-Enable,0-Disable)
    //  GpioCtrlRegs.GPAMUX2.bit.GPIO18 = 2;   // Configure GPIO18 for LIN TX operation (2-Enable,0-Disable)
    GpioCtrlRegs.GPAMUX2.bit.GPIO22 = 3;   // Configure GPIO19 for LIN TX operation  (3-Enable,0-Disable)

    //  GpioCtrlRegs.GPAMUX1.bit.GPIO11 = 2;   // Configure GPIO11 for LIN RX operation  (2-Enable,0-Disable)
    //  GpioCtrlRegs.GPAMUX1.bit.GPIO15 = 2;   // Configure GPIO15 for LIN RX operation (2-Enable,0-Disable)
    //  GpioCtrlRegs.GPAMUX2.bit.GPIO19 = 2;   // Configure GPIO19 for LIN RX operation (2-Enable,0-Disable)
    GpioCtrlRegs.GPAMUX2.bit.GPIO23 = 3;   // Configure GPIO23 for LIN RX operation (3-Enable,0-Disable)

    //Reset module and release reset.
    LinaRegs.SCIGCR0.bit.RESET= 0;
    DELAY_US(5);
    LinaRegs.SCIGCR0.bit.RESET= 1;
    //LIN into software reset mode
    LinaRegs.SCIGCR1.bit.SWnRST= 0;
    //Select SCI Mode
    LinaRegs.SCIGCR1.all= 0x0302002A;
    //LinaRegs.SCIGCR1.bit.LINMODE= 0;
    //Configure SCI mode
    //LinaRegs.SCIGCR1.bit.SLEEP= 0;          // Sleep mode disabled
    //LinaRegs.SCIGCR1.bit.CLK_MASTER= 1;     // SCI clock enabled
    //LinaRegs.SCIGCR1.bit.CONT= 1;           // Continue on Emulation suspend
    //LinaRegs.SCIGCR1.bit.LOOPBACK= 0;       // External communication mode
    //LinaRegs.SCIGCR1.bit.STOP= 0;           // 1 stop bit
    //LinaRegs.SCIGCR1.bit.PARITYENA= 0;      // Parity checking disabled
    //LinaRegs.SCIGCR1.bit.PARITY= 1;         // Parity even
    //LinaRegs.SCIGCR1.bit.TIMINGMODE= 1;     // Async mode
    //LinaRegs.SCIGCR1.bit.COMMMODE= 0;       // Idle-line mode
    //LinaRegs.SCIGCR1.bit.RXENA= 1;          // Enable RX pin
    //LinaRegs.SCIGCR1.bit.TXENA= 1;          // Enable TX pin
    //Set SCI interrupts
    LinaRegs.SCICLEARINT.all= 0xFFFFFFFF;
    LinaRegs.SCICLEARINTLVL.bit.CLRTXINTLVL= 1; // TX int set to INT0
    LinaRegs.SCISETINTLVL.bit.SETRXINTLVL= 1;   // RX int set to INT1
    //Baud Rate Settings - 60MHz device
    // Можно посчитать вот так:
    uint32_t a= 30000000/baudrate;  // 30000000 это SYSCLOCK/2
    uint32_t p= (a>>4)-1;
    uint16_t m= (a-((a>>4)<<4));

    switch (baudrate) {
        case 1200:
            LinaRegs.BRSR.bit.SCI_LIN_PSL= 1561;
            LinaRegs.BRSR.bit.M= 8;
            break;
        case 2400:
            LinaRegs.BRSR.bit.SCI_LIN_PSL= 780;
            LinaRegs.BRSR.bit.M= 4;
            break;
        case 4800:
            LinaRegs.BRSR.bit.SCI_LIN_PSL= 389;
            LinaRegs.BRSR.bit.M= 10;
            break;
        case 9600:
            LinaRegs.BRSR.bit.SCI_LIN_PSL= 194;
            LinaRegs.BRSR.bit.M= 5;
            break;
        case 19200:
            LinaRegs.BRSR.bit.SCI_LIN_PSL= 96;
            LinaRegs.BRSR.bit.M= 11;
            break;
        case 38400:
            LinaRegs.BRSR.bit.SCI_LIN_PSL= 47;
            LinaRegs.BRSR.bit.M= 13;
            break;
        case 57600:
            LinaRegs.BRSR.bit.SCI_LIN_PSL= 31;
            LinaRegs.BRSR.bit.M= 9;
            break;
        case 115200:
            LinaRegs.BRSR.bit.SCI_LIN_PSL= 15;
            LinaRegs.BRSR.bit.M= 4;
            break;
        default:
            // TODO: Calculate baudrate
            break;
    }

    //LIN Character Size and Length
    LinaRegs.SCIFORMAT.all= 0x00000007;
//    LinaRegs.SCIFORMAT.bit.LENGTH= 0;       // 1 char tx/rx
//    LinaRegs.SCIFORMAT.bit.CHAR= 7;         // 8-bit char
    //IODFT Configuarations
    LinaRegs.IODFTCTRL.all= 0x00000000;
//    LinaRegs.IODFTCTRL.bit.IODFTENA = 0x0;  // IODFT testing module disabled
//    LinaRegs.IODFTCTRL.bit.LPBENA    = 0;   // IODFT loopback disabled
    //Release   SCI from software reset state - End of Config
    LinaRegs.SCIGCR1.bit.SWnRST= 1;

    PieVectTable.LIN0INTA= tx_isr;     // Attach ISR to interrupt vector
    PieVectTable.LIN1INTA= rx_isr;     // Attach ISR to interrupt vector
    EDIS;
    LinaRegs.SCISETINT.bit.SETRXINT= 1;       // Enable RX interrupt
    // Enable LININT0 in PIE
    PieCtrlRegs.PIEIER9.bit.INTx3= 1;       // Enable INT 9.3 in the PIE
    // Enable LININT1 in PIE
    PieCtrlRegs.PIEIER9.bit.INTx4= 1;       // Enable INT 9.4 in the PIE

    // Flush RX buffer
    while (LinaRegs.SCIFLR.bit.RXRDY) {
        dummy= LinaRegs.SCIRD;
    }
    IER |= M_INT9;
#endif
}

void sciTxEnable(void) {
#if (MODE_SCI == 1)
    //SciaRegs.SCIFFTX.bit.TXFFINT= 1;
    SciaRegs.SCICTL2.bit.TXINTENA= 1;
#elif (MODE_LIN == 1)
    LinaRegs.SCISETINT.bit.SETTXINT= 1;
#endif
}

void sciTxDisable(void) {
#if (MODE_SCI == 1)
    //SciaRegs.SCIFFTX.bit.TXFFINT= 0;
    SciaRegs.SCICTL2.bit.TXINTENA= 0;
#elif (MODE_LIN == 1)
    LinaRegs.SCICLEARINT.bit.CLRTXINT= 1;
    LinaRegs.SCISETINT.bit.SETRXINT= 1;
#endif
}



