
#ifndef DOOM_IMPORTS_H_
#define DOOM_IMPORTS_H_

#include <stdint.h>

#include "doom_exports.h"

/*
  This file declares the imports needed to properly instantiate and run the Doom
  WebAssembly module.

  Each function here is passed an instance of `doom_module_context_t` to enable
  interaction with any of the features exported by the Doom WebAssembly module
  (e.g. for getting access to the exported memory).

  This file also describes a high-level `run_game` function that is expected to
  fully drive the execution of Doom by leveraging the exports of an instance of
  the Doom WebAssembly module.
*/

/*
 * Returns a non-NULL error if there was an issue when running the game,
 * otherwise NULL is returned on success.
 */
doom_module_error_t *run_game(doom_module_context_t *context);

///////////////////////////////////////////////////////////////////////////////
//
// All imports needed by the Doom WebAssembly module.
//
// Each import receives an `doom_module_context_t*` as its first arg.
// which is the key to accessing any of the features exported by the Doom
// WebAssembly, e.g. the exported memory.
//
///////////////////////////////////////////////////////////////////////////////

/*
 * Perform one-time initialization upon Doom first starting up
 *
 * args:
 *  context:
 *    - allows interaction with Doom WebAssembly module exports
 *  width:
 *    - width, in pixels, of the frame buffer that will be passed to
 *      `ui_drawFrame`
 *  height:
 *    - height, in pixels, of the frame buffer that will be passed to
 *      `ui_drawFrame`
 *
 * Implements Doom import: function loading.onGameInit(i32, i32) -> ()
 */
void loading_onGameInit(doom_module_context_t *context, int32_t width,
                        int32_t height);

/*
 * Report size information about the WAD data that Doom should load
 *
 * This information provided by this function allows Doom to prepare the space
 * in memory needed to then call `loading_readWads` to receive the data in all
 * WADs that are to be loaded by Doom.
 *
 * This function communicates this information to Doom:
 *  `int32_t numberOfWads`
 *    - the number of WAD files that should be loaded
 *  `int32_t numberOfTotalBytesInAllWads`
 *    - the total length, in bytes, of all WAD files combined
 *
 * This function communicates these details by writing related `int32_t` values,
 * in little-endian order, to the memory exported by the Doom WebAssembly
 * module.
 *
 * The `int32_t` value stored in the memory locations for `numberOfWads` before
 * this function is called is 0. The value for `numberOfWads` still being 0 when
 * this function returns communicates to Doom "No custom WAD data to load;
 * please load the Doom shareware WAD instead".
 *
 * In other words, this function is allowed to do nothing and that will result
 * in `loading_readWads` NOT being called and the Doom Shareware WAD being
 * loaded into Doom.
 *
 * args:
 *  context:
 *    - allows interaction with Doom WebAssembly module exports
 *  numberOfWadsOffset:
 *    - byte index into Doom exported memory where the value `int32_t
 *      numberOfWads` should be written in little-endian order
 *  numberOfTotalBytesInAllWadsOffset:
 *    - byte index into Doom exported memory where the value `int32_t
 *      numberOfTotalBytesInAllWads` should be written in little-endian order
 *
 * Implements Doom import: function loading.wadSizes(i32, i32) -> ()
 */
void loading_wadSizes(doom_module_context_t *context,
                      int32_t numberOfWadsOffset,
                      int32_t numberOfTotalBytesInAllWadsOffset);

/*
 * Copy, to memory exported by the Doom WebAssembly module, the data for all WAD
 * files that Doom should load, and the byte length of each WAD file
 *
 * To understand how this function operates, consider that this function is
 * called immediately after a call to `loading_wadSizes`, and the result of
 * calling `loading_wadSizes` is that two int32_t values have been written to
 * memory:
 *  `int32_t numberOfWads`
 *    - the number of WAD files that should be loaded
 *  `int32_t numberOfTotalBytesInAllWads`
 *    - the total length, in bytes, of all WAD files combined
 *
 * This function is only called if the `numberOfWads` value after the call to
 * `loading_wadSizes` is greater than 0.
 *
 * args:
 *  context:
 *    - allows interaction with Doom WebAssembly module exports
 *  wadDataDestinationOffset:
 *    - bytes index into Doom exported memory where the data for all WAD files
 *      should be written, end-to-end. The order that the WADs are written here
 *      determines the order in which Doom loads them. The number of bytes
 *      available to write to at `wadDataDestinationOffset` is exactly
 *      `numberOfTotalBytesInAllWads`, and this function is expected to write
 *      exactly that number of bytes.
 *  byteLengthOfEachWadOffset:
 *    - bytes index into Doom exported memory where an array of `int32_t` with
 *      length `numberOfWads` exists. To this array should be written exactly
 *      `numberOfWads` `int32_t` values, in little-endian fashion, each which is
 *      the byte length of the respective WAD file.
 *
 * Implements Doom import: function loading.readWads(i32, i32) -> ()
 */
void loading_readWads(doom_module_context_t *context,
                      int32_t wadDataDestinationOffset,
                      int32_t byteLengthOfEachWadOffset);

