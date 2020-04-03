/*
 * proto_private.c
 *
 *  Created on: 22 мая 2017 г.
 *      Author: p.borisenko
 */

#include "proto_private.h"
#include "modbus_user_data.h"

static proto_t * pProto;

void setProto(proto_t * p) {
    pProto= p;
}

proto_t * getProto(void) {
    return pProto;
}

void uartRxTimerCallback(void) {
    pProto->rx.length= pProto->rx.idx; // Store received packet length
    pProto->rx.idx= 0;
    pProto->flags.packetReceived= 1;
}

void uartTxTimerCallback(void) {
    // Set driver to receive
    if (pProto->tx_en_cb != NULL) {
        pProto->tx_en_cb(false);
    }
    pProto->flags.txlocked= 0;
}

void uartRxByteCallback(void) {
    uint16_t dat;
    // Get data byte
    dat= MODBUS_UART_RX();
    pProto->rx.buffer[pProto->rx.idx]= dat;
    pProto->rx.idx++;
    // Start/Restart delimiter timer
    MODBUS_TIMER_START(uartRxTimerCallback);
}


void uartTxByteCallback(void) {
    if (pProto->tx.idx < pProto->tx.length) {
        MODBUS_UART_TX(pProto->tx.buffer[pProto->tx.idx]);
        pProto->tx.idx++;
    } else {
        pProto->tx.length= 0;
        // Disable transmission
        MODBUS_UART_TX_DISABLE();
        // Start delimiter timer
        MODBUS_TIMER_START(uartTxTimerCallback);
    }
}

uint16_t txData(proto_t * pProto) {
    uint16_t i= 0;
    // Is any data to send?
    if (pProto->data.length > 0) {
        // Set driver to transmit
        if (pProto->tx_en_cb != NULL) {
            pProto->tx_en_cb(true);
        }
        pProto->tx.idx= 1;
        MODBUS_UART_TX(pProto->tx.buffer[0]);   // Send first byte
        MODBUS_UART_TX_ENABLE();                // then enable tx interrupts
        i= 1;
    }
    return i;
}

bool registerAddressCheck(proto_t * pProto, uint16_t address) {
    return requestCheck(pProto, address, 1);
}

bool requestCheck(proto_t * pProto, uint16_t address, uint16_t datacnt) {
    bool result= true;
    uint16_t data_idx= 0;
    if (address < ADDRESS_OFFSET) {
        result= false;
    } else {
        data_idx= address-ADDRESS_OFFSET;
        if (data_idx >= pProto->data.length) {
            result= false;
        }
        if (datacnt > 1) {
            if (data_idx+datacnt > pProto->data.length) {
                result= false;
            }
        }
    }
    return result;
}

uint16_t registerRead(proto_t * pProto, uint16_t address) {
    //! Unsafe function. Be sure calling addressCheck before this
    uint16_t idx= address - ADDRESS_OFFSET;
    return *(pProto->data.data[idx].reg);
}

bool registerCheckWritable(proto_t * pProto, uint16_t address) {
    //! Unsafe function. Be sure calling addressCheck before this
    uint16_t idx= address - ADDRESS_OFFSET;
    return pProto->data.data[idx].we;
}

void registerWrite(proto_t * pProto, uint16_t address, uint16_t data) {
    //! Unsafe function. Be sure calling addressCheck and checkWritable before this
    uint16_t idx= address - ADDRESS_OFFSET;
    *((uint16_t *)pProto->data.data[idx].reg)= data;
    pProto->modbus_data_write_result= WRITE_SUCCESS;    // Use in case of external memory device
}

uint16_t createAdu(uint16_t * buf, uint16_t address, uint16_t pdu_len) {
    uint16_t crc, len;
    len= MODBUS_ADU_PDU_START_idx + pdu_len;
    buf[MODBUS_ADU_DEVICE_ADDRESS_idx]= address;
    crc= crc_modbus((const unsigned char *)buf, len);
    buf[len++]= crc&0x00FF;
    buf[len++]= (crc>>8)&0x00FF;
    return len;
}

uint16_t createErrorPdu(uint16_t * buf, uint16_t function, uint16_t err) {
    buf[MODBUS_PDU_FUNCTION_CODE_idx]= MODBUS_FUNCTION_ERROR(function);
    buf[MODBUS_PDU_EXCEPTION_CODE_idx]= err;
    return MODBUS_PDU_EXCEPTION_CODE_idx+1;
}

uint16_t createReadPdu(proto_t * pProto, uint16_t * buf, uint16_t function, uint16_t dataaddr, uint16_t datalen) {
    uint16_t i, reg;
    buf[MODBUS_PDU_FUNCTION_CODE_idx]= function;
    buf[MODBUS_PDU_READ_RESP_REGISTER_BYTE_COUNT_idx]= datalen<<1;   // Byte number is twice as register number
    for (i= 0; i < datalen; i++) {
        reg= registerRead(pProto, dataaddr+i);
        buf[MODBUS_PDU_READ_RESP_REGISTER_FIRST_VALUE_idx+(i<<1)]= (reg>>8)&0xFF;
        buf[MODBUS_PDU_READ_RESP_REGISTER_FIRST_VALUE_idx+((i<<1)+1)]= reg&0xFF;
    }
    return (i<<1)+MODBUS_PDU_READ_RESP_REGISTER_FIRST_VALUE_idx;
}

uint16_t createAckPdu(uint16_t * buf, uint16_t function, uint16_t addr, uint16_t val) {
    buf[MODBUS_PDU_FUNCTION_CODE_idx]= function;
    buf[MODBUS_PDU_WRITE_RESP_REGISTER_BYTE_ADDR_H_idx]= (addr>>8)&0xFF;
    buf[MODBUS_PDU_WRITE_RESP_REGISTER_BYTE_ADDR_L_idx]= addr&0xFF;
    buf[MODBUS_PDU_WRITE_RESP_REGISTER_BYTE_COUNT_H_idx]= (val>>8)&0xFF;
    buf[MODBUS_PDU_WRITE_RESP_REGISTER_BYTE_COUNT_L_idx]= val&0xFF;
    return 5;
}
