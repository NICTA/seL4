/*
 * Copyright 2014, General Dynamics C4 Systems
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(GD_GPL)
 */

#ifndef __ARCH_FASTPATH_H
#define __ARCH_FASTPATH_H

#include <arch/linker.h>
#include <mode/fastpath/fastpath.h>
#include <benchmark/benchmark_track.h>
#include <arch/machine/debug.h>

void slowpath(syscall_t syscall)
NORETURN;

void fastpath_call(word_t cptr, word_t r_msgInfo)
NORETURN SECTION(".vectors.fastpath_call");

void fastpath_reply_recv(word_t cptr, word_t r_msgInfo, word_t reply)
NORETURN SECTION(".vectors.fastpath_reply_recv");

#endif /* __ARCH_FASTPATH_H */

