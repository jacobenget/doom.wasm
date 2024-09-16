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

#include <stdio.h>

#include "config.h"

#include "doomtype.h"
#include "m_argv.h"

#include "w_file.h"
#include "m_misc.h"
#include "z_zone.h"

typedef struct {
  wad_file_t wad;
  FILE *fstream;
} file_based_wad_file_t;

wad_file_t *W_OpenFile(char *path) {
  file_based_wad_file_t *result;
  FILE *fstream;

  fstream = fopen(path, "rb");

  if (fstream == NULL) {
    return NULL;
  }

  // Create a new file_based_wad_file_t to hold the file handle.

  result = Z_Malloc(sizeof(file_based_wad_file_t), PU_STATIC, 0);
  result->wad.mapped = NULL;
  result->wad.length = M_FileLength(fstream);
  result->fstream = fstream;

  return &result->wad;
}

void W_CloseFile(wad_file_t *wad) {
  file_based_wad_file_t *wad_file;

  wad_file = (file_based_wad_file_t *)wad;

  fclose(wad_file->fstream);
  Z_Free(wad_file);
}

size_t W_Read(wad_file_t *wad, unsigned int offset, void *buffer,
              size_t buffer_len) {
  file_based_wad_file_t *wad_file;
  size_t result;

  wad_file = (file_based_wad_file_t *)wad;

  // Jump to the specified position in the file.

  fseek(wad_file->fstream, offset, SEEK_SET);

  // Read into the buffer.

  result = fread(buffer, 1, buffer_len, wad_file->fstream);

  return result;
}
