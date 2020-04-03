/*
 * test_proto.c
 *
 *  Created on: 4 мая 2017 г.
 *      Author: p.borisenko
 */


#include "unity.h"

#include "mock_DSP28x_Project.h"
#include "mock_SCI.h"
#include "mock_modbus_timer.h"
#include "mock_checksum.h"
#include "mock_proto_private.h"
#include "mock_testCallbacks.h"

#include "modbus.h"
#include "proto.h"

// User definable data
#ifdef ADDRESS_OFFSET
#undef ADDRESS_OFFSET
#endif
#define ADDRESS_OFFSET 0x1001
#define REGS_NUMBER    3
#define OWN_ADDRESS 0x0015
proto_t proto;
struct  modbus_user_data {
	proto_reg_data_t reg1;
	proto_reg_data_t reg2;
	proto_reg_data_t reg3;
} test_data;

// Test data
proto_init_t initStruct;

void setUp(void)
{
    initStruct.cpuFreqMhz= 50;
    initStruct.modbus_user_data= (void*)&test_data;
    initStruct.registers_number= REGS_NUMBER;
    initStruct.address= OWN_ADDRESS;
    initStruct.tx_en= callbackWithOneIntArgument;
    setProto_Expect(&proto);
    mbTimerInit_Ignore();
    sciInit_Ignore();
    init_crc16_tab_Ignore();
    callbackWithOneIntArgument_Ignore();
    protoInit(&proto, &initStruct);
}

void tearDown(void)
{
}

void test_protoInit_shouldCallUartTimerCrcInits(void) {
    setProto_Expect(&proto);
    mbTimerInit_Expect(50, (uint32_t)3645, true);
    //sciInit_Expect(UART_BAUDRATE, uartTxByteCallback, uartRxByteCallback);
    sciInit_Ignore();
    init_crc16_tab_Expect();
    callbackWithOneIntArgument_Expect(0);
    protoInit(&proto, &initStruct);
}

void test_protoInit_shouldSetDeviceAddress(void) {
    TEST_ASSERT_EQUAL(OWN_ADDRESS, proto.ownAddress);
}

void test_protoInit_shouldResetInternalBuffers(void) {
    TEST_ASSERT_EQUAL(0, proto.tx.length);
    TEST_ASSERT_EQUAL(0, proto.tx.idx);
    TEST_ASSERT_EQUAL(0, proto.rx.length);
    TEST_ASSERT_EQUAL(0, proto.rx.idx);
}

void test_protoInit_shouldResetInternalFlagsAndState(void) {
    TEST_ASSERT_EQUAL(IDLE, proto.state);
    TEST_ASSERT_EQUAL(0, proto.flags.packetReceived);
    TEST_ASSERT_EQUAL(0, proto.flags.txlocked);
    TEST_ASSERT_EQUAL(0, proto.flags.broadcast);
    TEST_ASSERT_EQUAL(0, proto.flags.rts);
}

void test_protoInit_shouldAssignConfigValuesToInternalDataPointers(void) {
    TEST_ASSERT_EQUAL_PTR_ARRAY(&test_data, proto.data.data, 3);
    TEST_ASSERT_EQUAL(REGS_NUMBER, proto.data.length);
}

void test_protoBackgroundProcess_shouldLeaveStateInitializedIfDataWasNotReceived(void) {
    proto.flags.packetReceived= 0;
    protoBackgroundProcess(&proto);
    TEST_ASSERT_EQUAL(IDLE, proto.state);
}

void test_protoBackgroundProcess_shouldLeaveStateUnchangedIfDataContainsWrongAddress(void) {
    proto.rx.buffer[0]= OWN_ADDRESS+1;
    proto.flags.packetReceived= 1;
    protoBackgroundProcess(&proto);
    TEST_ASSERT_EQUAL(IDLE, proto.state);
    TEST_ASSERT_EQUAL(0, proto.flags.packetReceived);
}

