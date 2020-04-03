/*
 * SCI.h
 *
 *  Created on: 2 мая 2017 г.
 *      Author: p.borisenko
 *      Hardware: tms320f2803x
 */

#ifndef INCLUDE_SCI_H_
#define INCLUDE_SCI_H_

#include <DSP28x_Project.h>
#include <stdint.h>

#define MODE_LIN    1
#define MODE_SCI    0


#if (MODE_SCI == 1)
#define SCI_RX_REG (SciaRegs.SCIRXBUF)
#define SCI_TX_REG (SciaRegs.SCITXBUF)
#elif (MODE_LIN == 1)
#define SCI_RX_REG (LinaRegs.SCIRD)
#define SCI_TX_REG (LinaRegs.SCITD)
#endif

void sciInit(uint32_t baudrate, void (*tx_callback)(void), void (*rx_callback)(void));
void sciTxEnable(void);
void sciTxDisable(void);
uint16_t sciGetRxReg(void);
void sciSetTxReg(uint16_t dat);


#endif /* INCLUDE_SCI_H_ */
