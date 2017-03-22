/*
 * Copyright 2016, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(D61_GPL)
 */
#ifndef __PLAT_MACHINE_TIMER_H
#define __PLAT_MACHINE_TIMER_H

#include <config.h>
#include <basic_types.h>
#include <arch/linker.h>

/* see tools/reciprocal.py for calculation of CLK_MAGIC and CLK_SHIFT */
#define CLK_MAGIC 2863311531
#define CLK_SHIFT 36
#define TIMER_CLOCK_HZ     24000000llu

#include <arch/machine/generic_timer.h>

compile_assert(magic_will_work, TIMER_CLOCK_MHZ == 24llu);

static inline PURE ticks_t
getTimerPrecision(void)
{
    return usToTicks(1u);
}

CONST static inline time_t
getKernelWcetUs(void)
{
    return 10u;
}

BOOT_CODE void initTimer(void);
#endif /* __PLAT_MACHINE_TIMER_H */
