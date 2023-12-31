/*
    FreeRTOS V8.2.0 - Copyright (C) 2015 Real Time Engineers Ltd.
    All rights reserved

    VISIT http://www.FreeRTOS.org TO ENSURE YOU ARE USING THE LATEST VERSION.

    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation >>!AND MODIFIED BY!<< the FreeRTOS exception.

	***************************************************************************
    >>!   NOTE: The modification to the GPL is included to allow you to     !<<
    >>!   distribute a combined work that includes FreeRTOS without being   !<<
    >>!   obliged to provide the source code for proprietary components     !<<
    >>!   outside of the FreeRTOS kernel.                                   !<<
	***************************************************************************

    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.  Full license text is available on the following
    link: http://www.freertos.org/a00114.html

    ***************************************************************************
     *                                                                       *
     *    FreeRTOS provides completely free yet professionally developed,    *
     *    robust, strictly quality controlled, supported, and cross          *
     *    platform software that is more than just the market leader, it     *
     *    is the industry's de facto standard.                               *
     *                                                                       *
     *    Help yourself get started quickly while simultaneously helping     *
     *    to support the FreeRTOS project by purchasing a FreeRTOS           *
     *    tutorial book, reference manual, or both:                          *
     *    http://www.FreeRTOS.org/Documentation                              *
     *                                                                       *
    ***************************************************************************

    http://www.FreeRTOS.org/FAQHelp.html - Having a problem?  Start by reading
	the FAQ page "My application does not run, what could be wrong?".  Have you
	defined configASSERT()?

	http://www.FreeRTOS.org/support - In return for receiving this top quality
	embedded software for free we request you assist our global community by
	participating in the support forum.

	http://www.FreeRTOS.org/training - Investing in training allows your team to
	be as productive as possible as early as possible.  Now you can receive
	FreeRTOS training directly from Richard Barry, CEO of Real Time Engineers
	Ltd, and the world's leading authority on the world's leading RTOS.

    http://www.FreeRTOS.org/plus - A selection of FreeRTOS ecosystem products,
    including FreeRTOS+Trace - an indispensable productivity tool, a DOS
    compatible FAT file system, and our tiny thread aware UDP/IP stack.

    http://www.FreeRTOS.org/labs - Where new FreeRTOS products go to incubate.
    Come and try FreeRTOS+TCP, our new open source TCP/IP stack for FreeRTOS.

    http://www.OpenRTOS.com - Real Time Engineers ltd. license FreeRTOS to High
    Integrity Systems ltd. to sell under the OpenRTOS brand.  Low cost OpenRTOS
    licenses offer ticketed support, indemnification and commercial middleware.

    http://www.SafeRTOS.com - High Integrity Systems also provide a safety
    engineered and independently SIL3 certified version for use in safety and
    mission critical applications that require provable dependability.

    1 tab == 4 spaces!
*/

/*******************************************************************************
// Copyright (c) 2003-2015 Cadence Design Systems, Inc.
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
--------------------------------------------------------------------------------
*/

#include <stdlib.h>
#include <xtensa/config/core.h>

#include "xtensa_rtos.h"

#include "rom/ets_sys.h"

#include "FreeRTOS.h"
#include "task.h"

/* Defined in portasm.h */
extern void _frxt_tick_timer_init(void);

/* Defined in xtensa_context.S */
extern void _xt_coproc_init(void);


/*-----------------------------------------------------------*/

unsigned port_xSchedulerRunning[portNUM_PROCESSORS] = {0}; // Duplicate of inaccessible xSchedulerRunning; needed at startup to avoid counting nesting
unsigned port_interruptNesting[portNUM_PROCESSORS] = {0};  // Interrupt nesting level

/*-----------------------------------------------------------*/

// User exception dispatcher when exiting
void _xt_user_exit(void);

/* 
 * Stack initialization
 */
