#!/bin/bash
#
# Copyright 2014, NICTA
#
# This software may be distributed and modified according to the terms of
# the GNU General Public License version 2. Note that NO WARRANTY is provided.
# See "LICENSE_GPLv2.txt" for details.
#
# @TAG(NICTA_GPL)
#
# 
# Embed a CRC32 checksum into the kernel ELF binary.
# The binary must already have a .crc32.FOO section for the checksum of .FOO.
#

if [ $# -ne 2 ]; then
  echo "Usage: $0 kernel.elf .section-name" >2
  exit 1
fi

# FIXME: these aren't inherited from the makefile for some reason
READELF=${TOOLPREFIX}readelf
OBJCOPY=${TOOLPREFIX}objcopy

KERNEL="$1"
SECTION="$2"
SECTION_CRC=".crc32${SECTION}"
SECTION_DUMP=${KERNEL}${SECTION}

# build CRC32 program
: ${HOSTCC:=cc} $ use host cc
# we need crc32.h, but don't want to pull in all of kernel/include/
# (which contains oddly named files like "stdint.h" and "stdarg.h")
TEMPDIR=`mktemp -d`
mkdir ${TEMPDIR}/selftest && cp ${SOURCE_ROOT}/include/selftest/crc32.h ${TEMPDIR}/selftest/ &&
$HOSTCC -I${TEMPDIR} -DHOST_CRC32 ${SOURCE_ROOT}/src/selftest/crc32.c ${SOURCE_ROOT}/tools/crc32.c -o crc32 &&
{
  # for some reason, objcopy doesn't fail if the section doesn't exist
  if ! ${READELF} -S $KERNEL | grep " ${SECTION} " > /dev/null; then
    echo "$0: error: could not find section {SECTION} in ${KERNEL}" >&2
    rm -r $TEMPDIR
    exit 1
  fi
  # get the contents of $SECTION
  ${OBJCOPY} -j $SECTION -O binary $KERNEL $SECTION_DUMP
} &&
{
  # get position of the CRC section
  CRC_PLACE=$(
    ${READELF} -S $KERNEL |  # ... using readelf
    grep " ${SECTION_CRC} " | # ... and get just the CRC section
    sed 's/ \+/ /g' |        # ... and cut out its offset
    sed 's/^.*\] //' | 
    cut '-d ' -f4 | 
    perl -e 'print hex(<>)') # ... and convert the offset to decimal
  if [ $CRC_PLACE = 0 ]; then
    echo "$0: error: could not find section ${SECTION_CRC} in ${KERNEL}" >&2
    rm $SECTION_DUMP
    rm -r $TEMPDIR
    exit 1
  fi
} &&
# calculate CRC
./crc32 < $SECTION_DUMP |
  # write into binary at CRC_PLACE
  dd of=$KERNEL bs=1 seek=$CRC_PLACE conv=notrunc status=noxfer

CODE=$?

# cleanup
rm $SECTION_DUMP
rm -r $TEMPDIR

exit $CODE
