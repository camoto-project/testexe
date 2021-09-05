/*
 * Main program.
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

#include "debug.h"
#include "types.h"
#include "zip.h"

/* Exit codes */
#define ERRORLEVEL_OK       0
#define ERRORLEVEL_IOERROR  1
#define ERRORLEVEL_NODATA   2

int main(int argc, const char *argv[])
{
	const char *filename;
	int len, result;
	long offEndSelf; /* Size of our .exe */
	FILE *fSelf, *fTarget, *fOutput;

	ZIP zip;
	uint32_t lenContent;
	char content[256];

	const char *dataFilename = argv[0];
	debug("loadmod: opening self: %s\n", dataFilename);
	fSelf = fopen(dataFilename, "rb");
	if (!fSelf) {
		fprintf(stderr, "Unable to open data file (%s).\n", dataFilename);
		return ERRORLEVEL_IOERROR;
	}

	if ((argc > 3) && (stricmp(argv[1], "-r") == 0)) {
		/* Replace the current zip (if any) with a new one. */
		printf("Reading %s\n", argv[2]);
		fTarget = fopen(argv[2], "rb");
		if (!fTarget) {
			fprintf(stderr, "Unable to open zip file (%s).\n", argv[2]);
			return ERRORLEVEL_IOERROR;
		}

		printf("Creating %s\n", argv[3]);
		/* For some reason fopen() doesn't truncate the file, seems like a bug in
		   the Turbo C standard library, so we'll just delete it first. */
		unlink(argv[3]);
		fOutput = fopen(argv[3], "wb");
		if (!fOutput) {
			fprintf(stderr, "Unable to open output file (%s).\n", argv[3]);
			return ERRORLEVEL_IOERROR;
		}

		result = zipReplace(fSelf, fTarget, fOutput);
		if (result < 0) {
			switch (result) {
				case ZIP_ERR_ALGO:
					fprintf(stderr, "Unsupported feature found in .zip file.\n");
					return ERRORLEVEL_NODATA;
			}

			fprintf(stderr, "Error reading replacement .zip\n");
			return ERRORLEVEL_IOERROR;
		}
		printf("Write complete.\n");

		fclose(fOutput);
		fclose(fTarget);
		fclose(fSelf);

		return ERRORLEVEL_OK;
	}

	result = zipOpen(&zip, fSelf);
	if (result < 0) {
		switch (result) {
			case ZIP_ERR_NO_DIR:
				fprintf(stderr, "You need to embed patch data before running this program!\n");
				return ERRORLEVEL_NODATA;
		}

		fprintf(stderr, "Error reading from file: %s\n", dataFilename);
		return ERRORLEVEL_IOERROR;
	}

	filename = "config.ini";
	result = zipSeekFile(&zip, filename, &lenContent);
	if (result < 0) {
		switch (result) {
			case ZIP_ERR_NO_DIR:
				fprintf(stderr, "Unable to find internal file list.\n");
				return ERRORLEVEL_NODATA;

			case ZIP_ERR_NOT_FOUND:
				fprintf(stderr, "Unable to find internal file: %s\n", filename);
				return ERRORLEVEL_NODATA;

			case ZIP_ERR_ALGO:
				fprintf(stderr, "Unsupported compression algorithm used for: %s\n", filename);
				return ERRORLEVEL_NODATA;
		}

		fprintf(stderr, "Error reading from file: %s\n", dataFilename);
		return ERRORLEVEL_IOERROR;
	}

	len = fread(content, lenContent, 1, fSelf);
	printf("%s has content:\n%s\n", filename, content);

	fclose(fSelf);

	return ERRORLEVEL_OK;
}
