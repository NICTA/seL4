/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef __LIBSEL4_TYPES_H
#define __LIBSEL4_TYPES_H

#ifdef HAVE_AUTOCONF
#include <autoconf.h>
#endif
#include <sel4/simple_types.h>
#include <sel4/macros.h>
#include <sel4/arch/types.h>
#include <sel4/sel4_arch/types.h>
#include <sel4/sel4_arch/types_gen.h>
#include <sel4/types_gen.h>
#include <sel4/syscall.h>
#include <sel4/objecttype.h>
#include <sel4/sel4_arch/objecttype.h>
#include <sel4/arch/objecttype.h>
#include <sel4/errors.h>
#include <sel4/constants.h>
#include <sel4/shared_types_gen.h>
#include <sel4/shared_types.h>

#define seL4_UntypedRetypeMaxObjects 256
#define seL4_GuardSizeBits 5
#define seL4_GuardBits 18
#define seL4_BadgeBits 28

typedef seL4_Uint64 seL4_Time;
typedef seL4_CPtr seL4_CNode;
typedef seL4_CPtr seL4_IRQHandler;
typedef seL4_CPtr seL4_IRQControl;
typedef seL4_CPtr seL4_TCB;
typedef seL4_CPtr seL4_Untyped;
typedef seL4_CPtr seL4_DomainSet;
typedef seL4_CPtr seL4_SchedContext;
typedef seL4_CPtr seL4_SchedControl;

#define seL4_NilData seL4_CapData_Badge_new(0)

#include <sel4/arch/constants.h>

#endif
