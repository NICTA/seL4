/*
 * Copyright 2014, General Dynamics C4 Systems
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(GD_GPL)
 */

#include <types.h>
#include <benchmark/benchmark.h>
#include <arch/benchmark.h>
#include <benchmark/benchmark_track.h>
#include <benchmark/benchmark_utilisation.h>
#include <api/syscall.h>
#include <api/failures.h>
#include <api/faults.h>
#include <kernel/cspace.h>
#include <kernel/faulthandler.h>
#include <kernel/thread.h>
#include <kernel/vspace.h>
#include <machine/io.h>
#include <plat/machine/hardware.h>
#include <object/interrupt.h>
#include <model/statedata.h>
#include <string.h>
#include <kernel/traps.h>
#include <arch/machine.h>

#ifdef DEBUG
#include <arch/machine/capdl.h>
#endif

/* The haskell function 'handleEvent' is split into 'handleXXX' variants
 * for each event causing a kernel entry */

exception_t
handleInterruptEntry(void)
{
    irq_t irq;

    irq = getActiveIRQ();

    if (irq != irqInvalid) {
        handleInterrupt(irq);
        Arch_finaliseInterrupt();
    } else {
#ifdef CONFIG_IRQ_REPORTING
        userError("Spurious interrupt!");
#endif
        handleSpuriousIRQ();
    }

    schedule();
    activateThread();

    return EXCEPTION_NONE;
}

