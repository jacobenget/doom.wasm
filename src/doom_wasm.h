#ifndef DOOM_WASM_H_
#define DOOM_WASM_H_

#include <stddef.h>
#include <stdint.h>

#define IMPORT_MODULE(moduleName) __attribute__((import_module(moduleName)))
#define EXPORT __attribute__((visibility("default")))

/*
 * This file specifies the module interface (i.e. imports and exports) that is
 * produced when Doom is compiled to a WebAssembly module.
 */

// *****************************************************************************
// *                             EXPORTED FUNCTIONS                            *
// *****************************************************************************

// NOTE: `_initializeDoom` is exported and then externally combined with the
// auto-generated and exported function `_initialize` to produce the single
// exported function named `initGame`.
//
// This is done so the user of this WebAssembly module only has one exported
// 'init' function they're expected to call: `initGame`.
//
// ps. An `_initialize` function is auto-generated and then exported because
// this module is built as a WASI 'reactor', see here:
// https://github.com/WebAssembly/WASI/blob/main/legacy/application-abi.md#current-unstable-abi
EXPORT void _initializeDoom();
EXPORT void tickGame();
EXPORT void reportKeyDown(int32_t doomKey);
EXPORT void reportKeyUp(int32_t doomKey);

// *****************************************************************************
// *                             IMPORTED FUNCTIONS                            *
// *****************************************************************************

IMPORT_MODULE("loading") void onGameInit(int32_t width, int32_t height);
IMPORT_MODULE("loading")
void getWadsSizes(int32_t *numberOfWads, size_t *numberOfTotalBytesInAllWads);
IMPORT_MODULE("loading")
void readDataForAllWads(uint8_t *wadDataDestination,
                        int32_t *byteLengthOfEachWad);

IMPORT_MODULE("ui") void drawFrame(uint32_t *screenBuffer);

IMPORT_MODULE("console") void info(const char *message, size_t length);
IMPORT_MODULE("console") void error(const char *message, size_t length);

IMPORT_MODULE("runtimeControl") uint64_t getTicksMs();

// Return the size of the associated save game data, 0 in the case that no save
// data exists for this slot
IMPORT_MODULE("gameSaving") size_t sizeOfSaveGame(int32_t gameSaveId);
// Return the number of bytes read
IMPORT_MODULE("gameSaving")
size_t readSaveGame(int32_t gameSaveId, uint8_t *dataDestination);
// Return the number of bytes written
IMPORT_MODULE("gameSaving")
size_t writeSaveGame(int32_t gameSaveId, uint8_t *data, size_t length);

#endif /* DOOM_WASM_H_ */
