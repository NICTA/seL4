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
Checksums of the kernel binary.
*/
#include <selftest/crc32.h>
#include <selftest/checksum.h>

#include <object/structures.h>

/*
Utils
TODO: some of these may already be provided elsewhere in the kernel
*/
static char const hex_digit[] = "0123456789abcdef";
static void showBufHex(byte_t const *buf, word_t n, char *str) {
  word_t i;
  for(i = 0; i < n; ++i) {
    str[2*i]   = hex_digit[buf[i] >> 4];
    str[2*i+1] = hex_digit[buf[i] & 0xf];
  }

  str[2*i] = '\0';
}

static int memequal(byte_t const *a, byte_t const *b, word_t len) {
  word_t i;
  for(i = 0; i < len; ++i) {
    if(a[i] != b[i]) return 0;
  }
  return 1;
}

/* generic CRC32 testing */
static int testCRC32With(char const *name,
                         byte_t const *start, byte_t const *end,
                         byte_t const checksum[4]) {
  byte_t result[4];
  crc(start, end - start, result);
  if(!memequal(result, checksum, 4)) {
    char buf1[9], buf2[9];
    showBufHex(result, 4, buf1);
    showBufHex(checksum, 4, buf2);
    userError("%s checksum mismatch: got %s; expected value is %s",
              name, buf1, buf2);
    return 0;
  }
  return 1;
}



/* Checksums of .text and .rodata. */

/* The linker points these variables to the correct places; see linker.lds */
extern byte_t text_start, text_end;
extern byte_t rodata_start, rodata_end;

/*
 * Sections to store the checksums.
 * At build time, the crc32_embed.sh script stores the actual
 * CRC values into these arrays.
*/
SECTION(".crc32.text") byte_t const text_crc32_checksum[4];
SECTION(".crc32.rodata") byte_t const rodata_crc32_checksum[4];

int selftestKernelChecksum(void) {
  /* userError(".text: %x .. %x", (word_t)&text_start, (word_t)&text_end); */
  if (!testCRC32With("text_crc32", &text_start, &text_end, text_crc32_checksum)) {
    return 0;
  }

  /* userError(".rodata: %x .. %x", (word_t)&rodata_start, (word_t)&rodata_end); */
  if (!testCRC32With("rodata_crc32", &rodata_start, &rodata_end, rodata_crc32_checksum)) {
    return 0;
  }

  return 1;
}


/* Checksum of kernel objects */
word_t selftestChecksum(cap_t cap) {
    void *data = cap_get_capPtr(cap);
    word_t size = cap_get_capSizeBits(cap);
    unsigned char type = cap_get_capType(cap);
    /* Also checksum the cap type */
    word_t sum = update_crc(init_crc(), &type, 1);

    /* userError("checksum: cap %u, ptr %x, size %x", cap_get_capType(cap), (word_t)data, size); */

    if (!data || !size) {
	sum = finish_crc(sum);
    } else {
	sum = finish_crc(update_crc(sum, data, BIT(size)));
    }
    /* d202ef8d is the null cap checksum, this xor makes it easy to spot null caps
     * when debugging.
     * FIXME: remove if no longer needed */
    return sum ^ 0xd202ef8d; 
}
