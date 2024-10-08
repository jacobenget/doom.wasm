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
//      Miscellaneous File Stuff.
//

#ifndef __FILE_MISC__
#define __FILE_MISC__

#include <stdbool.h>
#include <stdio.h>

bool WriteFile(char *name, void *source, int length);
long FileLength(FILE *handle);
bool FileExists(char *pathToFile);

#endif