void test_protoBackgroundProcess_shouldLeaveStateUnchangedIfRxLengthLessThan4(void) {
    proto.rx.buffer[0]= OWN_ADDRESS;
    proto.rx.buffer[1]= 0x0001;
    proto.rx.length= 2;
    proto.flags.packetReceived= 1;
    protoBackgroundProcess(&proto);
    TEST_ASSERT_EQUAL(0, proto.state);
}

void test_protoBackgroundProcess_shouldChangeStateToCrcCheckIfDataContainsOwnAddress(void) {
    proto.rx.buffer[0]= OWN_ADDRESS;
    proto.rx.buffer[1]= 0x0001;
    proto.rx.buffer[2]= 0x00CE;
    proto.rx.buffer[3]= 0x00E0;
    proto.rx.length= 4;
    proto.flags.packetReceived= 1;
    protoBackgroundProcess(&proto);
    TEST_ASSERT_EQUAL(CRC_CHECK, proto.state);
    TEST_ASSERT_EQUAL(0, proto.flags.packetReceived);
    TEST_ASSERT_EQUAL(0, proto.flags.broadcast);
}

void test_protoBackgroundProcess_shouldChangeStateToCrcCheckAndSetBroadcastFlagIfDataContainsBroadAddress(void) {
    proto.rx.buffer[0]= MODBUS_BROADCAST_ADDRESS;
    proto.rx.buffer[1]= 0x0001;
    proto.rx.buffer[2]= 0x00CE;
    proto.rx.buffer[3]= 0x00E0;
    proto.rx.length= 4;
    proto.flags.packetReceived= 1;
    protoBackgroundProcess(&proto);
    TEST_ASSERT_EQUAL(CRC_CHECK, proto.state);
    TEST_ASSERT_EQUAL(0, proto.flags.packetReceived);
    TEST_ASSERT_EQUAL(1, proto.flags.broadcast);
}

void test_protoBackgroundProcess_shouldCallCrcCalcAndPassBuffer(void) {
    proto.rx.buffer[0]= OWN_ADDRESS;
    proto.rx.buffer[1]= 0x0001;
    proto.rx.buffer[2]= 0x00CE;
    proto.rx.buffer[3]= 0x00E0;
    proto.rx.length= 4;
    proto.flags.packetReceived= 1;
    protoBackgroundProcess(&proto);
    crc_modbus_ExpectAndReturn((const unsigned char *)proto.rx.buffer, 2, 0xE0CE);
    protoBackgroundProcess(&proto);
}

void test_protoBackgroundProcess_shouldChangeStateToPARSE_FUNCTIONIfCrcIsCorrect(void) {
    proto.rx.buffer[0]= OWN_ADDRESS;
    proto.rx.buffer[1]= 0x0001;
    proto.rx.buffer[2]= 0x00CE;
    proto.rx.buffer[3]= 0x00E0;
    proto.rx.length= 4;
    proto.flags.packetReceived= 1;
    protoBackgroundProcess(&proto);
    crc_modbus_IgnoreAndReturn(0xE0CE);
    protoBackgroundProcess(&proto);
    TEST_ASSERT_EQUAL(PARSE_FUNCTION, proto.state);
}

void test_protoBackgroundProcess_shouldChangeStateTo0IfCrcIsWrong(void) {
    proto.rx.buffer[0]= OWN_ADDRESS;
    proto.rx.buffer[1]= 0x0001;
    proto.rx.buffer[2]= 0x00CE;
    proto.rx.buffer[3]= 0x00E1;
    proto.rx.length= 4;
    proto.flags.packetReceived= 1;
    protoBackgroundProcess(&proto);
    crc_modbus_IgnoreAndReturn(0xE0CE);
    protoBackgroundProcess(&proto);
    TEST_ASSERT_EQUAL(IDLE, proto.state);
}

