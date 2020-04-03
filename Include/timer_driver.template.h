/*
 * modbus_timer.h
 *
 *  Created on: 4 мая 2017 г.
 *      Author: p.borisenko
 *      Hardware: tms320f2803x
 */

#ifndef INCLUDE_MODBUS_TIMER_H_
#define INCLUDE_MODBUS_TIMER_H_

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <DSP28x_Project.h>

void mbTimerInit(uint32_t cpuFreqMhz, uint32_t period_us, bool oneshot);

void mbTimerStart(void(*cb)(void));

#endif /* INCLUDE_MODBUS_TIMER_H_ */
