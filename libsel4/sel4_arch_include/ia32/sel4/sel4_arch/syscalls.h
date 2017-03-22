/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef __LIBSEL4_SEL4_ARCH_SYSCALLS_H
#define __LIBSEL4_SEL4_ARCH_SYSCALLS_H

#include <autoconf.h>
#include <sel4/arch/functions.h>
#include <sel4/types.h>

/*
 * A general description of the x86_sys_ functions and what they do can be found in
 * the ARM aarch32 syscalls.h file (substituting ARM for x86)
 *
 * Further there are two version of every function, one that supports Position
 * Independent Code, and one that does not. The PIC variant works to preserve
 * EBX, which is used by the compiler, and has to do additional work to save/restore
 * and juggle the contents of EBX as the kernel ABI uses EBX
 */

#if defined(__pic__)

static inline void
x86_sys_send(seL4_Word sys, seL4_Word dest, seL4_Word info, seL4_Word mr1)
{
    asm volatile (
        "pushl %%ebp       \n"
        "pushl %%ebx       \n"
        "movl %%esp, %%ecx \n"
        "movl %%edx, %%ebx \n"
        "leal 1f, %%edx    \n"
        "1:                \n"
        "sysenter          \n"
        "popl %%ebx        \n"
        "popl %%ebp        \n"
        : "+d" (dest)
        : "a" (sys),
        "S" (info),
        "D" (mr1)
        : "%ecx"
    );
}

static inline void
x86_sys_send_null(seL4_Word sys, seL4_Word src, seL4_Word info)
{
    asm volatile (
        "pushl %%ebp       \n"
        "pushl %%ebx       \n"
        "movl %%esp, %%ecx \n"
        "movl %%edx, %%ebx \n"
        "leal 1f, %%edx    \n"
        "1:                \n"
        "sysenter          \n"
        "popl %%ebx        \n"
        "popl %%ebp        \n"
        : "+d" (src)
        : "a" (sys),
        "S" (info)
        : "%ecx"
    );
}

static inline void
x86_sys_recv(seL4_Word sys, seL4_Word src, seL4_Word *out_badge, seL4_Word *out_info, seL4_Word *out_mr1, seL4_Word reply)
{
    asm volatile (
        "pushl %%ebp       \n"
        "pushl %%ebx       \n"
        "movl %%ecx, %%ebp \n"
        "movl %%esp, %%ecx \n"
        "movl %%edx, %%ebx \n"
        "leal 1f, %%edx    \n"
        "1:                \n"
        "sysenter          \n"
        "movl %%ebx, %%edx \n"
        "popl %%ebx        \n"
        "movl %%ebp, %%ecx \n"
        "popl %%ebp        \n"
        :
        "=d" (*out_badge),
        "=S" (*out_info),
        "=D" (*out_mr1),
        "+c" (reply)
        : "a" (sys),
        "d" (src)
        : "memory"
    );
}

static inline void
x86_sys_send_recv(seL4_Word sys, seL4_Word dest, seL4_Word *out_badge, seL4_Word info, seL4_Word *out_info, seL4_Word *in_out_mr1, seL4_Word reply)
{
    asm volatile (
        "pushl %%ebp       \n"
        "pushl %%ebx       \n"
        "movl %%ecx, %%ebp \n"
        "movl %%esp, %%ecx \n"
        "movl %%edx, %%ebx \n"
        "leal 1f, %%edx    \n"
        "1:                \n"
        "sysenter          \n"
        "movl %%ebx, %%edx \n"
        "popl %%ebx        \n"
        "movl %%ebp, %%ecx \n"
        "popl %%ebp        \n"
        :
        "=S" (*out_info),
        "=D" (*in_out_mr1),
        "=d" (*out_badge),
        "+c" (reply)
        : "a" (sys),
        "S" (info),
        "D" (*in_out_mr1),
        "d" (dest)
        : "memory"
    );
}

