/*
 * modbus_timer.c
 *
 *  Created on: 4 мая 2017 г.
 *      Author: p.borisenko
 */

#include "modbus_timer.h"

static void (*timer_callback)(void);
static bool timer_oneshot;

__interrupt void mbTimerIsr(void) {
    if (timer_callback != NULL) {
        timer_callback();
    }
    if (timer_oneshot) {
        CpuTimer2Regs.TCR.bit.TSS= 1;   // Stop timer
    }
    CpuTimer2Regs.TCR.bit.TIF= 1;   // Clear Interrupt flag
}

void mbTimerInit(uint32_t cpuFreqMhz, uint32_t period_us, bool oneshot) {
    uint32_t     periodInClocks;
    timer_oneshot= oneshot;
    // Make sure timers are stopped:
    CpuTimer2Regs.TCR.bit.TSS = 1;
    // Initialize pre-scale counter to divide by 1 (SYSCLKOUT):
    CpuTimer2Regs.TPR.all  = 0;
    CpuTimer2Regs.TPRH.all = 0;
    // Initialize timer period:
    periodInClocks= (uint32_t) (cpuFreqMhz * period_us);
    CpuTimer2Regs.PRD.all= periodInClocks - 1;// Counter decrements PRD+1 times each period

    CpuTimer2Regs.TCR.bit.SOFT = 0;
    CpuTimer2Regs.TCR.bit.FREE = 0;     // Timer Free Run Disabled

    EALLOW;
    PieVectTable.TINT2= mbTimerIsr;     // Attach ISR to interrupt vector
    EDIS;

    CpuTimer2Regs.TCR.bit.TIF= 1;   // Clear Interrupt flag
    CpuTimer2Regs.TCR.bit.TIE = 1;      // 0 = Disable/ 1 = Enable Timer Interrupt
    IER|= M_INT14;      // Enable CPU Interrupts
}

void mbTimerStart(void(*cb)()) {
    // Initialize timer control register:
    CpuTimer2Regs.TCR.bit.TSS = 1;      // 1 = Stop timer, 0 = Start/Restart Timer
    timer_callback= cb;
    CpuTimer2Regs.TCR.bit.TRB = 1;      // 1 = reload timer
    CpuTimer2Regs.TCR.bit.TIF= 1;   // Clear Interrupt flag
    CpuTimer2Regs.TCR.bit.TSS= 0;   // Start timer
}