#if portUSING_MPU_WRAPPERS
StackType_t *pxPortInitialiseStack( StackType_t *pxTopOfStack, TaskFunction_t pxCode, void *pvParameters, BaseType_t xRunPrivileged )
#else
StackType_t *pxPortInitialiseStack( StackType_t *pxTopOfStack, TaskFunction_t pxCode, void *pvParameters )
#endif
{
	StackType_t *sp, *tp;
	XtExcFrame  *frame;
	#if XCHAL_CP_NUM > 0
	uint32_t *p;
	#endif

	/* Create interrupt stack frame aligned to 16 byte boundary */
	sp = (StackType_t *) (((UBaseType_t)(pxTopOfStack + 1) - XT_CP_SIZE - XT_STK_FRMSZ) & ~0xf);

	/* Clear the entire frame (do not use memset() because we don't depend on C library) */
	for (tp = sp; tp <= pxTopOfStack; ++tp)
		*tp = 0;

	frame = (XtExcFrame *) sp;

	/* Explicitly initialize certain saved registers */
	frame->pc   = (UBaseType_t) pxCode;             /* task entrypoint                */
	frame->a0   = 0;                                /* to terminate GDB backtrace     */
	frame->a1   = (UBaseType_t) sp + XT_STK_FRMSZ;  /* physical top of stack frame    */
	frame->exit = (UBaseType_t) _xt_user_exit;      /* user exception exit dispatcher */

	/* Set initial PS to int level 0, EXCM disabled ('rfe' will enable), user mode. */
	/* Also set entry point argument parameter. */
	#ifdef __XTENSA_CALL0_ABI__
	frame->a2 = (UBaseType_t) pvParameters;
	frame->ps = PS_UM | PS_EXCM;
	#else
	/* + for windowed ABI also set WOE and CALLINC (pretend task was 'call4'd). */
	frame->a6 = (UBaseType_t) pvParameters;
	frame->ps = PS_UM | PS_EXCM | PS_WOE | PS_CALLINC(1);
	#endif

	#ifdef XT_USE_SWPRI
	/* Set the initial virtual priority mask value to all 1's. */
	frame->vpri = 0xFFFFFFFF;
	#endif

	#if XCHAL_CP_NUM > 0
	/* Init the coprocessor save area (see xtensa_context.h) */
	/* No access to TCB here, so derive indirectly. Stack growth is top to bottom.
         * //p = (uint32_t *) xMPUSettings->coproc_area;
	 */
	p = (uint32_t *)(((uint32_t) pxTopOfStack - XT_CP_SIZE) & ~0xf);
	p[0] = 0;
	p[1] = 0;
	p[2] = (((uint32_t) p) + 12 + XCHAL_TOTAL_SA_ALIGN - 1) & -XCHAL_TOTAL_SA_ALIGN;
	#endif

	return sp;
}

/*-----------------------------------------------------------*/

void vPortEndScheduler( void )
{
	/* It is unlikely that the Xtensa port will get stopped.  If required simply
	disable the tick interrupt here. */
}

/*-----------------------------------------------------------*/

BaseType_t xPortStartScheduler( void )
{
	// Interrupts are disabled at this point and stack contains PS with enabled interrupts when task context is restored

	#if XCHAL_CP_NUM > 0
	/* Initialize co-processor management for tasks. Leave CPENABLE alone. */
	_xt_coproc_init();
	#endif

	/* Init the tick divisor value */
	_xt_tick_divisor_init();

	/* Setup the hardware to generate the tick. */
	_frxt_tick_timer_init();

	port_xSchedulerRunning[xPortGetCoreID()] = 1;

	// Cannot be directly called from C; never returns
	__asm__ volatile ("call0    _frxt_dispatch\n");

	/* Should not get here. */
	return pdTRUE;
}
/*-----------------------------------------------------------*/

BaseType_t xPortSysTickHandler( void )
{
	BaseType_t ret;

	portbenchmarkIntLatency();
	ret = xTaskIncrementTick();
	if( ret != pdFALSE )
	{
		portYIELD_FROM_ISR();
	}

	return ret;
}
/*-----------------------------------------------------------*/

/*
 * Used to set coprocessor area in stack. Current hack is to reuse MPU pointer for coprocessor area.
 */