static inline void
x86_sys_nbsend_wait(seL4_Word sys, seL4_Word src, seL4_Word *out_badge, seL4_Word info, seL4_Word *out_info, seL4_Word *in_out_mr1, seL4_Word reply)
{
    asm volatile(
        "pushl %%ebp       \n"
        "pushl %%ebx       \n"
        "movl %%ecx, %%ebp \n"
        "movl %%esp, %%ecx \n"
        "movl %%edx, %%ebx \n"
        "leal 1f, %%edx    \n"
        "1:                \n"
        "sysenter          \n"
        "movl %%ebx, %%edx \n"
        "popl %%ebx        \n"
        "movl %%ebp, %%ecx \n"
        "popl %%ebp        \n"
        :
        "=S" (*out_info),
        "=D" (*in_out_mr1),
        "=d" (*out_badge),
        "+c" (reply)
        : "a" (sys),
        "S" (info),
        "D" (*in_out_mr1),
        "d" (src)
        : "memory"
    );
}

static inline void
x86_sys_null(seL4_Word sys)
{
    asm volatile (
        "pushl %%ebp       \n"
        "pushl %%ebx       \n"
        "movl %%esp, %%ecx \n"
        "leal 1f, %%edx    \n"
        "1:                \n"
        "sysenter          \n"
        "popl %%ebx        \n"
        "popl %%ebp        \n"
        :
        : "a" (sys)
        : "%ecx", "%edx"
    );
}

#else

static inline void
x86_sys_send(seL4_Word sys, seL4_Word dest, seL4_Word info, seL4_Word mr1)
{
    asm volatile (
        "pushl %%ebp       \n"
        "movl %%esp, %%ecx \n"
        "leal 1f, %%edx    \n"
        "1:                \n"
        "sysenter          \n"
        "popl %%ebp        \n"
        :
        : "a" (sys),
        "b" (dest),
        "S" (info),
        "D" (mr1)
        : "%ecx", "%edx"
    );
}

static inline void
x86_sys_send_null(seL4_Word sys, seL4_Word dest, seL4_Word info)
{
    asm volatile ( \
                   "pushl %%ebp       \n"
                   "movl %%esp, %%ecx \n"
                   "leal 1f, %%edx    \n"
                   "1:                \n"
                   "sysenter          \n"
                   "popl %%ebp        \n"
                   :
                   : "a" (sys),
                   "b" (dest),
                   "S" (info)
                   : "%ecx", "edx"
                 );
}

static inline void
x86_sys_recv(seL4_Word sys, seL4_Word src, seL4_Word *out_badge, seL4_Word *out_info, seL4_Word *out_mr1, seL4_Word reply)
{
    asm volatile ( \
                   "pushl %%ebp       \n"
                   "movl %%ecx, %%ebp \n"
                   "movl %%esp, %%ecx \n"
                   "leal 1f, %%edx    \n"
                   "1:                \n"
                   "sysenter          \n"
                   "movl %%ebp, %%ecx \n"
                   "popl %%ebp        \n"
                   : "=b" (*out_badge),
                   "=S" (*out_info),
                   "=D" (*out_mr1),
                   "+c" (reply)
                   : "a" (sys),
                   "b" (src)
                   : "%edx", "memory"
                 );
}

static inline void
x86_sys_send_recv(seL4_Word sys, seL4_Word dest, seL4_Word *out_badge, seL4_Word info, seL4_Word *out_info, seL4_Word *in_out_mr1, seL4_Word reply)
{
    asm volatile (
        "pushl %%ebp       \n"
        "movl %%ecx, %%ebp \n"
        "movl %%esp, %%ecx \n"
        "leal 1f, %%edx    \n"
        "1:                \n"
        "sysenter          \n"
        "movl %%ebp, %%ecx \n"
        "popl %%ebp        \n"
        : "=S" (*out_info),
        "=D" (*in_out_mr1),
        "=b" (*out_badge),
        "+c" (reply)
        : "a" (sys),
        "S" (info),
        "D" (*in_out_mr1),
        "b" (dest)
        : "%edx", "memory"
    );
}

static inline void
x86_sys_nbsend_wait(seL4_Word sys, seL4_Word src, seL4_Word *out_badge, seL4_Word info, seL4_Word *out_info, seL4_Word *in_out_mr1, seL4_Word reply)
{
    asm volatile (
        "pushl %%ebp       \n"
        "movl %%ecx, %%ebp \n"
        "movl %%esp, %%ecx \n"
        "leal 1f, %%edx    \n"
        "1:                \n"
        "sysenter          \n"
        "movl %%ebp, %%ecx \n"
        "popl %%ebp        \n"
        : "=S" (*out_info),
        "=D" (*in_out_mr1),
        "=b" (*out_badge),
        "+c" (reply)
        : "a" (sys),
        "S" (info),
        "D" (*in_out_mr1),
        "b" (src)
        : "%edx", "memory"
    );
}

