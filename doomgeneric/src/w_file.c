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
#include <string.h>

#include "config.h"

#include "doomtype.h"
#include "m_argv.h"

#include "w_file.h"
#include "m_misc.h"
#include "z_zone.h"

wad_file_t *W_OpenFile(byte *wadData, size_t wadByteLength) {
  wad_file_t *result = Z_Malloc(sizeof(wad_file_t), PU_STATIC, 0);
  result->mapped = wadData;
  result->length = wadByteLength;

  return result;
}

void W_CloseFile(wad_file_t *wad) { Z_Free(wad); }

size_t W_Read(wad_file_t *wad, unsigned int offset, void *buffer,
              size_t buffer_len) {
  memcpy(buffer, wad->mapped + offset, buffer_len);
  return buffer_len;
}
