/*
 * modbus_user_data.h
 *
 *  Created on: 15 мая 2017 г.
 *      Author: p.borisenko
 */

#ifndef INCLUDE_MODBUS_USER_DATA_H_
#define INCLUDE_MODBUS_USER_DATA_H_

#include <stdint.h>
#include <stdbool.h>

#include "proto_types.h"

#define ADDRESS_OFFSET  0x1001  // Bus address of first register

/*
 * @brief: Structure to setup application data
 *
 * How to use:
 * 1 - Define struct fields in accordance with your protocol implementation
 * (fields should be 16-bit aligned)
 * 2 - In your code define an entity of this struct
 * 3 - Assign an addresses of actual data to struct fields
 * 4 - Pass pointer to this struct to the appropriate field in protocol init structure (protoInit_t)
 * 5 - Cast it as (void *) to prevent warning
 */
typedef struct {
    proto_reg_data_t reg1001;
    proto_reg_data_t reg1002;
    proto_reg_data_t reg1003;
} modbus_user_data_t;

#endif /* INCLUDE_MODBUS_USER_DATA_H_ */