static inline void
x86_sys_null(seL4_Word sys)
{
    asm volatile (
        "pushl %%ebp       \n"
        "movl %%esp, %%ecx \n"
        "leal 1f, %%edx    \n"
        "1:                \n"
        "sysenter          \n"
        "popl %%ebp        \n"
        :
        : "a" (sys)
        : "%ebx", "%ecx", "%edx"
    );
}

#endif /* defined(__pic__) */

LIBSEL4_INLINE_FUNC void
seL4_Send(seL4_CPtr dest, seL4_MessageInfo_t msgInfo)
{
    x86_sys_send(seL4_SysSend, dest, msgInfo.words[0], seL4_GetMR(0));
}

LIBSEL4_INLINE_FUNC void
seL4_SendWithMRs(seL4_CPtr dest, seL4_MessageInfo_t msgInfo,
                 seL4_Word *mr0)
{
    x86_sys_send(seL4_SysSend, dest, msgInfo.words[0], mr0 != seL4_Null ? *mr0 : 0);
}

LIBSEL4_INLINE_FUNC void
seL4_NBSend(seL4_CPtr dest, seL4_MessageInfo_t msgInfo)
{
    x86_sys_send(seL4_SysNBSend, dest, msgInfo.words[0], seL4_GetMR(0));
}

LIBSEL4_INLINE_FUNC void
seL4_NBSendWithMRs(seL4_CPtr dest, seL4_MessageInfo_t msgInfo,
                   seL4_Word *mr0)
{
    x86_sys_send(seL4_SysNBSend, dest, msgInfo.words[0], mr0 != seL4_Null ? *mr0 : 0);
}

LIBSEL4_INLINE_FUNC void
seL4_Signal(seL4_CPtr dest)
{
    x86_sys_send_null(seL4_SysSend, dest, seL4_MessageInfo_new(0, 0, 0, 0).words[0]);
}

LIBSEL4_INLINE_FUNC seL4_MessageInfo_t
seL4_Recv(seL4_CPtr src, seL4_Word* sender, seL4_CPtr reply)
{
    seL4_MessageInfo_t info;
    seL4_Word badge;
    seL4_Word mr0;

    x86_sys_recv(seL4_SysRecv, src, &badge, &info.words[0], &mr0, reply);

    seL4_SetMR(0, mr0);

    if (sender) {
        *sender = badge;
    }

    return info;
}

LIBSEL4_INLINE_FUNC seL4_MessageInfo_t
seL4_RecvWithMRs(seL4_CPtr src, seL4_Word* sender,
                 seL4_Word *mr0, seL4_CPtr reply)
{
    seL4_MessageInfo_t info;
    seL4_Word badge;
    seL4_Word msg0 = 0;

    x86_sys_recv(seL4_SysRecv, src, &badge, &info.words[0], &msg0, reply);

    if (mr0 != seL4_Null) {
        *mr0 = msg0;
    }

    if (sender) {
        *sender = badge;
    }

    return info;
}

LIBSEL4_INLINE_FUNC seL4_MessageInfo_t
seL4_NBRecv(seL4_CPtr src, seL4_Word* sender, seL4_CPtr reply)
{
    seL4_MessageInfo_t info;
    seL4_Word badge;
    seL4_Word mr0;

    x86_sys_recv(seL4_SysRecv, src, &badge, &info.words[0], &mr0, reply);

    seL4_SetMR(0, mr0);

    if (sender) {
        *sender = badge;
    }

    return info;
}

LIBSEL4_INLINE_FUNC seL4_MessageInfo_t
seL4_Wait(seL4_CPtr src, seL4_Word* sender)
{
    seL4_MessageInfo_t info;
    seL4_Word badge;
    seL4_Word mr0;

    x86_sys_recv(seL4_SysWait, src, &badge, &info.words[0], &mr0, 0);

    seL4_SetMR(0, mr0);

    if (sender) {
        *sender = badge;
    }

    return info;
}