exception_t
handleUnknownSyscall(word_t w)
{
#ifdef CONFIG_DEBUG_BUILD
    if (w == SysDebugPutChar) {
        kernel_putchar(getRegister(NODE_STATE(ksCurThread), capRegister));
        return EXCEPTION_NONE;
    }
    if (w == SysDebugHalt) {
        printf("Debug halt syscall from user thread %p\n", NODE_STATE(ksCurThread));
        halt();
    }
    if (w == SysDebugSnapshot) {
        printf("Debug snapshot syscall from user thread %p\n", NODE_STATE(ksCurThread));
        capDL();
        return EXCEPTION_NONE;
    }
    if (w == SysDebugCapIdentify) {
        word_t cptr = getRegister(NODE_STATE(ksCurThread), capRegister);
        lookupCapAndSlot_ret_t lu_ret = lookupCapAndSlot(NODE_STATE(ksCurThread), cptr);
        word_t cap_type = cap_get_capType(lu_ret.cap);
        setRegister(NODE_STATE(ksCurThread), capRegister, cap_type);
        return EXCEPTION_NONE;
    }

    if (w == SysDebugNameThread) {
        /* This is a syscall meant to aid debugging, so if anything goes wrong
         * then assume the system is completely misconfigured and halt */
        const char *name;
        word_t len;
        word_t cptr = getRegister(NODE_STATE(ksCurThread), capRegister);
        lookupCapAndSlot_ret_t lu_ret = lookupCapAndSlot(NODE_STATE(ksCurThread), cptr);
        /* ensure we got a TCB cap */
        word_t cap_type = cap_get_capType(lu_ret.cap);
        if (cap_type != cap_thread_cap) {
            userError("SysDebugNameThread: cap is not a TCB, halting");
            halt();
        }
        /* Add 1 to the IPC buffer to skip the message info word */
        name = (const char*)(lookupIPCBuffer(true, NODE_STATE(ksCurThread)) + 1);
        if (!name) {
            userError("SysDebugNameThread: Failed to lookup IPC buffer, halting");
            halt();
        }
        /* ensure the name isn't too long */
        len = strnlen(name, seL4_MsgMaxLength * sizeof(word_t));
        if (len == seL4_MsgMaxLength * sizeof(word_t)) {
            userError("SysDebugNameThread: Name too long, halting");
            halt();
        }
        setThreadName(TCB_PTR(cap_thread_cap_get_capTCBPtr(lu_ret.cap)), name);
        return EXCEPTION_NONE;
    }
#endif /* CONFIG_DEBUG_BUILD */

#ifdef DANGEROUS_CODE_INJECTION
    if (w == SysDebugRun) {
        ((void (*) (void *))getRegister(NODE_STATE(ksCurThread), capRegister))((void*)getRegister(NODE_STATE(ksCurThread), msgInfoRegister));
        return EXCEPTION_NONE;
    }
#endif

#ifdef CONFIG_ENABLE_BENCHMARKS
    if (w == SysBenchmarkFlushCaches) {
        arch_clean_invalidate_caches();
        return EXCEPTION_NONE;
    } else if (w == SysBenchmarkResetLog) {
#ifdef CONFIG_BENCHMARK_USE_KERNEL_LOG_BUFFER
        if (ksUserLogBuffer == 0) {
            userError("A user-level buffer has to be set before resetting benchmark.\
                    Use seL4_BenchmarkSetLogBuffer\n");
            setRegister(NODE_STATE(ksCurThread), capRegister, seL4_IllegalOperation);
            return EXCEPTION_SYSCALL_ERROR;
        }

        ksLogIndex = 0;
#endif /* CONFIG_BENCHMARK_USE_KERNEL_LOG_BUFFER */
#ifdef CONFIG_BENCHMARK_TRACK_UTILISATION
        benchmark_log_utilisation_enabled = true;
        NODE_STATE(ksIdleThread)->benchmark.utilisation = 0;
        NODE_STATE(ksCurThread)->benchmark.schedule_start_time = ksEnter;
        benchmark_start_time = ksEnter;
        benchmark_arch_utilisation_reset();
#endif /* CONFIG_BENCHMARK_TRACK_UTILISATION */
        setRegister(NODE_STATE(ksCurThread), capRegister, seL4_NoError);
        return EXCEPTION_NONE;
    } else if (w == SysBenchmarkFinalizeLog) {
#ifdef CONFIG_BENCHMARK_USE_KERNEL_LOG_BUFFER
        ksLogIndexFinalized = ksLogIndex;
        setRegister(NODE_STATE(ksCurThread), capRegister, ksLogIndexFinalized);
#endif /* CONFIG_BENCHMARK_USE_KERNEL_LOG_BUFFER */
#ifdef CONFIG_BENCHMARK_TRACK_UTILISATION
        benchmark_utilisation_finalise();
#endif /* CONFIG_BENCHMARK_TRACK_UTILISATION */
        return EXCEPTION_NONE;
    } else if (w == SysBenchmarkSetLogBuffer) {
#ifdef CONFIG_BENCHMARK_USE_KERNEL_LOG_BUFFER
        word_t cptr_userFrame = getRegister(NODE_STATE(ksCurThread), capRegister);

        if (benchmark_arch_map_logBuffer(cptr_userFrame) != EXCEPTION_NONE) {
            setRegister(NODE_STATE(ksCurThread), capRegister, seL4_IllegalOperation);
            return EXCEPTION_SYSCALL_ERROR;
        }

        setRegister(NODE_STATE(ksCurThread), capRegister, seL4_NoError);
        return EXCEPTION_NONE;
#endif /* CONFIG_BENCHMARK_USE_KERNEL_LOG_BUFFER */
    }

#ifdef CONFIG_BENCHMARK_TRACK_UTILISATION
    else if (w == SysBenchmarkGetThreadUtilisation) {
        benchmark_track_utilisation_dump();
        return EXCEPTION_NONE;
    } else if (w == SysBenchmarkResetThreadUtilisation) {
        benchmark_track_reset_utilisation();
        return EXCEPTION_NONE;
    }
#endif /* CONFIG_BENCHMARK_TRACK_UTILISATION */

    else if (w == SysBenchmarkNullSyscall) {
        return EXCEPTION_NONE;
    }
#endif /* CONFIG_ENABLE_BENCHMARKS */

    updateTimestamp(false);
    if (likely(checkBudgetRestart())) {
        current_fault = seL4_Fault_UnknownSyscall_new(w);
        handleFault(NODE_STATE(ksCurThread));
    }

    schedule();
    activateThread();

    return EXCEPTION_NONE;
}

