/*
 * Copyright 2014, General Dynamics C4 Systems
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(GD_GPL)
 */

#include <config.h>
#include <types.h>
#include <machine/io.h>
#include <kernel/vspace.h>
#include <arch/machine.h>
#include <arch/kernel/vspace.h>
#include <plat/machine.h>
#include <arch/linker.h>
#include <plat/machine/devices.h>
#include <plat/machine/hardware.h>


#define TIMER_INTERVAL_MS (CONFIG_TIMER_TICK_MS)

#define TIOCP_CFG_SOFTRESET BIT(0)

#define TIER_MATCHENABLE BIT(0)
#define TIER_OVERFLOWENABLE BIT(1)
#define TIER_COMPAREENABLE BIT(2)

#define TCLR_AUTORELOAD BIT(1)
#define TCLR_COMPAREENABLE BIT(6)
#define TCLR_STARTTIMER BIT(0)

#define TISR_OVF_FLAG (BIT(0) | BIT(1) | BIT(2))

#define TICKS_PER_SECOND 32768 // 32KHz
#define TIMER_INTERVAL_TICKS ((int)(1UL * TIMER_INTERVAL_MS * TICKS_PER_SECOND / 1000))

volatile struct TIMER_map {
    uint32_t tidr; // 00h TIDR Identification Register
    uint32_t padding1[3];
    uint32_t cfg; // 10h TIOCP_CFG Timer OCP Configuration Register
    uint32_t padding2[3];
    uint32_t tieoi; // 20h IRQ_EOI Timer IRQ End-Of-Interrupt Register
    uint32_t tisrr; // 24h IRQSTATUS_RAW Timer IRQSTATUS Raw Register
    uint32_t tisr; // 28h IRQSTATUS Timer IRQSTATUS Register
    uint32_t tier; // 2Ch IRQSTATUS_SET Timer IRQENABLE Set Register
    uint32_t ticr; // 30h IRQSTATUS_CLR Timer IRQENABLE Clear Register
    uint32_t twer; // 34h IRQWAKEEN Timer IRQ Wakeup Enable Register
    uint32_t tclr; // 38h TCLR Timer Control Register
    uint32_t tcrr; // 3Ch TCRR Timer Counter Register
    uint32_t tldr; // 40h TLDR Timer Load Register
    uint32_t ttgr; // 44h TTGR Timer Trigger Register
    uint32_t twps; // 48h TWPS Timer Write Posted Status Register
    uint32_t tmar; // 4Ch TMAR Timer Match Register
    uint32_t tcar1; // 50h TCAR1 Timer Capture Register
    uint32_t tsicr; // 54h TSICR Timer Synchronous Interface Control Register
    uint32_t tcar2; // 58h TCAR2 Timer Capture Register
} *timer = (volatile void*)DMTIMER0_PPTR;

#define WDT_REG(base, off) ((volatile uint32_t *)((base) + (off)))
#define WDT_REG_WWPS 0x34
#define WDT_REG_WSPR 0x48
#define WDT_WWPS_PEND_WSPR BIT(4)

static BOOT_CODE void
disableWatchdog(void)
{
    uint32_t wdt = WDT1_PPTR;

    // am335x ref man, sec 20.4.3.8
    *WDT_REG(wdt, WDT_REG_WSPR) = 0xaaaa;
    while ((*WDT_REG(wdt, WDT_REG_WWPS) & WDT_WWPS_PEND_WSPR)) {
        continue;
    }
    *WDT_REG(wdt, WDT_REG_WSPR) = 0x5555;
    while ((*WDT_REG(wdt, WDT_REG_WWPS) & WDT_WWPS_PEND_WSPR)) {
        continue;
    }
}

/*
 * Enable DMTIMER clocks, otherwise their registers wont be accessible.
 * This could be moved out of kernel.
 */
static BOOT_CODE void
enableTimers(void)
{
    uint32_t cmper = CMPER_PPTR;

    /* XXX repeat this for DMTIMER4..7 */
    /* select clock */
    *CMPER_REG(cmper, CMPER_CLKSEL_TIMER3) = CMPER_CKLSEL_MOSC;
    while ((*CMPER_REG(cmper, CMPER_CLKSEL_TIMER3) & 3) != CMPER_CKLSEL_MOSC) {
        continue;
    }

    /* enable clock */
    *CMPER_REG(cmper, CMPER_TIMER3_CLKCTRL) = CMPER_CLKCTRL_ENABLE;
    while ((*CMPER_REG(cmper, CMPER_TIMER3_CLKCTRL) & 3) != CMPER_CLKCTRL_ENABLE) {
        continue;
    }
}

/* Configure dmtimer0 as kernel preemption timer */
/**
   DONT_TRANSLATE
 */
BOOT_CODE void
initTimer(void)
{
    int timeout;

    disableWatchdog();
    enableTimers();

    timer->cfg = TIOCP_CFG_SOFTRESET;

    for (timeout = 10000; (timer->cfg & TIOCP_CFG_SOFTRESET) && timeout > 0; timeout--)
        ;
    if (!timeout) {
        printf("init timer failed\n");
        return;
    }

    maskInterrupt(/*disable*/ true, DMTIMER0_IRQ);

    /* Set the reload value */
    timer->tldr = 0xFFFFFFFFUL - TIMER_INTERVAL_TICKS;

    /* Enables interrupt on overflow */
    timer->tier = TIER_OVERFLOWENABLE;

    /* Clear the read register */
    timer->tcrr = 0xFFFFFFFFUL - TIMER_INTERVAL_TICKS;

    /* Set autoreload and start the timer */
    timer->tclr = TCLR_AUTORELOAD | TCLR_STARTTIMER;
}

/**
   DONT_TRANSLATE
 */
BOOT_CODE void
initIRQController(void)
{
    intc->intcps_sysconfig = INTCPS_SYSCONFIG_SOFTRESET;
    while (!(intc->intcps_sysstatus & INTCPS_SYSSTATUS_RESETDONE)) ;
}

BOOT_CODE void cpu_initLocalIRQController(void) {}

void plat_cleanL2Range(paddr_t start, paddr_t end) {}
void plat_invalidateL2Range(paddr_t start, paddr_t end) {}
void plat_cleanInvalidateL2Range(paddr_t start, paddr_t end) {}
void plat_cleanInvalidateCache(void) {}