void test_protoBackgroundProcess_shouldCallRegisterAddressCheckIfFunctionIsReadRegisterInStateParseFunction(void) {
    proto.rx.buffer[0]= OWN_ADDRESS;
    proto.rx.buffer[1]= MODBUS_FUNCTION_READ_HOLDING_REGISTERS;
    proto.rx.buffer[2]= 0x0003;
    proto.rx.buffer[3]= 0x00EA;
    proto.rx.buffer[4]= 0x0000;
    proto.rx.buffer[5]= 0x0002;
    proto.rx.buffer[6]= 0x0075;
    proto.rx.buffer[7]= 0x0057;
    proto.rx.length= 8;
    proto.flags.packetReceived= 1;
    protoBackgroundProcess(&proto);
    crc_modbus_IgnoreAndReturn(0x5775);
    protoBackgroundProcess(&proto);
    TEST_ASSERT_EQUAL(PARSE_FUNCTION, proto.state);
    requestCheck_ExpectAndReturn(&proto, 0x03EA, 2, 1);
    protoBackgroundProcess(&proto);
}

void test_protoBackgroundProcess_shouldGoStateFormatErrorIfAddressCheckingFailsInStateParseFunction(void) {
    proto.rx.buffer[0]= OWN_ADDRESS;
    proto.rx.buffer[1]= MODBUS_FUNCTION_READ_HOLDING_REGISTERS;
    proto.rx.buffer[2]= 0x0003;
    proto.rx.buffer[3]= 0x00EA;
    proto.rx.buffer[4]= 0x0000;
    proto.rx.buffer[5]= 0x0022;
    proto.rx.buffer[6]= 0x0075;
    proto.rx.buffer[7]= 0x0057;
    proto.rx.length= 8;
    proto.flags.packetReceived= 1;
    protoBackgroundProcess(&proto);
    crc_modbus_IgnoreAndReturn(0x5775);
    protoBackgroundProcess(&proto);
    requestCheck_ExpectAndReturn(&proto, 0x03EA, 0x22, 0);
    protoBackgroundProcess(&proto);
    TEST_ASSERT_EQUAL(FORMAT_ERROR, proto.state);
}

void test_protoBackgroundProcess_shouldGoStateFormatErrorIfFunctionIsUnknownInStateParseFunction(void) {
    proto.rx.buffer[0]= OWN_ADDRESS;
    proto.rx.buffer[1]= 0x00AD;
    proto.rx.buffer[2]= 0x0003;
    proto.rx.buffer[3]= 0x00EA;
    proto.rx.buffer[4]= 0x0075;
    proto.rx.buffer[5]= 0x0057;
    proto.rx.length= 6;
    proto.flags.packetReceived= 1;
    protoBackgroundProcess(&proto);
    crc_modbus_IgnoreAndReturn(0x5775);
    protoBackgroundProcess(&proto);
    protoBackgroundProcess(&proto);
    TEST_ASSERT_EQUAL(FORMAT_ERROR, proto.state);
}

void test_protoBackgroundProcess_shouldGoStateFormatErrorIfAddressNotWritableAndFunctionIsWriteRegisterInStateParseFunction(void) {
    proto.rx.buffer[0]= OWN_ADDRESS;
    proto.rx.buffer[1]= MODBUS_FUNCTION_WRITE_SINGLE_REGISTER;
    proto.rx.buffer[2]= 0x0003;
    proto.rx.buffer[3]= 0x00EA;
    proto.rx.buffer[4]= 0x0075;
    proto.rx.buffer[5]= 0x0057;
    proto.rx.length= 6;
    proto.flags.packetReceived= 1;
    protoBackgroundProcess(&proto);
    crc_modbus_IgnoreAndReturn(0x5775);
    protoBackgroundProcess(&proto);
    registerAddressCheck_IgnoreAndReturn(1);
    registerCheckWritable_ExpectAndReturn(&proto, 0x03EA, 0);
    protoBackgroundProcess(&proto);
    TEST_ASSERT_EQUAL(FORMAT_ERROR, proto.state);
}

