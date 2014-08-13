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
Calculate CRC32 checksum of input and print the checksum as raw bytes.
This is run during the build process to calculate the kernel checksum.
*/
#include <stdio.h>
#include <selftest/crc32.h>

int main(void) {
  word_t crc = init_crc();
  int c, i;
  unsigned char in, out[4];

  while((c = getchar()) != EOF) {
    in = c;
    crc = update_crc(crc, &in, 1);
  }
  store_crc(finish_crc(crc), out);

  for(i = 0; i < 4; ++i) {
    putchar(out[i]);
  }

  return 0;
}
