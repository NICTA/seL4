/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

#ifndef __SELFTEST_CHECKSUM_H
#define __SELFTEST_CHECKSUM_H

#include <types.h>

/* Returns 0 if the checksum failed to match. */
int selftestKernelChecksum(void);

/* Checksums the object pointed to by the given cap. */
word_t selftestChecksum(cap_t cap);

#endif