void test_protoBackgroundProcess_shouldGoStateFormatResponseIfAddressCheckingSuccessInStateParseFunction(void) {
    proto.rx.buffer[0]= OWN_ADDRESS;
    proto.rx.buffer[1]= MODBUS_FUNCTION_READ_HOLDING_REGISTERS;
    proto.rx.buffer[2]= 0x0003;
    proto.rx.buffer[3]= 0x00EA;
    proto.rx.buffer[4]= 0x0075;
    proto.rx.buffer[5]= 0x0057;
    proto.rx.length= 6;
    proto.flags.packetReceived= 1;
    protoBackgroundProcess(&proto);
    crc_modbus_IgnoreAndReturn(0x5775);
    protoBackgroundProcess(&proto);
    requestCheck_IgnoreAndReturn(1);
    protoBackgroundProcess(&proto);
    TEST_ASSERT_EQUAL(FORMAT_RESPONSE, proto.state);
}

void test_protoBackgroundProcess_shouldGoStateIdleIfBroadcastFlagSetInStateFormatResponse(void) {
    proto.rx.buffer[0]= MODBUS_BROADCAST_ADDRESS;
    proto.rx.buffer[1]= MODBUS_FUNCTION_READ_HOLDING_REGISTERS;
    proto.rx.buffer[2]= 0x0003;
    proto.rx.buffer[3]= 0x00EA;
    proto.rx.buffer[4]= 0x0075;
    proto.rx.buffer[5]= 0x0057;
    proto.rx.length= 6;
    proto.flags.packetReceived= 1;
    protoBackgroundProcess(&proto);   // to state 1
    crc_modbus_IgnoreAndReturn(0x5775);
    protoBackgroundProcess(&proto);   // to state 2
    requestCheck_IgnoreAndReturn(1);
    protoBackgroundProcess(&proto);   // to state 3
    protoBackgroundProcess(&proto);
    TEST_ASSERT_EQUAL(IDLE, proto.state);
}

void test_protoBackgroundProcess_shouldCreateAduPduAndGoStateSendDataIfBroadcastFlagNotSetInStateParseFunction(void) {
    proto.rx.buffer[0]= OWN_ADDRESS;
    proto.rx.buffer[1]= MODBUS_FUNCTION_READ_HOLDING_REGISTERS;
    proto.rx.buffer[2]= 0x0003;
    proto.rx.buffer[3]= 0x00EA;
    proto.rx.buffer[4]= 0x0000;
    proto.rx.buffer[5]= 0x0003;
    proto.rx.buffer[6]= 0x0075;
    proto.rx.buffer[7]= 0x0057;
    proto.rx.length= 8;
    proto.flags.packetReceived= 1;
    protoBackgroundProcess(&proto);   // to state 1
    crc_modbus_IgnoreAndReturn(0x5775);
    protoBackgroundProcess(&proto);   // to state 2
    requestCheck_IgnoreAndReturn(1);
    protoBackgroundProcess(&proto);   // to state 4
    //createReadPdu_ExpectAndReturn(&proto, PDU_buffer, MODBUS_FUNCTION_READ_HOLDING_REGISTERS, 0x03EA, 0x0003, 5);
    createReadPdu_IgnoreAndReturn(5);
    //createAdu_ExpectAndReturn(ADU_buffer, OWN_ADDRESS, PDU_buffer, 5, 8);
    createAdu_IgnoreAndReturn(8);
    protoBackgroundProcess(&proto);   // to state 100
    TEST_ASSERT_EQUAL(SEND_DATA, proto.state);
}