LIBSEL4_INLINE_FUNC seL4_MessageInfo_t
seL4_WaitWithMRs(seL4_CPtr src, seL4_Word* sender,
                 seL4_Word *mr0)
{
    seL4_MessageInfo_t info;
    seL4_Word badge;
    seL4_Word msg0 = 0;

    x86_sys_recv(seL4_SysWait, src, &badge, &info.words[0], &msg0, 0);

    if (mr0 != seL4_Null) {
        *mr0 = msg0;
    }

    if (sender) {
        *sender = badge;
    }

    return info;
}

LIBSEL4_INLINE_FUNC seL4_MessageInfo_t
seL4_NBWait(seL4_CPtr src, seL4_Word* sender)
{
    seL4_MessageInfo_t info;
    seL4_Word badge;
    seL4_Word mr0;

    x86_sys_recv(seL4_SysNBWait, src, &badge, &info.words[0], &mr0, 0);

    seL4_SetMR(0, mr0);

    if (sender) {
        *sender = badge;
    }

    return info;
}

LIBSEL4_INLINE_FUNC seL4_MessageInfo_t
seL4_Call(seL4_CPtr dest, seL4_MessageInfo_t msgInfo)
{
    seL4_MessageInfo_t info;
    seL4_Word mr0 = seL4_GetMR(0);

    x86_sys_send_recv(seL4_SysCall, dest, &dest, msgInfo.words[0], &info.words[0], &mr0, 0);

    seL4_SetMR(0, mr0);

    return info;
}

LIBSEL4_INLINE_FUNC seL4_MessageInfo_t
seL4_CallWithMRs(seL4_CPtr dest, seL4_MessageInfo_t msgInfo,
                 seL4_Word *mr0)
{
    seL4_MessageInfo_t info;
    seL4_Word msg0 = 0;

    if (mr0 != seL4_Null && seL4_MessageInfo_get_length(msgInfo) > 0) {
        msg0 = *mr0;
    }

    x86_sys_send_recv(seL4_SysCall, dest, &dest, msgInfo.words[0], &info.words[0], &msg0, 0);

    if (mr0 != seL4_Null) {
        *mr0 = msg0;
    }

    return info;
}

LIBSEL4_INLINE_FUNC seL4_MessageInfo_t
seL4_ReplyRecv(seL4_CPtr dest, seL4_MessageInfo_t msgInfo, seL4_Word *sender, seL4_CPtr reply)
{
    seL4_MessageInfo_t info;
    seL4_Word badge;
    seL4_Word mr0 = seL4_GetMR(0);

    x86_sys_send_recv(seL4_SysReplyRecv, dest, &badge, msgInfo.words[0], &info.words[0], &mr0, reply);

    seL4_SetMR(0, mr0);

    if (sender) {
        *sender = badge;
    }

    return info;
}

LIBSEL4_INLINE_FUNC seL4_MessageInfo_t
seL4_ReplyRecvWithMRs(seL4_CPtr dest, seL4_MessageInfo_t msgInfo, seL4_Word *sender,
                      seL4_Word *mr0, seL4_CPtr reply)
{
    seL4_MessageInfo_t info;
    seL4_Word badge;
    seL4_Word msg0 = 0;

    if (mr0 != seL4_Null && seL4_MessageInfo_get_length(msgInfo) > 0) {
        msg0 = *mr0;
    }

    x86_sys_send_recv(seL4_SysReplyRecv, dest, &badge, msgInfo.words[0], &info.words[0], &msg0, reply);

    if (mr0 != seL4_Null) {
        *mr0 = msg0;
    }

    if (sender) {
        *sender = badge;
    }

    return info;
}

LIBSEL4_INLINE_FUNC seL4_MessageInfo_t
seL4_NBSendRecv(seL4_CPtr dest, seL4_MessageInfo_t msgInfo, seL4_Word src, seL4_Word *sender, seL4_CPtr reply)
{
    seL4_MessageInfo_t info;
    seL4_Word badge;
    seL4_Word mr0 = seL4_GetMR(0);

    /* no spare registers on ia32 -> must use ipc buffer */
    seL4_SetReserved(dest);

    x86_sys_nbsend_wait(seL4_SysNBSendRecv, src, &badge, msgInfo.words[0], &info.words[0], &mr0, reply);

    seL4_SetMR(0, mr0);

    if (sender) {
        *sender = badge;
    }

    return info;
}

