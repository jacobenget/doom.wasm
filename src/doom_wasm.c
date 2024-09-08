#include "doomgeneric.h"

/*
 * This file provides augmentations to the interface (i.e. imports and exports)
 * that is produced when Doom is compiled to a WebAssembly module.
 */

__attribute__((visibility("default"))) void initGame(int argc, char **argv) {
  doomgeneric_Create(argc, argv);
}

__attribute__((visibility("default"))) void tickGame() { doomgeneric_Tick(); }
