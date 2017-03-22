/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(D61_GPL)
 */
#ifndef __KERNEL_SPORADIC_H
#define __KERNEL_SPORADIC_H
/* This header presents the interface for sporadic servers,
 * implemented according to Stankcovich et. al in
 * "Defects of the POSIX Spoardic Server and How to correct them",
 * although without the priority management.
 *
 * Briefly, a sporadic server is a period and a queue of refills. Each
 * refill consists of an amount, and a period. No thread is allowed to consume
 * more than amount ticks per period.
 *
 * The sum of all refill amounts in the refill queue is always the budget of the scheduling context -
 * that is it should never change, unless it is being updated / configured.
 *
 * Every time budget is consumed, that amount of budget is scheduled
 * for reuse in period time. If the refill queue is full (the queue's
 * minimum size is 2, and can be configured by the user per scheduling context
 * above this) the next refill is merged.
 */
#include <types.h>
#include <util.h>
#include <object/structures.h>
#include <machine/timer.h>
#include <model/statedata.h>

/* To do an operation in the kernel, the thread must have
 * at least this much budget - see comment on refill_sufficient */
#define MIN_BUDGET_US (2u * getKernelWcetUs())
#define MIN_BUDGET    (2u * getKernelWcetTicks())

/* Short hand for accessing refill queue items */
#define REFILL_INDEX(sc, index) ((sc)->scRefills[(index)])
#define REFILL_HEAD(sc) REFILL_INDEX((sc), (sc)->scRefillHead)
#define REFILL_TAIL(sc) REFILL_INDEX((sc), (sc)->scRefillTail)

/* Return the amount of budget this scheduling context
 * has available if usage is charged to it. */
static inline ticks_t
refill_capacity(sched_context_t *sc, ticks_t usage)
{
    if (unlikely(usage > REFILL_HEAD(sc).rAmount)) {
        return 0;
    }

    return REFILL_HEAD(sc).rAmount - usage;
}

/*
 * Return true if the head refill has sufficient capacity
 * to enter and exit the kernel after usage is charged to it.
 */
static inline bool_t
refill_sufficient(sched_context_t *sc, ticks_t usage)
{
    return refill_capacity(sc, usage) >= MIN_BUDGET;
}

/*
 * Return true if the refill is eligible to be used.
 * This indicates if the thread bound to the sc can be placed
 * into the scheduler, otherwise it needs to go into the release queue
 * to wait.
 */
static inline bool_t
refill_ready(sched_context_t *sc)
{
    return REFILL_HEAD(sc).rTime <= (NODE_STATE(ksCurTime) + getKernelWcetTicks());
}

/* Create a new refill in a non-active sc */
void refill_new(sched_context_t *sc, word_t max_refills, ticks_t budget, ticks_t period);

/* Update refills in an active sc without violating bandwidth constraints */
void refill_update(sched_context_t *sc, ticks_t new_period, ticks_t new_budget, word_t new_max_refills);


/* Charge the head refill its entire amount.
 *
 * `used` amount from its current replenishment without
 * depleting the budget, i.e refill_expired returns false.
 *
 * return any uncharged usage.
 */
ticks_t refill_budget_check(sched_context_t *sc, ticks_t used);

/*
 * Charge a scheduling context `used` amount from its
 * current refill. This will split the refill, leaving whatever is
 * left over at the head of the refill.
 */
void refill_split_check(sched_context_t *sc, ticks_t used);

/*
 * This is called when a thread is eligible to start running: it
 * iterates through the refills queue and merges any
 * refills that overlap.
 */
void refill_unblock_check(sched_context_t *sc);

#endif
