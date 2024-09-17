//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 1993-2008 Raven Software
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
//      Miscellaneous File Stuff.
//

#include "file_misc.h"

//
// WriteFile
//
bool WriteFile(char *name, void *source, int length) {
  FILE *handle;
  int count;

  handle = fopen(name, "wb");

  if (handle == NULL)
    return false;

  count = fwrite(source, 1, length, handle);
  fclose(handle);

  if (count < length)
    return false;

  return true;
}

//
// FileLength - Determine the length of an open file.
//
long FileLength(FILE *handle) {
  // save the current position in the file
  long savedpos = ftell(handle);

  // jump to the end and find the length
  fseek(handle, 0, SEEK_END);
  long length = ftell(handle);

  // go back to the old location
  fseek(handle, savedpos, SEEK_SET);

  return length;
}

//
// FileExists
//
bool FileExists(char *pathToFile) {
  FILE *handle = fopen(pathToFile, "r");

  bool fileExists = (handle != NULL);
  if (handle != NULL) {
    fclose(handle);
  }
  return fileExists;
}