void test_protoBackgroundProcess_shouldGoStateWRITE_TO_REGISTERIfAddressWritableAndFunctionIsWriteRegisterInStateParseFunction(void) {
    proto.rx.buffer[0]= OWN_ADDRESS;
    proto.rx.buffer[1]= MODBUS_FUNCTION_WRITE_SINGLE_REGISTER;
    proto.rx.buffer[2]= 0x0003;
    proto.rx.buffer[3]= 0x00EA;
    proto.rx.buffer[4]= 0x0075;
    proto.rx.buffer[5]= 0x0057;
    proto.rx.length= 6;
    proto.flags.packetReceived= 1;
    protoBackgroundProcess(&proto);
    crc_modbus_IgnoreAndReturn(0x5775);
    protoBackgroundProcess(&proto);
    registerAddressCheck_IgnoreAndReturn(1);
    registerCheckWritable_ExpectAndReturn(&proto, 0x03EA, 1);
    protoBackgroundProcess(&proto);
    TEST_ASSERT_EQUAL(WRITE_TO_REGISTER, proto.state);
}

void test_protoBackgroundProcess_shouldCallRegisterWriteAndGoStateCheckWriteResultInStateWriteToRegister(void) {
    proto.rx.buffer[0]= OWN_ADDRESS;
    proto.rx.buffer[1]= MODBUS_FUNCTION_WRITE_SINGLE_REGISTER;
    proto.rx.buffer[2]= 0x0003;
    proto.rx.buffer[3]= 0x00EA;
    proto.rx.buffer[4]= 0x000A;
    proto.rx.buffer[5]= 0x00AA;
    proto.rx.buffer[6]= 0x0075;
    proto.rx.buffer[7]= 0x0057;
    proto.rx.length= 8;
    proto.flags.packetReceived= 1;
    protoBackgroundProcess(&proto);   // to CRC_CHECK
    crc_modbus_IgnoreAndReturn(0x5775);
    protoBackgroundProcess(&proto);   // to PARSE_FUNCTION
    registerAddressCheck_IgnoreAndReturn(1);
    registerCheckWritable_ExpectAndReturn(&proto, 0x03EA, 1);
    protoBackgroundProcess(&proto);   // to WRITE_TO_REGISTER
    registerWrite_Expect(&proto, 0x03EA, 0x0AAA);
    protoBackgroundProcess(&proto);   // to CHECK_WRITE_RESULT
    protoBackgroundProcess(&proto);   // to FORMAT_ACK
    TEST_ASSERT_EQUAL(CHECK_WRITE_RESULT, proto.state);
}

void test_protoBackgroundProcess_shouldGoStateFORMAT_ACKIfDataWriteSuccessfulInStateCHECK_WRITE_RESULT(void) {
    proto.rx.buffer[0]= OWN_ADDRESS;
    proto.rx.buffer[1]= MODBUS_FUNCTION_WRITE_SINGLE_REGISTER;
    proto.rx.buffer[2]= 0x0003;
    proto.rx.buffer[3]= 0x00EA;
    proto.rx.buffer[4]= 0x000A;
    proto.rx.buffer[5]= 0x00AA;
    proto.rx.buffer[6]= 0x0075;
    proto.rx.buffer[7]= 0x0057;
    proto.rx.length= 8;
    proto.flags.packetReceived= 1;
    protoBackgroundProcess(&proto);   // to 1
    crc_modbus_IgnoreAndReturn(0x5775);
    protoBackgroundProcess(&proto);   // to 2
    registerAddressCheck_IgnoreAndReturn(1);
    registerCheckWritable_ExpectAndReturn(&proto, 0x03EA, 1);
    protoBackgroundProcess(&proto);   // to 3
    registerWrite_Expect(&proto, 0x03EA, 0x0AAA);
    protoBackgroundProcess(&proto);   // to 4
    protoBackgroundProcess(&proto);   // to 5
    proto.modbus_data_write_result= WRITE_SUCCESS;
    protoBackgroundProcess(&proto);   // to FORMAT_ACK
    TEST_ASSERT_EQUAL(FORMAT_ACK, proto.state);
}

