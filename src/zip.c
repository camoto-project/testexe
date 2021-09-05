/*
 * Minimal PK-ZIP compatible I/O.
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

#include <stdio.h>
#include <string.h>

#include "debug.h"
#include "types.h"
#include "zip.h"

typedef struct {
	uint16_t verExtract;
	uint16_t flags;
	uint16_t compression;
	uint16_t lastModTime;
	uint16_t lastModDate;
	uint32_t crc;
	uint32_t lenCompressed;
	uint32_t lenUncompressed;
	uint16_t lenFilename;
	uint16_t lenExtra;
} ZIP_LDIR_ENTRY;

typedef struct {
	uint16_t verMade;
	uint16_t verExtract;
	uint16_t flags;
	uint16_t compression;
	uint16_t lastModTime;
	uint16_t lastModDate;
	uint32_t crc;
	uint32_t lenCompressed;
	uint32_t lenUncompressed;
	uint16_t lenFilename;
	uint16_t lenExtra;
	uint16_t lenComment;
	uint16_t idxFirstDisk;
	uint16_t attrInternal;
	uint32_t attrExternal;
	uint32_t offLocal;
} ZIP_CDIR_ENTRY;

typedef struct {
	uint16_t diskIndex;
	uint16_t diskDirStart;
	uint16_t diskEntryCount;
	uint16_t totalEntryCount;
	uint32_t lenDirectory;
	uint32_t offDirectory;
	uint16_t lenComment;
} ZIP_CDIR_END;

#define ZIP_SIG_LDIR     0x04034B50
#define ZIP_SIG_CDIR     0x02014B50
#define ZIP_SIG_CDIR_END 0x06054B50

/* Find the zip central directory */
int zipPopulateDirOffset(ZIP *zip)
{
	size_t len;
	uint32_t sig;
	uint32_t endOffset;
	ZIP_CDIR_END zipDirEnd;

	debug("zip: Searching for central directory\n");

	endOffset = 22;
	fseek(zip->fHandle, 22 + 5, SEEK_END);
	sig = 0;
	while ((sig != ZIP_SIG_CDIR_END) && (endOffset < 65536 + 22)) {
		fseek(zip->fHandle, -5, SEEK_CUR);
		endOffset++;
		fread(&sig, 4, 1, zip->fHandle);
	}
	if (sig != ZIP_SIG_CDIR_END) {
		return ZIP_ERR_NO_DIR;
	}
	debug("zip: cdir_end at offset 0x%lX\n", ftell(zip->fHandle));

	len = fread(&zipDirEnd, 1, sizeof(zipDirEnd), zip->fHandle);
	if (len < sizeof(zipDirEnd)) {
		debug("zip: short read on cdir, expected %u, got %u\n", sizeof(zipDirEnd), len);
		return ZIP_ERR_IO;
	}
	debug("zip: cdir at offset 0x%lX\n", zipDirEnd.offDirectory);

	zip->offDir = zipDirEnd.offDirectory;
	zip->fileCount = zipDirEnd.totalEntryCount;

	return ZIP_OK;
}

int zipOpen(ZIP *zip, FILE *fZipFile)
{
	zip->fHandle = fZipFile;
	return zipPopulateDirOffset(zip);
}

