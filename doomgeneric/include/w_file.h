//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005-2014 Simon Howard
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//	WAD I/O functions.
//

#ifndef __W_FILE__
#define __W_FILE__

#include <stdio.h>
#include "doomtype.h"

typedef struct _wad_file_s wad_file_t;

struct _wad_file_s {
  // If this is NULL, the file cannot be mapped into memory.  If this
  // is non-NULL, it is a pointer to the mapped file.

  byte *mapped;

  // Length of the file, in bytes.

  size_t length;
};

// Create a file-like wrapper around the specified WAD data.
// Returns a pointer to a new wad_file_t handle for the WAD data,
// or NULL if this could not be done.

wad_file_t *W_OpenFile(byte *wadData, size_t wadByteLength);

// Close the specified WAD file.

void W_CloseFile(wad_file_t *wad);

// Read data from the specified file into the provided buffer.  The
// data is read from the specified offset from the start of the file.
// Returns the number of bytes read.

size_t W_Read(wad_file_t *wad, unsigned int offset, void *buffer,
              size_t buffer_len);

#endif /* #ifndef __W_FILE__ */