#if portUSING_MPU_WRAPPERS
void vPortStoreTaskMPUSettings( xMPU_SETTINGS *xMPUSettings, const struct xMEMORY_REGION * const xRegions, StackType_t *pxBottomOfStack, uint16_t usStackDepth )
{
	#if XCHAL_CP_NUM > 0
	xMPUSettings->coproc_area = (StackType_t*)((((uint32_t)(pxBottomOfStack + usStackDepth - 1)) - XT_CP_SIZE ) & ~0xf);


	/* NOTE: we cannot initialize the coprocessor save area here because FreeRTOS is going to
         * clear the stack area after we return. This is done in pxPortInitialiseStack().
	 */
	#endif
}
#endif



/*
 * Wrapper for the Xtensa compare-and-set instruction. This subroutine will atomically compare
 * *addr to compare, and if it's the same, will set *addr to set. It will return the old value 
 * of *addr.
 *
 * Note: the NOPs are needed on the ESP31 processor but superfluous on the ESP32.
 *
 * Warning: From the ISA docs: in some (unspecified) cases, the s32c1i instruction may return the
 * *bitwise inverse* of the old mem if the mem wasn't written. This doesn't seem to happen on the
 * ESP32, though. (Would show up directly if it did because the magic wouldn't match.)
 */
uint32_t uxPortCompareSet(volatile uint32_t *mux, uint32_t compare, uint32_t set)
{
	__asm__ __volatile__ (
		"WSR 	    %2,SCOMPARE1 \n" //initialize SCOMPARE1
		"ISYNC      \n" //wait sync
		"S32C1I     %0, %1, 0	 \n" //store id into the lock, if the lock is the same as comparel. Otherwise, no write-access
		:"=r"(set) \
		:"r"(mux), "r"(compare), "0"(set) \
		);
	return set;
}

/*
 * For kernel use: Initialize a per-CPU mux. Mux will be initialized unlocked.
 */
void vPortCPUInitializeMutex(portMUX_TYPE *mux) {
#ifdef portMUX_DEBUG
	ets_printf("Initializing mux %p\n", mux);
	mux->lastLockedFn="(never locked)";
	mux->lastLockedLine=-1;
#endif
	mux->mux=portMUX_FREE_VAL;
}


/*
 * For kernel use: Acquire a per-CPU mux. Spinlocks, so don't hold on to these muxes for too long.
 */
