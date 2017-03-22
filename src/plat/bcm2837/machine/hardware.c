/*
 * Copyright 2016, CSIRO Data61
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(DATA61_GPL)
 */


#include <config.h>
#include <types.h>
#include <arch/machine.h>
#include <mode/machine/timer.h>
#include <arch/linker.h>

BOOT_CODE void
initTimer(void)
{
    initGenericTimer();
}
