/*
 * testCallbacks.h
 *
 *  Created on: 22 мая 2017 г.
 *      Author: p.borisenko
 */

#ifndef TEST_CEEDLING_TEST_SUPPORT_TESTCALLBACKS_H_
#define TEST_CEEDLING_TEST_SUPPORT_TESTCALLBACKS_H_

void callbackWithoutArguments(void);

void callbackWithOneVoidArgument(void * arg);

void callbackWithOneIntArgument(uint16_t val);

void callbackWithOneBoolArgument(bool state);


#endif /* TEST_CEEDLING_TEST_SUPPORT_TESTCALLBACKS_H_ */