LIBSEL4_INLINE_FUNC seL4_MessageInfo_t
seL4_NBSendRecvWithMRs(seL4_CPtr dest, seL4_MessageInfo_t msgInfo, seL4_Word src, seL4_Word *sender,
                      seL4_Word *mr0, seL4_CPtr reply)
{
    seL4_MessageInfo_t info;
    seL4_Word badge;
    seL4_Word msg0 = 0;

    /* no spare registers on ia32 -> must use ipc buffer */
    seL4_SetReserved(dest);

    if (mr0 != seL4_Null && seL4_MessageInfo_get_length(msgInfo) > 0) {
        msg0 = *mr0;
    }

    x86_sys_nbsend_wait(seL4_SysNBSendRecv, src, &badge, msgInfo.words[0], &info.words[0], &msg0, reply);

    if (mr0 != seL4_Null) {
        *mr0 = msg0;
    }

    if (sender) {
        *sender = badge;
    }

    return info;
}

LIBSEL4_INLINE_FUNC seL4_MessageInfo_t
seL4_NBSendWait(seL4_CPtr dest, seL4_MessageInfo_t msgInfo, seL4_Word src, seL4_Word *sender)
{
    seL4_MessageInfo_t info;
    seL4_Word badge;
    seL4_Word mr0 = seL4_GetMR(0);

    x86_sys_nbsend_wait(seL4_SysNBSendWait, src, &badge, msgInfo.words[0], &info.words[0], &mr0, dest);

    seL4_SetMR(0, mr0);

    if (sender) {
        *sender = badge;
    }

    return info;
}

LIBSEL4_INLINE_FUNC seL4_MessageInfo_t
seL4_NBSendWaitWithMRs(seL4_CPtr dest, seL4_MessageInfo_t msgInfo, seL4_Word src, seL4_Word *sender,
                      seL4_Word *mr0)
{
    seL4_MessageInfo_t info;
    seL4_Word badge;
    seL4_Word msg0 = 0;

    if (mr0 != seL4_Null && seL4_MessageInfo_get_length(msgInfo) > 0) {
        msg0 = *mr0;
    }

    x86_sys_nbsend_wait(seL4_SysReplyRecv, src, &badge, msgInfo.words[0], &info.words[0], &msg0, dest);

    if (mr0 != seL4_Null) {
        *mr0 = msg0;
    }

    if (sender) {
        *sender = badge;
    }

    return info;
}

static inline void
seL4_Yield(void)
{
    x86_sys_null(seL4_SysYield);
    asm volatile("" :::"%esi", "%edi", "memory");
}

#ifdef CONFIG_VTX
LIBSEL4_INLINE_FUNC seL4_Word
seL4_VMEnter(seL4_CPtr vcpu, seL4_Word *sender)
{
    seL4_Word fault;
    seL4_Word badge;
    seL4_Word mr0 = seL4_GetMR(0);

    x86_sys_send_recv(seL4_SysVMEnter, vcpu, &badge, 0, &fault, &mr0, 0);

    seL4_SetMR(0, mr0);
    seL4_SetMR(1, mr1);
    if (!fault && sender) {
        *sender = badge;
    }
    return fault;
}
#endif

#if defined(CONFIG_DEBUG_BUILD)
LIBSEL4_INLINE_FUNC void
seL4_DebugPutChar(char c)
{
    seL4_Word unused0 = 0;
    seL4_Word unused1 = 0;
    seL4_Word unused2 = 0;

    x86_sys_send_recv(seL4_SysDebugPutChar, c, &unused0, 0, &unused1, &unused2, 0);
}
#endif

#ifdef CONFIG_DEBUG_BUILD
LIBSEL4_INLINE_FUNC void
seL4_DebugHalt(void)
{
    x86_sys_null(seL4_SysDebugHalt);
    asm volatile("" :::"%esi", "%edi", "memory");
}
#endif

#if defined(CONFIG_DEBUG_BUILD)
LIBSEL4_INLINE_FUNC void
seL4_DebugSnapshot(void)
{
    x86_sys_null(seL4_SysDebugSnapshot);
    asm volatile("" :::"%esi", "%edi", "memory");
}
#endif

