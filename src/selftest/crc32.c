/*
CRC32 checksum.

This file is special in that it is built for both host and target archs.
It is used on the host to generate the checksum at build time.
Therefore it must remain portable.

Use HOST_CRC32 to define substitutes for building on the host platform.
Conversely, you can put seL4 specific code in #ifndef HOST_CRC32.
*/

#include <selftest/crc32.h>

/*
This CRC algorithm is used in many places.
The implementation is adapted from the PNG 1.2 specification (15 Appendix).

MIT License:
Permission to use, copy, and distribute this specification for any purpose and
without fee or royalty is hereby granted, provided that the full text of this
NOTICE appears on ALL copies of the specification or portions thereof,
including modifications, that you make.

THIS SPECIFICATION IS PROVIDED "AS IS," AND COPYRIGHT HOLDERS MAKE NO
REPRESENTATIONS OR WARRANTIES, EXPRESS OR IMPLIED. BY WAY OF EXAMPLE, BUT NOT
LIMITATION, COPYRIGHT HOLDERS MAKE NO REPRESENTATIONS OR WARRANTIES OF
MERCHANTABILITY OR FITNESS FOR ANY PARTICULAR PURPOSE OR THAT THE USE OF THE
SPECIFICATION WILL NOT INFRINGE ANY THIRD PARTY PATENTS, COPYRIGHTS,
TRADEMARKS OR OTHER RIGHTS. COPYRIGHT HOLDERS WILL BEAR NO LIABILITY FOR ANY
USE OF THIS SPECIFICATION.

The name and trademarks of copyright holders may NOT be used in advertising or
publicity pertaining to the specification without specific, written prior
permission. Title to copyright in this specification and any associated
documentation will at all times remain with copyright holders.
*/

/* Table of CRCs of all 8-bit messages. */
static word_t crc_table[256];

/* Make the table for a fast CRC. */
static void make_crc_table(void)
{
  word_t c;
  int n, k;

  /* Flag: has the table been computed? Initially false. */
  static int crc_table_computed = 0;

  if(crc_table_computed) {
    return;
  }

  for (n = 0; n < 256; n++) {
    c = (word_t) n;
    for (k = 0; k < 8; k++) {
      if (c & 1)
        c = 0xedb88320UL ^ (c >> 1);
      else
        c = c >> 1;
    }
    crc_table[n] = c;
  }
  crc_table_computed = 1;
}

word_t init_crc(void) {
  return ~(word_t)0;
}

/* Update a running CRC with the bytes buf[0..len-1]--the CRC
   should be initialized to all 1's, and the transmitted value
   is the 1's complement of the final running CRC (see the
   finish_crc() routine below). */

word_t update_crc(word_t crc, byte_t const *data, word_t len)
{
  word_t c = crc;
  word_t n;

  make_crc_table();
  for (n = 0; n < len; n++) {
    c = crc_table[(c ^ data[n]) & 0xff] ^ (c >> 8);
  }
  return c;
}

word_t finish_crc(word_t crc)
{
  return ~crc;
}

void store_crc(word_t crc, byte_t out[4]) {
  int i;
  for(i = 0; i < 4; ++i) {
    out[3-i] = crc & 0xff;
    crc >>= 8;
  }
}

/* Compute CRC and store as big-endian */
void crc(byte_t const *data, word_t len, byte_t out[4]) {
  store_crc(finish_crc(update_crc(init_crc(), data, len)), out);
}
