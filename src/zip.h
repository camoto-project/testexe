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

#ifndef _ZIP_H_
#define _ZIP_H_

#include <stdio.h>
#include "types.h"

typedef struct {
	FILE *fHandle;
	uint32_t offDir;    /* Offset to start of central directory */
	uint16_t fileCount; /* Number of entries in central directory */
} ZIP;

#define ZIP_OK             0 /* All good */
#define ZIP_ERR_IO        -1 /* Short read */
#define ZIP_ERR_NO_DIR    -2 /* Central directory not found */
#define ZIP_ERR_NOT_FOUND -3 /* File not found inside .zip */
#define ZIP_ERR_ALGO      -4 /* Unsupported algorithm */

int zipOpen(ZIP *zip, FILE *fZipFile);
int zipSeekFile(const ZIP *zip, const char *filename, uint32_t *lenContent);

#endif /* _ZIP_H_ */
