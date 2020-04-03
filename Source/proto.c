/*
 * proto.c
 *
 *  Created on: 28 апр. 2017 г.
 *      Author: p.borisenko
 */

#include "proto_private.h"

void protoInit(proto_t * proto, proto_init_t * protoInitStruct) {
    uint32_t delimiterMicros;
    setProto(proto);  // For ISR
    proto->state= IDLE;
    proto->tx_en_cb= protoInitStruct->tx_en;
    PDU_buffer= &ADU_buffer[MODBUS_ADU_PDU_START_idx];

    // Init Buffers
    proto->rx.idx= 0;
    proto->rx.length= 0;
    proto->rx.buffer= ADU_buffer;
    proto->tx.idx= 0;
    proto->tx.length= 0;
    proto->tx.buffer= ADU_buffer;

    // Init data
    proto->data.data= (proto_reg_data_t *)protoInitStruct->modbus_user_data;
    proto->data.length= protoInitStruct->registers_number;
    proto->ownAddress= protoInitStruct->address;
    proto->modbus_data_write_result= WRITE_NO_RESULT;

    // Init Flags
    proto->flags.packetReceived= 0;
    proto->flags.txlocked= 0;
    proto->flags.broadcast= 0;
    proto->flags.rts= 0;

    // Init Timers
    delimiterMicros= (uint32_t)(10000000*MODBUS_DELIMITER_TIMER_PERIOD_syms*((float)(1.0/UART_BAUDRATE)));
    MODBUS_TIMER_INIT(protoInitStruct->cpuFreqMhz, delimiterMicros, true);

    // Init Hardware UART
    MODBUS_UART_INIT(UART_BAUDRATE, uartTxByteCallback, uartRxByteCallback);

    // Init Checksum
    init_crc16_tab();

    // Set driver to receive
    if (proto->tx_en_cb != NULL) {
        proto->tx_en_cb(false);
    }
}

