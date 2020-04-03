/*
 * test_proto_private.c
 *
 *  Created on: 22 мая 2017 г.
 *      Author: p.borisenko
 */


#include "unity.h"

#include "mock_DSP28x_Project.h"
#include "mock_SCI.h"
#include "mock_modbus_timer.h"
#include "mock_checksum.h"
#include "mock_testCallbacks.h"

#include "proto_private.h"

// User definable data
#ifdef ADDRESS_OFFSET
#undef ADDRESS_OFFSET
#endif
#define ADDRESS_OFFSET 0x1001
#define REGS_NUMBER    3
#define OWN_ADDRESS 0x15
static uint16_t register1= 0x1234;
static uint16_t register2= 0x5678;
static uint16_t register3= 0x9ABC;
proto_t proto;
proto_reg_data_t test_data[REGS_NUMBER]= {  {&register1, true},
                                            {&register2, true},
                                            {&register3, false}
                                        };

uint16_t data_to_send[]= {0x02, 0xAF, 0x4E, 0x51, 0x0D};

void setUp(void)
{
    memset(&proto, 0x00, sizeof(proto_t));
    proto.tx_en_cb= callbackWithOneIntArgument;
    proto.data.data= test_data;
    proto.data.length= REGS_NUMBER;
    proto.ownAddress= OWN_ADDRESS;
    proto.tx.buffer= data_to_send;
    proto.tx.length= 5;
    proto.tx.idx= 0;
}

void tearDown(void)
{
}

void test_txData_shouldCallTxEnable(void) {
    callbackWithOneIntArgument_Ignore();
    sciSetTxReg_Expect(0x02);
    sciTxEnable_Expect();
    txData(&proto);
}

void test_txData_shouldReturnOneIfSuccess(void) {
    uint16_t result= 0;
    callbackWithOneIntArgument_Ignore();
    sciSetTxReg_Expect(0x02);
    sciTxEnable_Ignore();
    result= txData(&proto);
    TEST_ASSERT_EQUAL(1, result);
}
/*
void test_txData_shouldReturnZeroIfBufferLocked(void) {
    uint16_t result= 1;
    proto.flags.txlocked= 1;
    //sciTxEnable_Expect();
    result= txData(&proto);
    TEST_ASSERT_EQUAL(0, result);
}

void test_txData_shouldLockTxBuffer(void) {
    callbackWithOneIntArgument_Ignore();
    sciSetTxReg_Expect(0x02);
    sciTxEnable_Ignore();
    txData(&proto);
    TEST_ASSERT_EQUAL(1, proto.flags.txlocked);
}
*/

void test_registerAddressCheck_shouldReturnFalseIfAddressLessThanStartAddress(void) {
    uint16_t address= ADDRESS_OFFSET - 1;
    TEST_ASSERT_FALSE(registerAddressCheck(&proto, address));
}

void test_registerAddressCheck_shouldReturnFalseIfAddressMoreThanStartAddressPlusRegNum(void) {
    uint16_t address= ADDRESS_OFFSET+3;
    TEST_ASSERT_FALSE(registerAddressCheck(&proto, address));
}

void test_registerAddressCheck_shouldReturnTrueIfAddressInRange(void) {
    uint16_t address= ADDRESS_OFFSET+1;
    TEST_ASSERT_TRUE(registerAddressCheck(&proto, address));
}

void test_registerRead_shouldReturnValueOfAddressedRegister(void) {
    uint16_t idx= 1;
    TEST_ASSERT_EQUAL_HEX16(0x5678, registerRead(&proto, idx+ADDRESS_OFFSET));
}

void test_registerCheckWritable_shouldReturnFalseIfDataReadOnly(void) {
    uint16_t idx= 2;
    TEST_ASSERT_FALSE(registerCheckWritable(&proto, idx + ADDRESS_OFFSET));
}

void test_registerCheckWritable_shouldReturnTrueIfDataWritable(void) {
    uint16_t idx= 1;
    TEST_ASSERT_TRUE(registerCheckWritable(&proto, idx + ADDRESS_OFFSET));
}

void test_registerWrite_shouldSetRegisterValue(void) {
    uint16_t idx= 1;
    uint16_t data= 0x5151;
    registerWrite(&proto, idx + ADDRESS_OFFSET, data);
    TEST_ASSERT_EQUAL_HEX16(data, register2);
}