void test_protoBackgroundProcess_shouldGoStateSendDataIfInStateFormatAck(void) {
    proto.rx.buffer[0]= OWN_ADDRESS;
    proto.rx.buffer[1]= MODBUS_FUNCTION_WRITE_SINGLE_REGISTER;
    proto.rx.buffer[2]= 0x0003;
    proto.rx.buffer[3]= 0x00EA;
    proto.rx.buffer[4]= 0x000A;
    proto.rx.buffer[5]= 0x00AA;
    proto.rx.buffer[6]= 0x0075;
    proto.rx.buffer[7]= 0x0057;
    proto.rx.length= 8;
    proto.flags.packetReceived= 1;
    protoBackgroundProcess(&proto);   // to CRC_CHECK
    crc_modbus_IgnoreAndReturn(0x5775);
    protoBackgroundProcess(&proto);   // to PARSE_FUNCTION
    registerAddressCheck_IgnoreAndReturn(1);
    registerCheckWritable_ExpectAndReturn(&proto, 0x03EA, 1);
    protoBackgroundProcess(&proto);   // to WRITE_TO_REGISTER
    registerWrite_Expect(&proto, 0x03EA, 0x0AAA);
    protoBackgroundProcess(&proto);   // to CHECK_WRITE_RESULT
    proto.modbus_data_write_result= WRITE_SUCCESS;
    protoBackgroundProcess(&proto);   // to FORMAT_ACK
    //createAckPdu_ExpectAndReturn(PDU_buffer, MODBUS_FUNCTION_WRITE_SINGLE_REGISTER, 0x03EA, 0x0AAA, 3);
    createAckPdu_IgnoreAndReturn(3);
    //createAdu_ExpectAndReturn(ADU_buffer, OWN_ADDRESS, PDU_buffer, 3, 6);
    createAdu_IgnoreAndReturn(6);
    protoBackgroundProcess(&proto);   // to SEND_DATA
    TEST_ASSERT_EQUAL(SEND_DATA, proto.state);
}

void test_protoBackgroundProcess_shouldGoStateIdleIfInStateSendData(void) {
    proto.rx.buffer[0]= OWN_ADDRESS;
    proto.rx.buffer[1]= MODBUS_FUNCTION_WRITE_SINGLE_REGISTER;
    proto.rx.buffer[2]= 0x0003;
    proto.rx.buffer[3]= 0x00EA;
    proto.rx.buffer[4]= 0x000A;
    proto.rx.buffer[5]= 0x00AA;
    proto.rx.buffer[6]= 0x0075;
    proto.rx.buffer[7]= 0x0057;
    proto.rx.length= 8;
    proto.flags.packetReceived= 1;
    protoBackgroundProcess(&proto);   // to CRC_CHECK
    crc_modbus_IgnoreAndReturn(0x5775);
    protoBackgroundProcess(&proto);   // to PARSE_FUNCTION
    registerAddressCheck_ExpectAndReturn(&proto, 0x03EA, 1);
    registerCheckWritable_ExpectAndReturn(&proto, 0x03EA, 1);
    protoBackgroundProcess(&proto);   // to WRITE_TO_REGISTER
    registerWrite_Expect(&proto, 0x03EA, 0x0AAA);
    protoBackgroundProcess(&proto);   // to CHECK_WRITE_RESULT
    proto.modbus_data_write_result= WRITE_SUCCESS;
    protoBackgroundProcess(&proto);   // to FORMAT_ACK
    createAckPdu_IgnoreAndReturn(3);
    createAdu_IgnoreAndReturn(6);
    protoBackgroundProcess(&proto);   // to SEND_DATA
    TEST_ASSERT_EQUAL(1, proto.flags.rts);
    TEST_ASSERT_EQUAL(6, proto.tx.length);
    txData_ExpectAndReturn(&proto, 1);
    protoBackgroundProcess(&proto);   // to IDLE
    TEST_ASSERT_EQUAL(0, proto.flags.rts);
    TEST_ASSERT_EQUAL(IDLE, proto.state);
}