#ifdef CONFIG_DEBUG_BUILD
LIBSEL4_INLINE_FUNC seL4_Uint32
seL4_DebugCapIdentify(seL4_CPtr cap)
{
    seL4_Word unused0 = 0;
    seL4_Word unused1 = 0;

    x86_sys_send_recv(seL4_SysDebugCapIdentify, cap, &cap, 0, &unused0, &unused1, 0);
    return (seL4_Uint32)cap;
}

char *strcpy(char *, const char *);
LIBSEL4_INLINE_FUNC void
seL4_DebugNameThread(seL4_CPtr tcb, const char *name)
{
    strcpy((char*)seL4_GetIPCBuffer()->msg, name);

    seL4_Word unused0 = 0;
    seL4_Word unused1 = 0;
    seL4_Word unused2 = 0;

    x86_sys_send_recv(seL4_SysDebugNameThread, tcb, &unused0, 0, &unused1, &unused2, 0);
}
#endif

#if defined(CONFIG_DANGEROUS_CODE_INJECTION)
LIBSEL4_INLINE_FUNC void
seL4_DebugRun(void (*userfn) (void *), void* userarg)
{
    x86_sys_send_null(seL4_SysDebugRun, (seL4_Word)userfn, (seL4_Word)userarg);
    asm volatile("" ::: "%edi", "memory");
}
#endif

#ifdef CONFIG_ENABLE_BENCHMARKS
LIBSEL4_INLINE_FUNC seL4_Error
seL4_BenchmarkResetLog(void)
{
    seL4_Word unused0 = 0;
    seL4_Word unused1 = 0;

    seL4_Word ret;

    x86_sys_send_recv(seL4_SysBenchmarkResetLog, 0, &ret, 0, &unused0, &unused1, 0);

    return (seL4_Error)ret;
}

LIBSEL4_INLINE_FUNC seL4_Word
seL4_BenchmarkFinalizeLog(void)
{
    seL4_Word unused0 = 0;
    seL4_Word unused1 = 0;
    seL4_Word unused2 = 0;
    seL4_Word index_ret;
    x86_sys_send_recv(seL4_SysBenchmarkFinalizeLog, 0, &index_ret, 0, &unused0, &unused1, &unused2);

    return (seL4_Word)index_ret;
}

LIBSEL4_INLINE_FUNC seL4_Error
seL4_BenchmarkSetLogBuffer(seL4_Word frame_cptr)
{
    seL4_Word unused0 = 0;
    seL4_Word unused1 = 0;

    x86_sys_send_recv(seL4_SysBenchmarkSetLogBuffer, frame_cptr, &frame_cptr, 0, &unused0, &unused1, 0);

    return (seL4_Error) frame_cptr;
}


LIBSEL4_INLINE_FUNC void
seL4_BenchmarkNullSyscall(void)
{
    x86_sys_null(seL4_SysBenchmarkNullSyscall);
    asm volatile("" :::"%esi", "%edi", "memory");
}

LIBSEL4_INLINE_FUNC void
seL4_BenchmarkFlushCaches(void)
{
    x86_sys_null(seL4_SysBenchmarkFlushCaches);
    asm volatile("" :::"%esi", "%edi", "memory");
}

#ifdef CONFIG_BENCHMARK_TRACK_UTILISATION
LIBSEL4_INLINE_FUNC void
seL4_BenchmarkGetThreadUtilisation(seL4_Word tcb_cptr)
{
    seL4_Word unused0 = 0;
    seL4_Word unused1 = 0;
    seL4_Word unused2 = 0;

    x86_sys_send_recv(seL4_SysBenchmarkGetThreadUtilisation, tcb_cptr, &unused0, 0, &unused1, &unused2, 0);
}

LIBSEL4_INLINE_FUNC void
seL4_BenchmarkResetThreadUtilisation(seL4_Word tcb_cptr)
{
    seL4_Word unused0 = 0;
    seL4_Word unused1 = 0;
    seL4_Word unused2 = 0;
    seL4_Word unused3 = 0;

    x86_sys_send_recv(seL4_SysBenchmarkResetThreadUtilisation, tcb_cptr, &unused0, 0, &unused1, &unused2, 0);
}
#endif /* CONFIG_BENCHMARK_TRACK_UTILISATION */
#endif /* CONFIG_ENABLE_BENCHMARKS */

#endif