int zipSeekFile(const ZIP *zip, const char *filename, uint32_t *lenContent)
{
	ZIP_CDIR_ENTRY zipCDir;
	ZIP_LDIR_ENTRY zipLDir;
	char entryFilename[256];
	int i;
	size_t len;
	uint32_t sig = 0;

	debug("zip: find: %s in %d files\n", filename, zip->fileCount);

	/* Jump to the start of the central directory.  The offset takes into account
	   the size of the .exe code preceding the .zip data. */
	fseek(zip->fHandle, zip->offDir, SEEK_SET);

	for (i = 0; i < zip->fileCount; i++) {

		/* Read the signature to confirm we're at the right place. */
		fread(&sig, 4, 1, zip->fHandle);
		if (sig != ZIP_SIG_CDIR) {
			return ZIP_ERR_NO_DIR;
		}

		len = fread(&zipCDir, 1, sizeof(zipCDir), zip->fHandle);
		if (len < sizeof(zipCDir)) {
			debug("zip: short read on file %d cdir (got %u, exp %u).\n", i, len, sizeof(zipCDir));
			return ZIP_ERR_IO;
		}
		fread(entryFilename, zipCDir.lenFilename, 1, zip->fHandle);
		entryFilename[zipCDir.lenFilename] = 0;

		if (stricmp(filename, entryFilename) == 0) {
			/* Found the file, seek to the data. */
			*lenContent = zipCDir.lenUncompressed;

			/* This moves to the start of the local file header. */
			fseek(zip->fHandle, zipCDir.offLocal, SEEK_SET);

			/* Read the signature to confirm we're at the right place. */
			fread(&sig, 4, 1, zip->fHandle);
			if (sig != ZIP_SIG_LDIR) {
				debug("zip: cdir pointed to invalid ldir\n");
				return ZIP_ERR_NOT_FOUND;
			}

			len = fread(&zipLDir, 1, sizeof(zipLDir), zip->fHandle);
			if (len < sizeof(zipLDir)) {
				debug("zip: short read on file %d ldir (got %u, exp %u).\n", i, len, sizeof(zipLDir));
				return ZIP_ERR_IO;
			}

			if (zipLDir.compression != 0) {
				debug("zip: unsupported compression algorithm\n");
				return ZIP_ERR_ALGO;
			}

			/* Skip over the variable-length fields. */
			fseek(zip->fHandle, zipLDir.lenFilename + zipLDir.lenExtra, SEEK_CUR);

			return ZIP_OK;
		}

		/* Skip over the trailing fields we aren't interested in. */
		fseek(zip->fHandle, zipCDir.lenExtra + zipCDir.lenComment, SEEK_CUR);
	}

	debug("zip: not found: %s\n", filename);
	return ZIP_ERR_NOT_FOUND;
}

#define CHUNK_SIZE 4096
int copyStream(FILE *fOut, FILE *fIn, unsigned long lenCopy)
{
	char buffer[CHUNK_SIZE];
	unsigned int lenNext, len;
	unsigned long off = 0;

	if (lenCopy == 0) return ZIP_OK;

	do {
		lenNext = lenCopy - off;
		if (lenNext > CHUNK_SIZE) lenNext = CHUNK_SIZE;
		len = fread(buffer, 1, lenNext, fIn);
		if (len == 0) {
			debug("copyStream: Short read (got %u, exp %u).\n", len, lenNext);
			return ZIP_ERR_IO;
		}
		len = fwrite(buffer, 1, lenNext, fOut);
		if (len == 0) {
			debug("copyStream: Short write (got %u, exp %u).\n", len, lenNext);
			return ZIP_ERR_IO;
		}
		off += lenNext;
	} while (off < lenCopy);
	debug("copyStream: Copied %lu of %lu bytes\n", off, lenCopy);

	return ZIP_OK;
}

/* Take a .zip file and attach it onto the end of an .exe, updating the offsets
   in the .zip file so the resulting .exe can still be opened with normal .zip
   utilities. */
