/*
 * proto_bindings.h
 *
 *  Created on: 12 мая 2017 г.
 *      Author: p.borisenko
 */

#ifndef INCLUDE_PROTO_BINDINGS_H_
#define INCLUDE_PROTO_BINDINGS_H_

/*
 * @brief: Hardware binding layer
 *  Used resources:
 *      #   Name    Quantity    Instances
 *      1   UART    1           LINA
 *      2   TIMER   1           TPM2
 */

// Hardware-specific include
#include "SCI.h"
#include "modbus_timer.h"

// Binding with hardware layer
// Read register (return value)
#define MODBUS_UART_RX                              sciGetRxReg
// Write register (accept value)
#define MODBUS_UART_TX                              sciSetTxReg
// void init(uint32_t baudrate, void (*tx_callback)(void), void (*rx_callback)(void))
#define MODBUS_UART_INIT                           sciInit
// Disable tx_complete or tx_fifo_empty interrupts
#define MODBUS_UART_TX_DISABLE                     sciTxDisable
// Enable tx_complete or tx_fifo_empty interrupts
#define MODBUS_UART_TX_ENABLE                      sciTxEnable
// void init(uint32_t cpuFreqMhz, uint32_t period, bool oneshot)
#define MODBUS_TIMER_INIT                          mbTimerInit
// void start(void(*callback)(void))
#define MODBUS_TIMER_START                         mbTimerStart

#endif /* INCLUDE_PROTO_BINDINGS_H_ */
