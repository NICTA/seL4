/*
 * Copyright 2014, General Dynamics C4 Systems
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(GD_GPL)
 */

#ifndef __KERNEL_THREAD_H
#define __KERNEL_THREAD_H

#include <types.h>
#include <util.h>
#include <object/structures.h>
#include <arch/machine.h>
#include <machine/timer.h>
#include <mode/machine.h>

static inline CONST word_t
ready_queues_index(word_t dom, word_t prio)
{
    if (CONFIG_NUM_DOMAINS > 1) {
        return dom * CONFIG_NUM_PRIORITIES + prio;
    } else {
        assert(dom == 0);
        return prio;
    }
}

static inline CONST word_t
prio_to_l1index(word_t prio)
{
    return (prio >> wordRadix);
}

static inline CONST word_t
l1index_to_prio(word_t l1index)
{
    return (l1index << wordRadix);
}

static inline bool_t
isCurThreadExpired(void)
{
    return NODE_STATE(ksCurThread)->tcbSchedContext->scRemaining <
           (NODE_STATE(ksConsumed) + getKernelWcetTicks());
}

static inline bool_t
isCurDomainExpired(void)
{
    return CONFIG_NUM_DOMAINS > 1 &&
           NODE_STATE(ksDomainTime) < (NODE_STATE(ksConsumed) + getKernelWcetTicks());
}

static inline void
commitTime(sched_context_t *sc)
{
    assert(sc->scCore == SMP_TERNARY(getCurrentCPUIndex(), 0));
    if (unlikely(sc->scRemaining < NODE_STATE(ksConsumed))) {
        /* avoid underflow */
        sc->scRemaining = 0;
    } else {
        sc->scRemaining -= NODE_STATE(ksConsumed);
    }

    if (CONFIG_NUM_DOMAINS > 1) {
        if (unlikely(ksDomainTime < NODE_STATE(ksConsumed))) {
            ksDomainTime = 0;
        } else {
            ksDomainTime -= NODE_STATE(ksConsumed);
        }
    }

    NODE_STATE(ksConsumed) = 0llu;
    NODE_STATE(ksCurTime) += 1llu;
}

static inline void
rollbackTime(void)
{
    NODE_STATE(ksCurTime) -= NODE_STATE(ksConsumed);
    NODE_STATE(ksConsumed) = 0llu;
}

void configureIdleThread(tcb_t *tcb);
void activateThread(void) VISIBLE;
void suspend(tcb_t *target);
void restart(tcb_t *target);
void doIPCTransfer(tcb_t *sender, endpoint_t *endpoint,
                   word_t badge, bool_t grant, tcb_t *receiver);
void doReplyTransfer(tcb_t *sender, tcb_t *receiver, cte_t *slot);
void doNormalTransfer(tcb_t *sender, word_t *sendBuffer, endpoint_t *endpoint,
                      word_t badge, bool_t canGrant, tcb_t *receiver,
                      word_t *receiveBuffer);
void doFaultTransfer(word_t badge, tcb_t *sender, tcb_t *receiver,
                     word_t *receiverIPCBuffer);
void doNBRecvFailedTransfer(tcb_t *thread);
void schedule(void);
void chooseThread(void);
void switchToThread(tcb_t *thread) VISIBLE;
void switchToIdleThread(void);
void setDomain(tcb_t *tptr, dom_t dom);
void setPriority(tcb_t *tptr, prio_t prio);
void setMCPriority(tcb_t *tptr, prio_t mcp);
void scheduleTCB(tcb_t *tptr);
void attemptSwitchTo(tcb_t *tptr);
void switchIfRequiredTo(tcb_t *tptr);
void setThreadState(tcb_t *tptr, _thread_state_t ts) VISIBLE;
void rescheduleRequired(void);

/* Update the kernels timestamp and stores in ksCurTime.
 * The difference between the previous kernel timestamp and the one just read
 * is stored in ksConsumed.
 *
 * Should be called on every kernel entry
 * where threads can be billed.
 *
 * @pre (NODE_STATE(ksConsumed) == 0
 */
static inline void
updateTimestamp(bool_t incrementConsumedTime)
{
    time_t prev = NODE_STATE(ksCurTime);
    NODE_STATE(ksCurTime) = getCurrentTime();
    if ((config_set(CONFIG_DEBUG_BUILD) || config_set(CONFIG_PRINTING))
        && incrementConsumedTime) {
        /* When executing debugging functions in the kernel that
         * increase the duration of a syscall, it's useful to call
         * updateTimestamp() in those debugging functions (such as printf).
         *
         * The reason we update the kernel timestamp is because if we don't,
         * the timestamp that will be used in setDeadline() will be very stale,
         * and the value programmed into the sched-timer will have considerable
         * skew.
         *
         * The standard case is below.
         */
        NODE_STATE(ksConsumed) += NODE_STATE(ksCurTime) - prev;
    } else {
        /* This is the standard case: we will usually want to track the
         * consumed time since the last call.
         */
        NODE_STATE(ksConsumed) = NODE_STATE(ksCurTime) - prev;
    }
}

/* Check if the current thread/domain budget has expired.
 * if it has, bill the thread, add it o the scheduler and
 * set up a reschedule.
 *
 * @return true if the thread/domain has enough budget to
 *              get through the current kernel operation.
 */
bool_t checkBudget(void);

/* Everything checkBudget does, but also set the thread
 * state to ThreadState_Restart. To be called from kernel entries
 * where the operation should be restarted once the current thread
 * has budget again.
 */
bool_t checkBudgetRestart(void);

/* Set the next kernel tick, which is either the end of the current
 * domains timeslice OR the end of the current threads timeslice.
 */
void setNextInterrupt(void);

/* End the timeslice for the current thread.
 * This will recharge the threads timeslice and place it at the
 * end of the scheduling queue for its priority.
 */
void endTimeslice(void);

static inline void
checkReschedule(void)
{
    if (isCurThreadExpired()) {
        endTimeslice();
    } else if (isCurDomainExpired()) {
        rescheduleRequired();
    }
}

#endif
