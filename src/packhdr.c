/*
 * Pack an .exe header to remove extra data.
 *
 * This makes it look the same as when it has been packed and unpacked, if the
 * packer disregarded the extra data as PKLite does with the -e option.
 *
 * Copyright (C) 2010-2021 Adam Nielsen <malvineous@shikadi.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <io.h>

#define STDIN  0
#define STDOUT 1
#define STDERR 2

#define O_BINARY 0x8000

typedef unsigned short uint16_t;
typedef unsigned long uint32_t;

typedef struct {
	uint16_t sig;
	uint16_t lenLastBlock;
	uint16_t blockCount;
	uint16_t relocCount;
	uint16_t pgLenHeader;
	uint16_t pgMinExtra;
	uint16_t pgMaxExtra;
	uint16_t segSS;
	uint16_t regSP;
	uint16_t checksum;
	uint16_t regIP;
	uint16_t segCS;
	uint16_t offRelocTable;
	uint16_t overlayIndex;
} EXE_HEADER;

unsigned char buf[1024];

void copybuf(unsigned int len, int discard)
{
	unsigned int n;

	while (len > 0) {
		n = len;
		if (n > sizeof(buf)) n = sizeof(buf);
		n = read(STDIN, buf, n);
		if (discard == 0) write(STDOUT, buf, n);
		len -= n;
	}
}

#define copy(a) copybuf(a, 0)
#define discard(a) copybuf(a, 1);

void printint(unsigned int n)
{
	buf[0] = 0x30 + (n / 10000) % 10;
	buf[1] = 0x30 + (n / 1000) % 10;
	buf[2] = 0x30 + (n / 100) % 10;
	buf[3] = 0x30 + (n / 10) % 10;
	buf[4] = 0x30 + (n % 10);
	buf[5] = '\n';
	write(STDERR, buf, 6);
}

int parseInt(const char *s)
{
	unsigned int out = 0;

	for (; *s; s++) {
		out <<= 4;
		if ((*s >= '0') && (*s <= '9')) {
			out += *s - '0';
		} else if ((*s >= 'A') && (*s <= 'F')) {
			out += 10 + *s - 'A';
		} else if ((*s >= 'a') && (*s <= 'f')) {
			out += 10 + *s - 'a';
		} else {
			return -1;
		}
	}

	return out;
}

int main(int argc, const char *argv[])
{
	EXE_HEADER hdr;
	int n, hdrExtra, lenReloc, lenRelocDrop, lenRelocAdd, lenOutputHeader;
	int addSigPKL = 0;
	uint32_t reloc;

	lenOutputHeader = sizeof(EXE_HEADER);

	/* Use -e parameter to add extra PKLite signature */
	if (argc > 2) {
		if (
			(argv[1][0] == '-')
			&& (argv[1][1] == 'e')
			&& (argv[1][2] == 0)
		) {
			addSigPKL = parseInt(argv[2]);
			lenOutputHeader += 2;
		}
	}
	if (addSigPKL < 0) {
		const char errmsg[] = "Bad signature\n";
		write(STDERR, errmsg, sizeof(errmsg));
		return 2;
	}

	/* Don't translate CRLF */
	setmode(STDIN, O_BINARY);
	setmode(STDOUT, O_BINARY);

	/* Read the input EXE header */
	n = read(STDIN, &hdr, sizeof(EXE_HEADER));
	if (n < sizeof(EXE_HEADER)) {
		const char errmsg[] = "Short read\n";
		write(STDERR, errmsg, sizeof(errmsg));
		return 1;
	}

	if (hdr.sig != 0x5A4D) {
		const char errmsg[] = "Not an EXE\n";
		write(STDERR, errmsg, sizeof(errmsg));
		return 1;
	}

	/* Recalculate where things should go */
	hdrExtra = hdr.offRelocTable - sizeof(EXE_HEADER);
	lenReloc = hdr.relocCount * 4;
	lenRelocDrop = (hdr.pgLenHeader << 4) - (hdr.offRelocTable + lenReloc);
	if (lenRelocDrop < 0) lenRelocDrop = 0;

	hdr.offRelocTable = lenOutputHeader;
	hdr.pgLenHeader = (hdr.offRelocTable + lenReloc + 0xF) >> 4;
	lenRelocAdd = (hdr.pgLenHeader << 4) - (hdr.offRelocTable + lenReloc);

	/* Update the total .exe size */
	reloc = (hdr.blockCount << 9) + hdr.lenLastBlock;
	if (hdr.lenLastBlock) reloc -= 512;
	reloc = reloc - hdrExtra - lenRelocDrop + lenRelocAdd
		+ (lenOutputHeader - sizeof(EXE_HEADER));
	hdr.lenLastBlock = reloc & 0x1FF;
	hdr.blockCount = (reloc + 0x1FF) >> 9;

	/* Write updated EXE header */
	write(STDOUT, &hdr, sizeof(EXE_HEADER));
	if (addSigPKL) {
		write(STDOUT, &addSigPKL, 2);
	}

	/* Discard extra header bytes */
	discard(hdrExtra);

	/* Copy and normalise relocation table.  This doesn't completely match PKLite
	   as it doesn't adjust the segment when the absolute address goes beyond
	   0x10000, but our test files aren't large enough for that. */
	for (n = 0; n < hdr.relocCount; n++) {
		read(STDIN, &reloc, 4);

		/* Convert to absolute address */
		reloc = ((reloc >> 12) & 0xFFFF0) + (reloc & 0xFFFF);
		/* Convert back to seg/off */
		reloc = ((reloc & 0xF0000) << 16) | (reloc & 0xFFFF);

		write(STDOUT, &reloc, 4);
	}

	/* Pad relocation table up to paragraph boundary */
	for (n = 0; n < lenRelocAdd; n++) buf[n] = 0;
	write(STDOUT, buf, lenRelocAdd);

	/* Discard extra bytes after reloc table */
	discard(lenRelocDrop);

	/* Copy rest of EXE */
	while ((n = read(STDIN, buf, sizeof(buf))) > 0) {
		write(STDOUT, buf, n);
	}

	return 0;
}