/*
 * Provide a representation of the current 'time', in milliseconds
 *
 * This function may never return a value that is smaller than a value it
 * previously returned, but that's the only requirement on how this function is
 * implemented.
 *
 * This function controls how time passes in Doom. A natural implementation of
 * this function would return the number of milliseconds that have passed since
 * some fixed moment in time.
 *
 * args:
 *  context:
 *    - allows interaction with Doom WebAssembly module exports
 *
 * returns:
 *  a value representing the current time, in milliseconds
 *
 * Implements Doom import: function runtimeControl.timeInMilliseconds() -> (i64)
 */
int64_t runtimeControl_timeInMilliseconds(doom_module_context_t *context);

/*
 * Respond to a new frame of the Doom game being available
 *
 * args:
 *  context:
 *    - allows interaction with Doom WebAssembly module exports
 *  screenBufferOffset:
 *    - byte index into Doom exported memory where the bytes/pixels of the
 *      screen buffer reside.
 *      - The screen buffer is exactly `width`*`height` int32_t pixels
 *        contiguous in memory, where `width` and `height` were the values
 *        passed perviously to `loading_onGameInit`
 *      - The pixels are ordered row major, from top-left pixel (index 0) to
 *        bottom-right (index `width`*`height`)
 *      - The color data in each 32-bit pixel is made up of four 8-bit color
 *        components packed in the order BGRA (i.e. Blue, Green, Red, Alpha),
 *        from low/first byte to high/last byte when the `int32_t` pixel is seen
 *        as an array of 4 bytes.
 *
 * Implements Doom import: function ui.drawFrame(i32) -> ()
 */
void ui_drawFrame(doom_module_context_t *context, int32_t screenBufferOffset);

/*
 * Report the size, in bytes, of a specific save game
 *
 * args:
 *  context:
 *    - allows interaction with Doom WebAssembly module exports
 *  gameSaveId:
 *    - identifies a specific save game
 *
 * returns:
 *  Number of bytes in the associated save game data. Returns 0 if no save game
 *  data exists for this `gameSaveId`.
 *
 * Implements Doom import: function gameSaving.sizeOfSaveGame(i32) -> (i32)
 */
int32_t gameSaving_sizeOfSaveGame(doom_module_context_t *context,
                                  int32_t gameSaveId);

/*
 * Copy, to memory exported by the Doom WebAssembly module, the data for a
 * specific save game
 *
 * args:
 *  context:
 *    - allows interaction with Doom WebAssembly module exports
 *  gameSaveId:
 *    - id associated with the save game
 *    - saved games are identified completely by this number
 *  dataDestinationOffset:
 *    - byte index into Doom exported memory where the bytes of the save game
 *      should be copied. It's guaranteed that at least `X` bytes are reserved
 *      in memory to hold this save game data, where `X` is the value last
 *      returned when `gameSaving_sizeOfSaveGame` was called with the same
 *      `gameSaveId`.
 *
 * returns:
 *  Number of bytes of data game data actually copied.
 *
 * Note: this function will only ever be called if `gameSaving_sizeOfSaveGame`
 * returned a non-zero value for this `gameSaveId`.
 *
 * Implements Doom import: function gameSaving.readSaveGame(i32, i32) -> (i32)
 */
int32_t gameSaving_readSaveGame(doom_module_context_t *context,
                                int32_t gameSaveId,
                                int32_t dataDestinationOffset);

/*
 * Respond to the user attempting to save their game
 *
 * args:
 *  context:
 *    - allows interaction with Doom WebAssembly module exports
 *  gameSaveId:
 *    - id associated with the save game
 *    - saved games are identified completely by this number
 *  dataOffset:
 *    - byte index into Doom exported memory where the bytes of the save game
 *      begin
 *  length:
 *    - the length, in bytes, of the save game data.
 *
 * returns:
 *  Number of bytes of data game data actually persisted
 *
 * Note: this function should return 0 in the case where saving of games isn't
 * supported.
 *
 * Implements Doom import: function gameSaving.writeSaveGame(i32, i32, i32) ->
 * (i32)
 */
int32_t gameSaving_writeSaveGame(doom_module_context_t *context,
                                 int32_t gameSaveId, int32_t dataOffset,
                                 int32_t length);

/*
 * Respond to Doom reporting a `char*` info message
 *
 * args:
 *  context:
 *    - allows interaction with Doom WebAssembly module exports
 *  messageOffset:
 *    - byte index into Doom exported memory where the first `char` of the
 *      message resides, the remaining characters of the message appear
 *      sequentially after.
 *    - the message is NOT NULL-terminated like a usual C-string.
 *  length:
 *    - length, in bytes, of the message
 *
 * Implements Doom import: function console.onInfoMessage(i32, i32) -> ()
 */
void console_onInfoMessage(doom_module_context_t *context,
                           int32_t messageOffset, int32_t length);

/*
 * Respond to Doom reporting a `char*` info message
 *
 * args:
 *  context:
 *    - allows interaction with Doom WebAssembly module exports.
 *  messageOffset:
 *    - byte index into Doom's exported memory where the first `char` of the
 *      message resides, the remaining characters of the message appear
 *      sequentially after.
 *    - the message is NOT NULL-terminated like a usual C-string.
 *  length:
 *    - length, in bytes, of the message
 *
 * Implements Doom import: function console.onErrorMessage(i32, i32) -> ()
 */
void console_onErrorMessage(doom_module_context_t *context,
                            int32_t messageOffset, int32_t length);

#endif