void protoBackgroundProcess(proto_t * proto) {
    static uint16_t address, function, error;
    static uint16_t dataaddr, datavalue, datacnt, value;
    switch(proto->state) {
    case IDLE: // Polling data
        if (proto->flags.packetReceived) {
            if (proto->rx.length > 3) {
                // Check packet address
                address= proto->rx.buffer[MODBUS_ADU_DEVICE_ADDRESS_idx];
                if (address == proto->ownAddress || address == MODBUS_BROADCAST_ADDRESS) {
                    if (address == MODBUS_BROADCAST_ADDRESS) {
                        proto->flags.broadcast= 1;
                    } else {
                        proto->flags.broadcast= 0;
                    }
                    // Address ok
                    proto->state= CRC_CHECK;
                }
            }
            proto->flags.packetReceived= 0;
        }
        break;
    case CRC_CHECK: // Check packet
        if (crc_modbus((const unsigned char *)proto->rx.buffer, proto->rx.length-2) == ((proto->rx.buffer[proto->rx.length-1])<<8|proto->rx.buffer[proto->rx.length-2])) {
            // CRC ok
            proto->state= PARSE_FUNCTION;
        } else {
            proto->state= IDLE;
        }
        break;
    case PARSE_FUNCTION: // Parse request
        function= PDU_buffer[MODBUS_PDU_FUNCTION_CODE_idx];    // get function code
        switch (function) {
        case MODBUS_FUNCTION_READ_HOLDING_REGISTERS:
            dataaddr= (PDU_buffer[MODBUS_PDU_READ_REGISTER_ADDRESS_HI_idx]<<8)|PDU_buffer[MODBUS_PDU_READ_REGISTER_ADDRESS_LO_idx];
            datacnt= (PDU_buffer[MODBUS_PDU_READ_REGISTER_NUM_REGS_HI_idx]<<8)|PDU_buffer[MODBUS_PDU_READ_REGISTER_NUM_REGS_LO_idx];
            if (requestCheck(proto, dataaddr, datacnt)) { // TODO: Change sending error to reading only available registers
                proto->state= FORMAT_RESPONSE;
            } else {
                error= MODBUS_EXCEPTION_ILLEGAL_DATA_ADDRESS;
                proto->state= FORMAT_ERROR;
            }
            break;
        case MODBUS_FUNCTION_WRITE_SINGLE_REGISTER:
            dataaddr= (PDU_buffer[MODBUS_PDU_READ_REGISTER_ADDRESS_HI_idx]<<8)|PDU_buffer[MODBUS_PDU_READ_REGISTER_ADDRESS_LO_idx];
            if (registerAddressCheck(proto, dataaddr) && registerCheckWritable(proto, dataaddr)) {
                datavalue= (PDU_buffer[MODBUS_PDU_WRITE_REGISTER_VALUE_HI_idx]<<8)|PDU_buffer[MODBUS_PDU_WRITE_REGISTER_VALUE_LO_idx];
                proto->state= WRITE_TO_REGISTER;
            } else {
                error= MODBUS_EXCEPTION_ILLEGAL_DATA_ADDRESS;
                proto->state= FORMAT_ERROR;
            }
            break;
        case MODBUS_FUNCTION_READ_COIL_STATUS: case MODBUS_FUNCTION_READ_DESCRETE_INPUTS:
        case MODBUS_FUNCTION_READ_INPUT_REGISTERS: case MODBUS_FUNCTION_FORCE_SINGLE_COIL:
        case MODBUS_FUNCTION_READ_EXCEPTION_STATUS: case MODBUS_FUNCTION_DIAGNOSTIC:
        case MODBUS_FUNCTION_GET_COM_EVENT_COUNTER: case MODBUS_FUNCTION_GET_COM_EVENT_LOG:
        case MODBUS_FUNCTION_FORCE_MULTIPLY_COILS: case MODBUS_FUNCTION_WRITE_MULTIPLY_REGISTERS:
        case MODBUS_FUNCTION_READ_FILE_RECORD: case MODBUS_FUNCTION_WRITE_FILE_RECORD:
        case MODBUS_FUNCTION_MASK_WRITE_REGISTER: case MODBUS_FUNCTION_READ_FIFO_QUEUE:
        case MODBUS_FUNCTION_ENCAPSULATED_INTERFACE_TRANSPORT:
        default:
            error= MODBUS_EXCEPTION_ILLEGAL_FUNCTION;
            proto->state= FORMAT_ERROR;
            break;
        }
        break;
    case FORMAT_RESPONSE: // Send requested data
        if (!proto->flags.broadcast) {
            proto->tx.length= createAdu(ADU_buffer, address, createReadPdu(proto, PDU_buffer, function, dataaddr, datacnt));
            proto->flags.rts= 1;
            proto->state= SEND_DATA;
        } else {
            proto->state= IDLE;
        }
        break;
    case WRITE_TO_REGISTER: // Write data to register
        registerWrite(proto, dataaddr, datavalue);
        proto->state= CHECK_WRITE_RESULT;
        break;
    case CHECK_WRITE_RESULT: // Check write result
        if (proto->modbus_data_write_result != WRITE_NO_RESULT) {
            switch (proto->modbus_data_write_result) {
                case WRITE_NO_RESULT: // No result
                    break;
                case WRITE_SUCCESS: // OK
                    value= datavalue;
                    proto->state= FORMAT_ACK;
                    break;
                case WRITE_WRONG_VALUE: // Data range error
                    error= MODBUS_EXCEPTION_ILLEGAL_DATA_VALUE;
                    proto->state= FORMAT_ERROR;
                    break;
                case WRITE_MEMORY_FAIL: // Write error
                    error= MODBUS_EXCEPTION_MEMORY_PARITY_ERROR;
                    proto->state= FORMAT_ERROR;
                    break;
                default:
                    error= MODBUS_EXCEPTION_SLAVE_DEVICE_FAILURE;
                    proto->state= FORMAT_ERROR;
                    break;
            }
            proto->modbus_data_write_result= WRITE_NO_RESULT;
        }
        break;
    case FORMAT_ACK: // Send Acknowledgement
        if (!proto->flags.broadcast) {
            proto->tx.length= createAdu(ADU_buffer, address, createAckPdu(PDU_buffer, function, dataaddr, value));   // Use same PDU that we just received
            proto->flags.rts= 1;
            proto->state= SEND_DATA;
        } else {
            proto->state= IDLE;
        }
        break;
    case FORMAT_ERROR: // Error
        if (!proto->flags.broadcast) {
            proto->tx.length= createAdu(ADU_buffer, address, createErrorPdu(PDU_buffer, function, error));
            proto->flags.rts= 1;
            proto->state= SEND_DATA;
        } else {
            proto->state= IDLE;
        }
        break;
    case SEND_DATA:  // Transmit data
        if (proto->flags.rts) {  // Request to Send
            if (txData(proto) == 1) {
                proto->flags.rts= 0;
            }
        }
        proto->state= IDLE;
        break;
    default:
        break;
    }
}