exception_t
handleUserLevelFault(word_t w_a, word_t w_b)
{
    updateTimestamp(false);
    if (likely(checkBudgetRestart())) {
        current_fault = seL4_Fault_UserException_new(w_a, w_b);
        handleFault(NODE_STATE(ksCurThread));
    }
    schedule();
    activateThread();

    return EXCEPTION_NONE;
}

exception_t
handleVMFaultEvent(vm_fault_type_t vm_faultType)
{
    updateTimestamp(false);
    if (likely(checkBudgetRestart())) {
        exception_t status = handleVMFault(NODE_STATE(ksCurThread), vm_faultType);
        if (status != EXCEPTION_NONE) {
            handleFault(NODE_STATE(ksCurThread));
        }
    }

    schedule();
    activateThread();

    return EXCEPTION_NONE;
}


static exception_t
handleInvocation(bool_t isCall, bool_t isBlocking)
{
    seL4_MessageInfo_t info;
    cptr_t cptr;
    lookupCapAndSlot_ret_t lu_ret;
    word_t *buffer;
    exception_t status;
    word_t length;
    tcb_t *thread;

    thread = NODE_STATE(ksCurThread);

    info = messageInfoFromWord(getRegister(thread, msgInfoRegister));
    cptr = getRegister(thread, capRegister);

    /* faulting section */
    lu_ret = lookupCapAndSlot(thread, cptr);

    if (unlikely(lu_ret.status != EXCEPTION_NONE)) {
        userError("Invocation of invalid cap #%lu.", cptr);
        current_fault = seL4_Fault_CapFault_new(cptr, false);

        if (isBlocking) {
            handleFault(thread);
        }

        return EXCEPTION_NONE;
    }

    buffer = lookupIPCBuffer(false, thread);

    status = lookupExtraCaps(thread, buffer, info);

    if (unlikely(status != EXCEPTION_NONE)) {
        userError("Lookup of extra caps failed.");
        if (isBlocking) {
            handleFault(thread);
        }
        return EXCEPTION_NONE;
    }

    /* Syscall error/Preemptible section */
    length = seL4_MessageInfo_get_length(info);
    if (unlikely(length > n_msgRegisters && !buffer)) {
        length = n_msgRegisters;
    }
    status = decodeInvocation(seL4_MessageInfo_get_label(info), length,
                              cptr, lu_ret.slot, lu_ret.cap,
                              current_extra_caps, isBlocking, isCall,
                              buffer);

    if (unlikely(status == EXCEPTION_PREEMPTED)) {
        return status;
    }

    if (unlikely(status == EXCEPTION_SYSCALL_ERROR)) {
        if (isCall) {
            replyFromKernel_error(thread);
        }
        return EXCEPTION_NONE;
    }

    if (unlikely(
                thread_state_get_tsType(thread->tcbState) == ThreadState_Restart)) {
        if (isCall) {
            replyFromKernel_success_empty(thread);
        }
        setThreadState(thread, ThreadState_Running);
    }

    return EXCEPTION_NONE;
}

static void
handleReply(void)
{
    cte_t *callerSlot;
    cap_t callerCap;

    callerSlot = TCB_PTR_CTE_PTR(NODE_STATE(ksCurThread), tcbCaller);
    callerCap = callerSlot->cap;

    switch (cap_get_capType(callerCap)) {
    case cap_reply_cap: {
        tcb_t *caller;

        if (cap_reply_cap_get_capReplyMaster(callerCap)) {
            break;
        }
        caller = TCB_PTR(cap_reply_cap_get_capTCBPtr(callerCap));
        /* Haskell error:
         * "handleReply: caller must not be the current thread" */
        assert(caller != NODE_STATE(ksCurThread));
        doReplyTransfer(NODE_STATE(ksCurThread), caller, callerSlot);
        return;
    }

    case cap_null_cap:
        userError("Attempted reply operation when no reply cap present.");
        return;

    default:
        break;
    }

    fail("handleReply: invalid caller cap");
}

