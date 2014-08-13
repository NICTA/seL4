/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */
/*
 * CRC32 checksum.
 *
 * This file is special in that it is built on both host and target archs.
 * It is used on the host to generate the checksum at build time.
 * Therefore it must remain portable.
 *
 * Use HOST_CRC32 to define substitutes for building on the host platform.
 */

#ifndef __SELFTEST_CRC32_H
#define __SELFTEST_CRC32_H

#ifdef HOST_CRC32
  #include <stdint.h>
  typedef uint32_t word_t;
#else
  #include <types.h>
#endif

/* using this a lot */
typedef unsigned char byte_t;

/* incremental calculation */
word_t init_crc(void);
word_t update_crc(word_t crc, byte_t const* data, word_t len);
word_t finish_crc(word_t crc);
void store_crc(word_t crc, byte_t out[4]);

/* wrapper */
void crc(byte_t const *data, word_t len, byte_t out[4]);

#endif
