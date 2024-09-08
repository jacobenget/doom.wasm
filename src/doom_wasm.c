#include "doomgeneric.h"

/*
 * This file provides augmentations to the interface (i.e. imports and exports)
 * that is produced when Doom is compiled to a WebAssembly module.
 */

// *****************************************************************************
// *                             IMPORTED FUNCTIONS                            *
// *****************************************************************************

__attribute__((import_module("ui"))) void onGameInit();
__attribute__((import_module("ui"))) void drawFrame();
__attribute__((import_module("ui"))) int getKey(int *pressed,
                                                unsigned char *key);
__attribute__((import_module("ui"))) void setWindowTitle(const char *title);

__attribute__((import_module("runtimeControl"))) void sleepMs(uint32_t ms);
__attribute__((import_module("runtimeControl"))) uint32_t getTicksMs();

// *****************************************************************************
// *                             EXPORTED FUNCTIONS                            *
// *****************************************************************************

__attribute__((visibility("default"))) void initGame(int argc, char **argv) {
  doomgeneric_Create(argc, argv);
}

__attribute__((visibility("default"))) void tickGame() { doomgeneric_Tick(); }

// *****************************************************************************
// *                           IMPLEMENTATION DETAILS                          *
// *****************************************************************************

// Here we implement the doomgeneric interface via the functions imported via
// WebAssembly imports.

void DG_Init() { onGameInit(); }

void DG_DrawFrame() { drawFrame(); }

int DG_GetKey(int *pressed, unsigned char *key) { return getKey(pressed, key); }

void DG_SetWindowTitle(const char *title) { setWindowTitle(title); }

void DG_SleepMs(uint32_t ms) { sleepMs(ms); }

uint32_t DG_GetTicksMs() { return getTicksMs(); }