int zipReplace(FILE *fExe, FILE *fZip, FILE *fOutput)
{
	unsigned short lenFinalBlock, numBlocks;
	unsigned long offEnd, offCopy;
	uint32_t sig;
	int len;
	ZIP_LDIR_ENTRY zipLDir;
	ZIP_CDIR_ENTRY zipCDir;
	ZIP_CDIR_END zipCEnd;

	fseek(fExe, 2, SEEK_SET);
	fread(&lenFinalBlock, 2, 1, fExe);
	fread(&numBlocks, 2, 1, fExe);

	offEnd = numBlocks * 512;
	if (lenFinalBlock) offEnd -= 512 - lenFinalBlock;

	/* Copy the .exe, removing any existing trailing data. */
	fseek(fExe, 0, SEEK_SET);
	copyStream(fOutput, fExe, offEnd);

	/* Copy the .zip, updating the offsets by offEnd (the new start of the zip
	   file. */
	fseek(fZip, 0, SEEK_SET);
	do {

		len = fread(&sig, 1, 4, fZip);
		if (len < 4) {
			debug("zipReplace: short read (got %u, exp 4)\n", len);
			return ZIP_ERR_IO;
		}

		len = fwrite(&sig, 1, 4, fOutput);
		if (len < 4) {
			debug("zipReplace: short write to output (got %u, exp 4)\n", len);
			return ZIP_ERR_IO;
		}

		/* Turbo C doesn't appear to be able to handle 32-bit values in a switch
		   statement, so we only check the two bytes following the "PK". */
		switch (sig >> 16) {
			case ZIP_SIG_LDIR >> 16:
				debug("zipReplace: copying ldir\n");
				len = fread(&zipLDir, 1, sizeof(zipLDir), fZip);
				if (len != sizeof(zipLDir)) {
					debug("zipReplace: short read from zip (got %u, exp %u)\n", len, sizeof(zipLDir));
					return ZIP_ERR_IO;
				}
				len = fwrite(&zipLDir, 1, sizeof(zipLDir), fOutput);
				if (len != sizeof(zipLDir)) {
					debug("zipReplace: short write to output (got %u, exp %u)\n", len, sizeof(zipLDir));
					return ZIP_ERR_IO;
				}

				len = copyStream(fOutput, fZip, zipLDir.lenFilename + zipLDir.lenExtra + zipLDir.lenCompressed);
				if (len != ZIP_OK) return len;

				break;

			case ZIP_SIG_CDIR >> 16:
				debug("zipReplace: copying cdir\n");
				len = fread(&zipCDir, 1, sizeof(zipCDir), fZip);
				if (len != sizeof(zipCDir)) {
					debug("zipReplace: short read from zip (got %u, exp %u)\n", len, sizeof(zipCDir));
					return ZIP_ERR_IO;
				}
				zipCDir.offLocal += offEnd; /* update local file offset */
				len = fwrite(&zipCDir, 1, sizeof(zipCDir), fOutput);
				if (len != sizeof(zipCDir)) {
					debug("zipReplace: short write to output (got %u, exp %u)\n", len, sizeof(zipCDir));
					return ZIP_ERR_IO;
				}

				len = copyStream(fOutput, fZip, zipCDir.lenFilename + zipCDir.lenExtra + zipCDir.lenComment);
				if (len != ZIP_OK) return len;

				break;

			case ZIP_SIG_CDIR_END >> 16:
				debug("zipReplace: copying cend\n");
				len = fread(&zipCEnd, 1, sizeof(zipCEnd), fZip);
				if (len != sizeof(zipCEnd)) {
					debug("zipReplace: short read from zip (got %u, exp %u)\n", len, sizeof(zipCEnd));
					return ZIP_ERR_IO;
				}
				zipCEnd.offDirectory += offEnd; /* update central directory offset */
				len = fwrite(&zipCEnd, 1, sizeof(zipCEnd), fOutput);
				if (len != sizeof(zipCEnd)) {
					debug("zipReplace: short write to output (got %u, exp %u)\n", len, sizeof(zipCEnd));
					return ZIP_ERR_IO;
				}

				len = copyStream(fOutput, fZip, zipCEnd.lenComment);
				if (len != ZIP_OK) return len;

				break;

			default:
				debug("zipReplace: invalid signature 0x%08lX\n", sig);
				return ZIP_ERR_ALGO;
		}
	} while (sig != ZIP_SIG_CDIR_END);

	return ZIP_OK;
}
