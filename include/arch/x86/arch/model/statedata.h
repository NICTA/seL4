/*
 * Copyright 2014, General Dynamics C4 Systems
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(GD_GPL)
 */

#ifndef __ARCH_MODEL_STATEDATA_H
#define __ARCH_MODEL_STATEDATA_H

#include <config.h>
#include <types.h>
#include <util.h>
#include <object/structures.h>
#include <arch/types.h>
#include <plat/machine/devices.h>
#include <arch/object/vcpu.h>
#include <arch/object/iospace.h>
#include <plat/machine.h>

#include <mode/model/statedata.h>


#define TSS_IO_MAP_SIZE (65536 / 8 / sizeof(word_t) + 1)

typedef struct {
    tss_t   tss;
    word_t  io_map[TSS_IO_MAP_SIZE];
} PACKED tss_io_t;

NODE_STATE_BEGIN(archNodeState)
/* Interrupt currently being handled, not preserved across kernel entries */
NODE_STATE_DECLARE(interrupt_t, x86KScurInterrupt);
/* Interrupt that the hardware believes we are currently handling (is marked in service
 * in the APIC) but we have not yet gotten around to handling */
NODE_STATE_DECLARE(interrupt_t, x86KSPendingInterrupt);
/* Task State Segment (TSS), contains currently running TCB in ESP0 */
NODE_STATE_DECLARE(tss_io_t, x86KStss);
/* Global Descriptor Table (GDT) */
NODE_STATE_DECLARE(gdt_entry_t, x86KSgdt[GDT_ENTRIES]);
/* Interrupt Descriptor Table (IDT) */
NODE_STATE_DECLARE(idt_entry_t, x86KSidt[IDT_ENTRIES]);
/* Bitmask of all cores should receive the reschedule IPI */
NODE_STATE_DECLARE(word_t, ipiReschedulePending);

#ifdef CONFIG_VTX
NODE_STATE_DECLARE(vcpu_t *, x86KSCurrentVCPU);
#endif

NODE_STATE_DECLARE(word_t, x86KSCurrentFSBase);
NODE_STATE_DECLARE(word_t, x86KSCurrentGSBase);

/* If a GP exception occurs and this is non NULL then the exception should return to
 * this location instead of faulting. In addition the GP exception will clear this
 * back to NULL */
NODE_STATE_DECLARE(word_t, x86KSGPExceptReturnTo);

NODE_STATE_TYPE_DECLARE(modeNodeState, mode);
NODE_STATE_END(archNodeState);

extern asid_pool_t* x86KSASIDTable[];
extern uint32_t x86KScacheLineSizeBits;
extern user_fpu_state_t x86KSnullFpuState ALIGN(MIN_FPU_ALIGNMENT);

extern uint32_t x86KSnumDrhu;
extern vtd_rte_t* x86KSvtdRootTable;
extern uint32_t x86KSnumIOPTLevels;
extern uint32_t x86KSnumIODomainIDBits;
extern uint32_t x86KSFirstValidIODomain;

#ifdef CONFIG_PRINTING
extern uint16_t x86KSconsolePort;
#endif
#ifdef CONFIG_DEBUG_BUILD
extern uint16_t x86KSdebugPort;
#endif

extern x86_irq_state_t x86KSIRQState[];
extern uint32_t x86KStscMhz;
#endif
