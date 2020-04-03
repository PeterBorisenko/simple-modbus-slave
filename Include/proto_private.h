/*!
 * @file proto_private.h 
 *
 * @date 28 апр. 2017 г.
 * @author Peter Borisenko (peter@awsmtek.com)
 * @version 0.1a
 * @brief Internal methods and variables of Simple Modbus Protocol
 */

#ifndef SOURCE_PROTO_PRIVATE_H_
#define SOURCE_PROTO_PRIVATE_H_

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "enum.h"
#include "config.h"
#include "defines.h"

#include "checksum.h"
#include "proto.h"
#include "proto_bindings.h"

#pragma DATA_SECTION(ADU_buffer, "staticData");
static uint16_t ADU_buffer[MODBUS_MESSAGE_LEN_MAX];
static uint16_t * PDU_buffer;

/*!
 * @fn proto_t * getProto(void)
 * @brief Get the pointer to protocol instance (for using in ISR) (Not currently used)
 * @param none
 * @retval Pointer to proto_t
 */
proto_t * getProto(void);

/*!
 * @fn void setProto(proto_t * proto)
 * @brief Set the global pointer to protocol instance (for using in ISR)
 * @param[in] proto Pointer to protocol instance
 * @retval none 
 */
void setProto(proto_t * proto);

/*!
 * @fn void uartRxTimerCallback(void)
 * @brief Receive timer callback. Called when RX delimiter was received
 * @param none
 * @retval none
 */
void uartRxTimerCallback(void);

/*!
 * @fn void uartTxTimerCallback(void)
 * @brief Transmit timer callback. Called when all data and delimiter was transmitted
 * @param none
 * @retval none
 */
void uartTxTimerCallback(void);

/*!
 * @fn void uartRxByteCallback(void)
 * @brief One symbol receive callback.
 * 	Called after each byte received.
 * 	Restarting RX timer to check if delimiter will follow this symbol.
 * @param none
 * @retval none
 */
void uartRxByteCallback(void);

/*!
 * @fn void uartTxByteCallback(void)
 * @brief One symbol transmit callback.
 * 	Called after each byte transmitted.
 * 	Starting TX timer after last symbol processed and disable transmitting.
 * @param none
 * @retval none
 */
void uartTxByteCallback(void);

/*!
 * @fn uint16_t txData(proto_t * pProto)
 * @brief Transmitting data
 * @param[inout] pProto pointer to protocol entity which includes pointer to data and data length
 * @retval none
 */
uint16_t txData(proto_t * pProto);

bool registerAddressCheck(proto_t * pProto, uint16_t address);

bool requestCheck(proto_t * pProto, uint16_t dataaddr, uint16_t datacnt);

uint16_t registerRead(proto_t * pProto, uint16_t address);

bool registerCheckWritable(proto_t * pProto, uint16_t address);

void registerWrite(proto_t * pProto, uint16_t address, uint16_t data);

uint16_t createAdu(uint16_t * buf, uint16_t address, uint16_t pdu_len);

uint16_t createErrorPdu(uint16_t * buf, uint16_t function, uint16_t err);

uint16_t createReadPdu(proto_t * pProto, uint16_t * buf, uint16_t function, uint16_t dataaddr, uint16_t datalen);

uint16_t createAckPdu(uint16_t * buf, uint16_t function, uint16_t addr, uint16_t val);

#endif /* SOURCE_PROTO_PRIVATE_H_ */
