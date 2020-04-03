/*
 * proto_types.h
 *
 *  Created on: 25 мая 2017 г.
 *      Author: p.borisenko
 */

#ifndef INCLUDE_PROTO_TYPES_H_
#define INCLUDE_PROTO_TYPES_H_

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    WRITE_NO_RESULT= 0,
    WRITE_SUCCESS,
    WRITE_WRONG_VALUE,
    WRITE_MEMORY_FAIL
} write_result_t;

typedef enum {
    IDLE= 0,
    CRC_CHECK,
    PARSE_FUNCTION,
    FORMAT_RESPONSE,
    WRITE_TO_REGISTER,
    CHECK_WRITE_RESULT,
    FORMAT_ACK,
    FORMAT_ERROR,
    SEND_DATA
} proto_state_t;

typedef struct proto_reg_data {
    uint16_t * reg;        // Pointer to actual data
    bool we;                    // Write enable
} proto_reg_data_t;

typedef struct proto_modbus_data{
    proto_reg_data_t * data;
    uint16_t length;
} proto_modbus_data_t;


#endif /* INCLUDE_PROTO_TYPES_H_ */
