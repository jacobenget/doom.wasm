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

/*
 * Initialize Doom - exported as part of the function `initGame()`
 *
 * NOTE: `_initializeDoom` is exported and then externally combined with the
 * auto-generated and exported function `_initialize` to produce the single
 * exported function named `initGame`.
 *
 * This is done so the user of this WebAssembly module only has one exported
 * 'init' function they're expected to call: `initGame`.
 *
 * ps. An `_initialize` function is auto-generated and then exported because
 * this module is built as a WASI 'reactor', see here:
 * https://github.com/WebAssembly/WASI/blob/main/legacy/application-abi.md#current-unstable-abi
 */
EXPORT void _initializeDoom();

/*
 * Advance Doom by one 'tick' (i.e. one frame)
 *
 * Allows Doom to render a new frame after reacting to any key state changes
 * reported since the last `tick`.
 */
EXPORT void tickGame();

/*
 * Report to Doom that a key is now pressed down
 *
 * args:
 *  doomKey:
 *    - A numerical representation of the key, in the range [0, 255]
 *    - If the key in question naturally produces a printable ASCII character:
 *      - then the `doomKey` associated with the key is the ASCII code for that
 *        printable character
 *      - e.g. the keys 'a' through 'z' have a `doomKey` value of 97 through
 *        122, respectively
 *    - Otherwise:
 *      - then the `doomKey` associated with the key is some special value
 *      - e.g. the key for "USE"
 *      - all such special `doomKey` values are exported from `doom.wasm` as
 *        global constants with a helpful descriptive name (e.g. "KEY_USE")
 *
 * Note: calling this function with a `doomKey` value outside of the range [0,
 * 255] only results in a logged error message, no other harm is done
 *
 * Note: multiple calls to this function with the same `doomKey`, without a call
 * to `reportKeyUp` with the same `doomKey` in between, will just be ignored
 */
EXPORT void reportKeyDown(int32_t doomKey);

/*
 * Report to Doom that a key is no longer pressed down
 *
 * args:
 *  doomKey:
 *    - A numerical representation of the key, in the range [0, 255]
 *    - If the key in question naturally produces a printable ASCII character:
 *      - then the `doomKey` associated with the key is the ASCII code for that
 *        printable character
 *      - e.g. the keys 'a' through 'z'
 *    - Otherwise:
 *      - then the `doomKey` associated with the key is some special value
 *      - e.g. the key for "USE"
 *      - all such special `doomKey` values are exported from `doom.wasm` as
 *        global constants with a helpful descriptive name (e.g. "KEY_USE")
 *
 * Note: calling this function with a `doomKey` value outside of the range [0,
 * 255] only results in a logged error message, no other harm is done
 *
 * Note: multiple calls to this function with the same `doomKey`, without a call
 * to `reportKeyDown` with the same `doomKey` in between, will just be ignored
 */
EXPORT void reportKeyUp(int32_t doomKey);

// *****************************************************************************
// *                             IMPORTED FUNCTIONS                            *
// *****************************************************************************

/*
 * Perform one-time initialization upon Doom first starting up
 *
 * args:
 *  width:
 *    - width, in pixels, of frame buffer passed to `drawFrame`
 *  height:
 *    - height, in pixels, of frame buffer passed to `drawFrame`
 */
IMPORT_MODULE("loading") void onGameInit(int32_t width, int32_t height);

/*
 * Report size information about the WAD data that Doom should load
 *
 * The information provided by this function allows Doom to prepare the space
 * in memory needed to then call `readWads` to receive the data in all WADs that
 * are to be loaded by Doom.
 *
 * The value stored in `*numberOfWads` before this function is called is 0. The
 * value of `*numberOfWads` still being 0 when this function returns
 * communicates to Doom "No custom WAD data to load; please load the Doom
 * shareware WAD instead".
 *
 * In other words, this function is allowed to do nothing and that will result
 * in `readWads` NOT being called and the Doom Shareware WAD being loaded into
 * Doom.
 *
 * args:
 *  numberOfWads:
 *    - pointer to the number of WADs that Doom will be loading
 *  numberOfTotalBytesInAllWads:
 *    - pointer to the total combined size, in bytes, of the WADs that Doom
 * should load
 */
IMPORT_MODULE("loading")
void wadSizes(int32_t *numberOfWads, size_t *numberOfTotalBytesInAllWads);

