/*
 * main.c
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "proto.h"
#include "modbus_user_data.h"

static proto_t proto;
volatile static modbus_user_data_t myData;

void tx_enable(uint16_t state) {
    if (state) {
        // GPIO Set
    } else {
        // GPIO Clear
    }
}

void main(void)
{

    // These registers can be wherever you want
    uint16_t reservedReg= 0x0000;
    uint32_t value= 0x12345678;

    myData.reg1001.reg= GET_PTR_TO_HALFWORD_HIGH(&value); // V_bus_H
    myData.reg1001.we= false;
    myData.reg1002.reg= GET_PTR_TO_HALFWORD_LOW(&value); // V_bus_L
    myData.reg1002.we= false;
    myData.reg1003.reg= &reservedReg; // V_in_H
    myData.reg1003.we= true;

    proto_init_t protoInitStruct= {
                                  .address= 0x10,   // NOTE: Use function or macro readAddress() instead
                                  .cpuFreqMhz= 50,
                                  .registers_number= 0x1C,     // NOTE: Dont use sizeof cuz we need a registers number
                                  .modbus_user_data= (void*) &myData,
                                  .tx_en= tx_enable
    };

    protoInit(&proto, &protoInitStruct);

	for(;;)
	{
	  protoBackgroundProcess();
	}
}   //end main