void test_createAdu_shouldSetAddressToAduBuffer(void) {
    uint16_t ADU_buffer[MODBUS_MESSAGE_LEN_MAX];
    uint16_t * PDU_buffer= &ADU_buffer[MODBUS_ADU_PDU_START_idx];
    PDU_buffer[0]= 0x01;
    PDU_buffer[1]= 0x01;
    PDU_buffer[2]= 0x01;
    PDU_buffer[3]= 0x01;
    crc_modbus_ExpectAndReturn((char *)ADU_buffer, 5, 0x89AC);
    uint16_t len= createAdu(ADU_buffer, 1, 4);
    TEST_ASSERT_EQUAL_HEX16(1, ADU_buffer[0]);
}

void test_createAdu_shouldCopyPduToAduBuffer(void) {
    uint16_t ADU_buffer[MODBUS_MESSAGE_LEN_MAX];
    uint16_t * PDU_buffer= &ADU_buffer[MODBUS_ADU_PDU_START_idx];
    PDU_buffer[0]= 0x01;
    PDU_buffer[1]= 0x01;
    PDU_buffer[2]= 0x01;
    PDU_buffer[3]= 0x01;
    crc_modbus_ExpectAndReturn((char *)ADU_buffer, 5, 0x89AC);
    uint16_t len= createAdu(ADU_buffer, 1, 4);
    TEST_ASSERT_EQUAL_HEX16_ARRAY(PDU_buffer, &ADU_buffer[MODBUS_ADU_PDU_START_idx], 4);
}

void test_createAdu_shouldReturnLengthOfBuffer(void) {
    uint16_t ADU_buffer[MODBUS_MESSAGE_LEN_MAX];
    uint16_t * PDU_buffer= &ADU_buffer[MODBUS_ADU_PDU_START_idx];
    PDU_buffer[0]= 0x01;
    PDU_buffer[1]= 0x01;
    PDU_buffer[2]= 0x01;
    PDU_buffer[3]= 0x01;
    crc_modbus_ExpectAndReturn((char *)ADU_buffer, 5, 0x89AC);
    uint16_t len= createAdu(ADU_buffer, 1, 4);
    // ADU length= PDU length + Address length + CRC length = 4 + 1 + 2
    TEST_ASSERT_EQUAL(7, len);
}

void test_createAdu_shouldSetCrcToBuffer(void) {
    uint16_t ADU_buffer[MODBUS_MESSAGE_LEN_MAX];
    uint16_t * PDU_buffer= &ADU_buffer[MODBUS_ADU_PDU_START_idx];
    PDU_buffer[0]= 0x01;
    PDU_buffer[1]= 0x01;
    PDU_buffer[2]= 0x01;
    PDU_buffer[3]= 0x01;
    crc_modbus_ExpectAndReturn((char *)ADU_buffer, 5, 0x89AC);
    uint16_t crc_buf[]= {0xAC, 0x89};
    uint16_t len= createAdu(ADU_buffer, 1, 4);
    TEST_ASSERT_EQUAL_HEX16_ARRAY(crc_buf, &ADU_buffer[1+4], 2);
}

void test_createErrorPdu_shouldReturnLength(void) {
    uint16_t ADU_buffer[MODBUS_MESSAGE_LEN_MAX];
    uint16_t * PDU_buffer= &ADU_buffer[MODBUS_ADU_PDU_START_idx];
    uint16_t function, err;
    uint16_t length= createErrorPdu(PDU_buffer, function, err);
    // Length is always 2 (MODBUS_PDU_EXCEPTION_CODE_idx)
    TEST_ASSERT_EQUAL(2, length);
}

void test_createErrorPdu_shouldPrepareErrorFunction(void) {
    uint16_t ADU_buffer[MODBUS_MESSAGE_LEN_MAX];
    uint16_t * PDU_buffer= &ADU_buffer[MODBUS_ADU_PDU_START_idx];
    uint16_t function= 0x01, err= 0x02;
    createErrorPdu(PDU_buffer, function, err);
    TEST_ASSERT_EQUAL_HEX16(function, PDU_buffer[MODBUS_PDU_FUNCTION_CODE_idx]&(~0x80));
    TEST_ASSERT_EQUAL_HEX16(err, PDU_buffer[MODBUS_PDU_EXCEPTION_CODE_idx]);
}

void test_requestCheck(void) {
    
}

void test_createReadPdu(void) {

}

void test_createAckPdu(void) {

}