static void
handleRecv(bool_t isBlocking)
{
    word_t epCPtr;
    lookupCap_ret_t lu_ret;

    epCPtr = getRegister(NODE_STATE(ksCurThread), capRegister);

    lu_ret = lookupCap(NODE_STATE(ksCurThread), epCPtr);

    if (unlikely(lu_ret.status != EXCEPTION_NONE)) {
        /* current_lookup_fault has been set by lookupCap */
        current_fault = seL4_Fault_CapFault_new(epCPtr, true);
        handleFault(NODE_STATE(ksCurThread));
        return;
    }

    switch (cap_get_capType(lu_ret.cap)) {
    case cap_endpoint_cap:
        if (unlikely(!cap_endpoint_cap_get_capCanReceive(lu_ret.cap))) {
            current_lookup_fault = lookup_fault_missing_capability_new(0);
            current_fault = seL4_Fault_CapFault_new(epCPtr, true);
            handleFault(NODE_STATE(ksCurThread));
            break;
        }

        deleteCallerCap(NODE_STATE(ksCurThread));
        receiveIPC(NODE_STATE(ksCurThread), lu_ret.cap, isBlocking);
        break;

    case cap_notification_cap: {
        notification_t *ntfnPtr;
        tcb_t *boundTCB;
        ntfnPtr = NTFN_PTR(cap_notification_cap_get_capNtfnPtr(lu_ret.cap));
        boundTCB = (tcb_t*)notification_ptr_get_ntfnBoundTCB(ntfnPtr);
        if (unlikely(!cap_notification_cap_get_capNtfnCanReceive(lu_ret.cap)
                     || (boundTCB && boundTCB != NODE_STATE(ksCurThread)))) {
            current_lookup_fault = lookup_fault_missing_capability_new(0);
            current_fault = seL4_Fault_CapFault_new(epCPtr, true);
            handleFault(NODE_STATE(ksCurThread));
            break;
        }

        receiveSignal(NODE_STATE(ksCurThread), lu_ret.cap, isBlocking);
        break;
    }
    default:
        current_lookup_fault = lookup_fault_missing_capability_new(0);
        current_fault = seL4_Fault_CapFault_new(epCPtr, true);
        handleFault(NODE_STATE(ksCurThread));
        break;
    }
}

static void
handleYield(void)
{
    /* checkBudgetRestart should have failed if we got here */
    assert(refill_sufficient(NODE_STATE(ksCurSC), NODE_STATE(ksConsumed)));
    /* Yield the current remaining budget */
    refill_budget_check(NODE_STATE(ksCurSC), REFILL_HEAD(NODE_STATE(ksCurSC)).rAmount);
    /* we just charged all of the time to the yielding thread */
    NODE_STATE(ksConsumed) = 0;
    endTimeslice();
}

exception_t
handleSyscall(syscall_t syscall)
{
    exception_t ret = EXCEPTION_NONE;
    updateTimestamp(false);

    if (likely(checkBudgetRestart())) {
        switch (syscall) {
        case SysSend:
            ret = handleInvocation(false, true);
           break;

        case SysNBSend:
            ret = handleInvocation(false, false);
            break;

        case SysCall:
            ret = handleInvocation(true, true);
            break;

        case SysRecv:
            handleRecv(true);
            break;

        case SysReply:
            handleReply();
            break;

        case SysReplyRecv:
            handleReply();
            handleRecv(true);
            break;

        case SysNBRecv:
            handleRecv(false);
            break;

        case SysYield:
            handleYield();
            break;

        default:
            fail("Invalid syscall");
        }

        if (unlikely(ret != EXCEPTION_NONE)) {
            irq_t irq = getActiveIRQ();
            if (irq != irqInvalid) {
                if (checkBudget()) {
                    commitTime();
                }
                handleInterrupt(irq);
                Arch_finaliseInterrupt();
            }
        }
    }

    schedule();
    activateThread();

    return EXCEPTION_NONE;
}