#ifdef portMUX_DEBUG
void vPortCPUAcquireMutex(portMUX_TYPE *mux, const char *fnName, int line) {
#else
void vPortCPUAcquireMutex(portMUX_TYPE *mux) {
#endif
	uint32_t res;
	uint32_t recCnt;
	unsigned int irqStatus;
#ifdef portMUX_DEBUG
	uint32_t cnt=(1<<16);
	if ( (mux->mux & portMUX_MAGIC_MASK) != portMUX_MAGIC_VAL ) {
		ets_printf("ERROR: vPortCPUAcquireMutex: mux %p is uninitialized (0x%X)!\n", mux, mux->mux);
		mux->mux=portMUX_FREE_VAL;
	}
#endif

	irqStatus=portENTER_CRITICAL_NESTED();
	do {
		//Lock mux if it's currently unlocked
		res=uxPortCompareSet(&mux->mux, portMUX_FREE_VAL, (xPortGetCoreID()<<portMUX_VAL_SHIFT)|portMUX_MAGIC_VAL);
		//If it wasn't free and we're the owner of the lock, we are locking recursively.
		if ( (res != portMUX_FREE_VAL) && (((res&portMUX_VAL_MASK)>>portMUX_VAL_SHIFT) == xPortGetCoreID()) ) {
			//Mux was already locked by us. Just bump the recurse count by one.
			recCnt=(res&portMUX_CNT_MASK)>>portMUX_CNT_SHIFT;
			recCnt++;
#ifdef portMUX_DEBUG_RECURSIVE
			ets_printf("Recursive lock: recCnt=%d last non-recursive lock %s line %d, curr %s line %d\n", recCnt, mux->lastLockedFn, mux->lastLockedLine, fnName, line);
#endif
			mux->mux=portMUX_MAGIC_VAL|(recCnt<<portMUX_CNT_SHIFT)|(xPortGetCoreID()<<portMUX_VAL_SHIFT);
			break;
		}
		cnt--;
#ifdef portMUX_DEBUG
		if (cnt==0) {
			ets_printf("Timeout on mux! last non-recursive lock %s line %d, curr %s line %d\n", mux->lastLockedFn, mux->lastLockedLine, fnName, line);
			ets_printf("Mux value %X\n", mux->mux);
		}
#endif
	} while (res!=portMUX_FREE_VAL);
#ifdef portMUX_DEBUG
	if (res==portMUX_FREE_VAL) { //initial lock
		mux->lastLockedFn=fnName;
		mux->lastLockedLine=line;
	}
#endif
	portEXIT_CRITICAL_NESTED(irqStatus);
}

/*
 * For kernel use: Release a per-CPU mux. Returns true if everything is OK, false if mux
 * was already unlocked or is locked by a different core.
 */
#ifdef portMUX_DEBUG
portBASE_TYPE vPortCPUReleaseMutex(portMUX_TYPE *mux, const char *fnName, int line) {
#else
portBASE_TYPE vPortCPUReleaseMutex(portMUX_TYPE *mux) {
#endif
	uint32_t res=0;
	uint32_t recCnt;
	unsigned int irqStatus;
	portBASE_TYPE ret=pdTRUE;
//	ets_printf("Unlock %p\n", mux);
	irqStatus=portENTER_CRITICAL_NESTED();
#ifdef portMUX_DEBUG
	const char *lastLockedFn=mux->lastLockedFn;
	int lastLockedLine=mux->lastLockedLine;
	mux->lastLockedFn=fnName;
	mux->lastLockedLine=line;
	if ( (mux->mux & portMUX_MAGIC_MASK) != portMUX_MAGIC_VAL ) ets_printf("ERROR: vPortCPUReleaseMutex: mux %p is uninitialized (0x%X)!\n", mux, mux->mux);
#endif
	//Unlock mux if it's currently locked with a recurse count of 0
	res=uxPortCompareSet(&mux->mux, (xPortGetCoreID()<<portMUX_VAL_SHIFT)|portMUX_MAGIC_VAL, portMUX_FREE_VAL);

	if ( res == portMUX_FREE_VAL ) {
#ifdef portMUX_DEBUG
		ets_printf("ERROR: vPortCPUReleaseMutex: mux %p was already unlocked!\n", mux);
		ets_printf("Last non-recursive unlock %s line %d, curr unlock %s line %d\n", lastLockedFn, lastLockedLine, fnName, line);
#endif
		ret=pdFALSE;
	} else if ( ((res&portMUX_VAL_MASK)>>portMUX_VAL_SHIFT) != xPortGetCoreID() ) {
#ifdef portMUX_DEBUG
		ets_printf("ERROR: vPortCPUReleaseMutex: mux %p wasn't locked by this core (%d) but by core %d (ret=%x, mux=%x).\n", mux, xPortGetCoreID(), ((res&portMUX_VAL_MASK)>>portMUX_VAL_SHIFT), res, mux->mux);
		ets_printf("Last non-recursive lock %s line %d\n", lastLockedFn, lastLockedLine);
		ets_printf("Called by %s line %d\n", fnName, line);
#endif
		ret=pdFALSE;
	} else if ( ((res&portMUX_CNT_MASK)>>portMUX_CNT_SHIFT)!=0) {
		//We locked this, but the reccount isn't zero. Decrease refcount and continue.
		recCnt=(res&portMUX_CNT_MASK)>>portMUX_CNT_SHIFT;
		recCnt--;
#ifdef portMUX_DEBUG_RECURSIVE
		ets_printf("Recursive unlock: recCnt=%d last locked %s line %d, curr %s line %d\n", recCnt, lastLockedFn, lastLockedLine, fnName, line);
#endif
		mux->mux=portMUX_MAGIC_VAL|(recCnt<<portMUX_CNT_SHIFT)|(xPortGetCoreID()<<portMUX_VAL_SHIFT);
	}
	portEXIT_CRITICAL_NESTED(irqStatus);
	return ret;
}