/*
 * Copy to memory the data for all WAD files that Doom should load, and the byte
 * length of each WAD file
 *
 * To understand how this function operates, consider that this function is
 * called immediately after Doom calls `wadSizes`, and `wadSizes` provided these
 * two values: `numberOfWads`
 *    - the number of WAD files that should be loaded
 *  `numberOfTotalBytesInAllWads`
 *    - the total length, in bytes, of all WAD files combined
 *
 * This function is only called if the `numberOfWads` value provided by
 * `wadSizes` is greater than 0.
 *
 * args:
 *  wadDataDestination:
 *    - location in memory where the data for all WAD files should be written,
 *      end-to-end. The order that the WADs are written here determines the
 *      order in which Doom loads them. The number of bytes available to write
 *      to at `wadDataDestination` is exactly `numberOfTotalBytesInAllWads`, and
 *      this function is expected to write exactly that number of bytes.
 *  byteLengthOfEachWad:
 *    - location in memory where an array of `int32_t` values with length
 *      `numberOfWads` exists. This array should be populated with exactly
 *      `numberOfWads` values, each which is the byte length of the respective
 *      WAD file.
 */
IMPORT_MODULE("loading")
void readWads(uint8_t *wadDataDestination, int32_t *byteLengthOfEachWad);

/*
 * Respond to a new frame of the Doom game being available
 *
 * args:
 *  screenBuffer:
 *    - pointer to Doom's frame buffer.
 *      - Doom's frame buffer is exactly `width`*`height` 32-bit pixels
 *        contiguous in memory, where `width` and `height` were the values
 *        previously passed to `onGameInit`
 *      - The pixels are ordered row major, from top-left pixel (index 0) to
 *        bottom-right (index `width`*`height`)
 *      - The color data in each 32-bit pixel is made up of four 8-bit color
 *        components packed in the order BGRA (i.e. Blue, Green, Red, Alpha),
 *        from low/first byte to high/last byte when the 32-bit pixel is seen
 *        as an array of 4 bytes.
 */
IMPORT_MODULE("ui") void drawFrame(uint32_t *screenBuffer);

/*
 * Respond to Doom reporting an info message
 *
 * args:
 *  message:
 *    - the message itself; NOT NULL-terminated like a usual C-string
 *  length:
 *    - length, in bytes, of the message
 */
IMPORT_MODULE("console") void onInfoMessage(const char *message, size_t length);

/*
 * Respond to Doom reporting an error message
 *
 * args:
 *  message:
 *    - the message itself; NOT NULL-terminated like a usual C-string
 *  length:
 *    - length, in bytes, of the message
 */
IMPORT_MODULE("console")
void onErrorMessage(const char *message, size_t length);

/*
 * Provide a representation of the current 'time', in milliseconds
 *
 * This function may never return a value that is smaller than a value it
 * previously returned, but that's the only requirement on how this function is
 * implemented.
 *
 * This function controls how time passes in Doom. A natural implementation of
 * this function would return the number of milliseconds that have passed since
 * some fixed point in time.
 *
 * returns:
 *  a value representing the current time, in milliseconds
 */
IMPORT_MODULE("runtimeControl") uint64_t timeInMilliseconds();

/*
 * Report the size, in bytes, of a specific save game
 *
 * This information provided by this function allows Doom to prepare any space
 * in memory needed to then call `readSaveGame` to receive the data associated
 * with a specific save game.
 *
 * args:
 *  gameSaveId:
 *    - identifies a specific save game
 *
 * returns:
 *  Number of bytes in the associated save game data. Returns 0 if no save game
 *  data exists for this `gameSaveId`.
 */
IMPORT_MODULE("gameSaving") size_t sizeOfSaveGame(int32_t gameSaveId);

/*
 * Copy data for a specific save game to memory
 *
 * args:
 *  gameSaveId:
 *    - id associated with the save game
 *    - saved games are identified completely by this number
 *  dataDestination:
 *    - location in memory where the bytes of the save game should be copied.
 *      It's guaranteed that at least `X` bytes are reserved in memory to hold
 *      this save game data, where `X` is the value last returned when
 *      `gameSaving_sizeOfSaveGame` was called with the given `gameSaveId`.
 *
 * returns:
 *  Number of bytes of data game data actually copied.
 *
 * Note: this function will only ever be called if `sizeOfSaveGame` returned a
 * non-zero value for this `gameSaveId`.
 */
IMPORT_MODULE("gameSaving")
size_t readSaveGame(int32_t gameSaveId, uint8_t *dataDestination);

/*
 * Respond to the user attempting to save their game
 *
 * args:
 *  gameSaveId:
 *    - id associated with the save game
 *    - saved game are identified completely by this number
 *  data:
 *    - location of the bytes of the save game data
 *  length:
 *    - the length, in bytes, of the save game data
 *
 * returns:
 *  Number of bytes of data game data actually persisted
 *
 * Note: this function should return 0 in the case where saving of games isn't
 * supported.
 */
IMPORT_MODULE("gameSaving")
size_t writeSaveGame(int32_t gameSaveId, uint8_t *data, size_t length);

#endif /* DOOM_WASM_H_ */
