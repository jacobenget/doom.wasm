"""
Runs Doom via WebAssembly.

Leverages PyGame to provide user input and support rendering to the screen.
Uses Wasmtime as the WebAssembly runtime.

Expected to be run a main Python program.
Accepts '--help' command line arg as a request for details on how to be run.
"""

import sys, os, time, struct, argparse

from wasmtime import Store, Module, Instance, Func, FuncType, ValType, Caller

import pygame as pg
import numpy as np
import einops


def _read_string_from_memory(caller: Caller, str_offset: int, length: int) -> str:
  """Utility to read a sting of UTF-8 characters from Caller's memory

  Args:
      caller (Caller): Wasmtime caller, provides access to exported memory
      str_offset (int): byte index into Doom exported memory where the string starts
      length (int): length of the string

  Returns:
      str: resultant Python string
  """
  memory = caller.get("memory")
  string_bytes = memory.read(caller, str_offset, str_offset + length)
  return string_bytes.decode('utf-8')


# Mapping from PyGame key values to labels for Doom keys.
# A user pressing or releasing one of these PyGame keys
# will result in Doom being told that the respective action
# happened to the related Doom key.
_doom_key_label_from_pg_key = {
  pg.K_LEFT: 'KEY_LEFTARROW',
  pg.K_RIGHT: 'KEY_RIGHTARROW',
  pg.K_UP: 'KEY_UPARROW',
  pg.K_DOWN: 'KEY_DOWNARROW',
  pg.K_COMMA: 'KEY_STRAFE_L',
  pg.K_PERIOD: 'KEY_STRAFE_R',
  pg.K_LCTRL: 'KEY_FIRE',
  pg.K_RCTRL: 'KEY_FIRE',
  pg.K_SPACE: 'KEY_USE',
  pg.K_LSHIFT: 'KEY_SHIFT',
  pg.K_RSHIFT: 'KEY_SHIFT',
  pg.K_TAB: 'KEY_TAB',
  pg.K_ESCAPE: 'KEY_ESCAPE',
  pg.K_RETURN: 'KEY_ENTER',
  pg.K_BACKSPACE: 'KEY_BACKSPACE',
  pg.K_LALT: 'KEY_ALT',
  pg.K_RALT: 'KEY_ALT',
}


#################################################################
#
#                   Imports Required by Doom
#
#################################################################


# State shared used by implementations of the functions that Doom imports
paths_to_wad_files = []


def loading__onGameInit(width: int, height: int) -> None:
  """Perform one-time initialization upon Doom first starting up

  Args:
      width (int): width, in pixels, of frame buffer passed to `ui__drawFrame`
      height (int): height, in pixels, of frame buffer passed to `ui__drawFrame`
  """
  pg.init()
  pg.display.set_mode((width, height))
  pg.display.set_caption('DOOM')


def loading__wadSizes(caller: Caller, number_of_wads_offset: int, number_of_total_bytes_in_all_wads_offset: int) -> None:
  """Report size information about the WAD data that Doom should load

  This information provided by this function allows Doom to prepare the space
  in memory needed to then call `loading_readWads` to receive the data in all WADs that are to
  be loaded by Doom.

  This function communicates this information to Doom:
    `int32_t numberOfWads`
      - the number of WAD files that should be loaded
    `int32_t numberOfTotalBytesInAllWads`
      - the total length, in bytes, of all WAD files combined

  This function communicates these details by writing related `int32_t` values,
  in little-endian order, to the memory exported by the Doom WebAssembly
  module.

  The `int32_t` value stored in the memory locations for `numberOfWads` before this
  function is called is 0. The value for `numberOfWads` still being 0 when this
  function returns communicates to Doom "No custom WAD data to load; please
  load the Doom shareware WAD instead".

  Args:
      caller (Caller): Wasmtime caller, provides access to exported memory
      number_of_wads_offset (int):
        byte index into Doom exported memory where the value `int32_t
        numberOfWads` should be written in little-endian order
      number_of_total_bytes_in_all_wads_offset (int):
        byte index into Doom exported memory where the value `int32_t
        numberOfTotalBytesInAllWads` should be written in little-endian order
  """
  try:
    number_of_wads = len(paths_to_wad_files)
    number_of_total_bytes_in_all_wads = sum([os.path.getsize(p) for p in paths_to_wad_files])
  except OSError as err:
    print(f'This error was encountered while loading a WAD file: {err}', file=sys.stderr)
    print(f'Defaulting to loading the Shareware WAD instead', file=sys.stderr)
    number_of_wads = 0
    number_of_total_bytes_in_all_wads = 0

  memory = caller.get("memory")
  memory.write(caller, struct.pack('<i', number_of_wads), number_of_wads_offset)
  memory.write(caller, struct.pack('<i', number_of_total_bytes_in_all_wads), number_of_total_bytes_in_all_wads_offset)


def loading__readWads(caller: Caller, wad_data_destination_offset: int, byte_length_of_each_wad_offset: int) -> None:
  """Copy, to memory exported by the Doom WebAssembly module, the data for all WAD files that Doom should load, and the byte length of each WAD file

  To understand how this function operates, consider that this function is
  called immediately after a call to `loading_wadSizes`, and the result of
  calling `loading_wadSizes` is that two int32_t values have been written to
  memory:
    `int32_t numberOfWads`
      - the number of WAD files that should be loaded
    `int32_t numberOfTotalBytesInAllWads`
      - the total length, in bytes, of all WAD files combined

  This function is only called if the `numberOfWads` value after the call to
  `loading_wadSizes` is greater than 0.

  Args:
      caller (Caller): Wasmtime caller, provides access to exported memory
      wad_data_destination_offset (int):
        bytes index into Doom exported memory where the data for all WAD files
        should be written, end-to-end. The order that the WADs are written here
        determines the order in which Doom loads them. The number of bytes
        available to write to at `wadDataDestinationOffset` is exactly
        `numberOfTotalBytesInAllWads`, and this function is expected to write
        exactly that number of bytes.
      byte_length_of_each_wad_offset (int):
        bytes index into Doom exported memory where an array of `int32_t` with
        length `numberOfWads` exists. To this array should be written exactly
        `numberOfWads` `int32_t` values, in little-endian fashion, each which is
        the byte length of the respective WAD file.
  """
  # Keep track of where the details for the current WAD should be copied
  cur_wad_data_offset = wad_data_destination_offset
  cur_wad_byte_length_offset = byte_length_of_each_wad_offset

  memory = caller.get("memory")

  # Write the data for each WAD, one at a time
  for path in paths_to_wad_files:
    with open(path, mode='rb') as wad_file:
      data = wad_file.read()
      wad_file_byte_length = len(data)

      memory.write(caller, data, cur_wad_data_offset)
      memory.write(caller, struct.pack('<i', wad_file_byte_length), cur_wad_byte_length_offset)

      cur_wad_data_offset += wad_file_byte_length
      cur_wad_byte_length_offset += 4


def runtimeControl__timeInMilliseconds() -> int:
  """Provide a representation of the current 'time', in milliseconds

  This function may never return a value that is smaller than a value it
  previously returned, but that's the only requirement on how this function is
  implemented.

  This function controls how time passes in Doom. A natural implementation of
  this function would return the number of milliseconds that have passed since
  some fixed moment in time.

  Returns:
      int: a value representing the current time, in milliseconds
  """
  return int(time.time() * 1000)


def ui__drawFrame(caller: Caller, screen_buffer_offset: int) -> None:
  """Respond to a new frame of the Doom game being available

  Args:
      caller (Caller): Wasmtime caller, provides access to exported memory
      screen_buffer_offset (int):
        byte index into Doom exported memory where the bytes/pixels of the
        screen buffer reside.
        - The screen buffer is exactly `width`*`height` int32_t pixels contiguous in
          memory, where `width` and `height` were the values passed perviously to
          `loading_onGameInit`
        - The pixels are ordered row major, from top-left pixel (index 0) to
          bottom-right (index `width`*`height`)
        - The color data in each 32-bit pixel is made up of four 8-bit color
          components packed in the order BGRA (i.e. Blue, Green, Red, Alpha),
          from low/first byte to high/last byte when the `int32_t` pixel is seen
          as an array of 4 bytes.
  """
  # `display`` dimensions were set to match dimensions of Doom buffer in loading__onGameInit,
  # so we can retrieve those here to remember the dimensions of the Doom buffer.
  (width, height) = pg.display.get_surface().get_size()

  memory = caller.get("memory")

  # Doom's frame buffer is a raw chunk of contiguous bytes, 4 for each pixel, ordered row-major.
  # The pixels in Doom's frame buffer have their 8-bit color components logically
  # ordered "ARGB", but this 32-bit value is stored in little-endian order (because
  # WebAssembly always orders multi-byte values in a little-endian way), so the order
  # of the bytes is reversed and is actually "BGRA".
  #
  # We want to use PyGame's `surfarray.blit_array` function to efficiently transfer the
  # pixels to the PyGame surface.
  #
  # One of the ways to call `surfarray.blit_array` is to give it a 3d numpy array, where
  # the highest/first dimension of this array is 'column' (i.e. column-major), the next
  # dimension is 'row' and the next dimension is filled with (R, G, B) value that describe
  # the color at that point.
  #
  # So, translating the pixels from the Doom format to the logical format expected by
  # `surfarray.blit_array` will be done via these steps:
  #
  # 1. Get a 1d numpy array that references the bytes of the screen buffer, in row-major order
  screen_buffer_byte_count = width * height * 4
  screen_buffer_bytes = np.frombuffer(memory.get_buffer_ptr(caller, size=screen_buffer_byte_count, offset=screen_buffer_offset), dtype=np.uint8)
  #
  # 2. Logically rearrange the 1d array of contiguous bytes into a 3d array, while going from row-major to column-major
  #    (many thanks to `einops` (https://einops.rocks/) for making this step so easy!)
  pixels_in_bgra = einops.rearrange(screen_buffer_bytes, "(row col pixel) -> col row pixel", row=height, col=width)
  #
  # 3. Drop the alpha component from every pixel, which is the last component
  pixels_in_bgr = pixels_in_bgra[...,:-1]
  #
  # 4. Reverse the order of components for every pixel, so the color components are ordered (R,G,B) instead of (B,G,R)
  pixels_in_rbg = pixels_in_bgr[...,::-1]

  pg.surfarray.blit_array(pg.display.get_surface(), pixels_in_rbg)
  pg.display.flip()


def _path_to_save_game(save_game_id: int) -> str:
  """Constructs path to save game file for a given save game id"""
  # All save games will be placed in a local 'savegame' directory
  return f'.savegame/doomsav{save_game_id}.dsg'


def gameSaving__sizeOfSaveGame(game_save_id: int) -> int:
  """Report the size, in bytes, of a specific save game

  Args:
      game_save_id (int): identifies a specific save game

  Returns:
      int: Number of bytes in the associated save game data.
        Returns 0 if no save game data exists for this `gameSaveId`.
  """
  try:
    return os.path.getsize(_path_to_save_game(game_save_id))
  except OSError as err:
    return 0


def gameSaving__readSaveGame(caller: Caller, game_save_id: int, data_destination_offset: int) -> int:
  """Copy, to memory exported by the Doom WebAssembly module, the data for a specific save game

  Args:
      caller (Caller): Wasmtime caller, provides access to exported memory
      game_save_id (int): identifies a specific save game
      data_destination_offset (int):
        byte index into Doom exported memory where the bytes of the save game
        should be copied. It's guaranteed that at least `X` bytes are reserved
        in memory to hold this save game data, where `X` is the value last
        returned when `gameSaving_sizeOfSaveGame` was called with the same
        `gameSaveId`.

  Returns:
      int: Number of bytes of data game data actually copied.
  """
  with open(_path_to_save_game(game_save_id), mode='rb') as game_save:
    data = game_save.read()
    return caller.get("memory").write(caller, data, data_destination_offset)


def gameSaving__writeSaveGame(caller: Caller, game_save_id: int, data_offset: int, length: int) -> int:
  """Respond to the user attempting to save their game

  Args:
      caller (Caller): Wasmtime caller, provides access to exported memory
      game_save_id (int): identifies a specific save game
      data_offset (int):
        byte index into Doom exported memory where the bytes of the save game
        begin
      length (int): the length, in bytes, of the save game data.

  Returns:
      int: Number of bytes of data game data actually persisted
  """
  save_game_file = _path_to_save_game(game_save_id)
  # Ensure any parent folders of a save game already exist
  os.makedirs(os.path.dirname(save_game_file), exist_ok=True)

  save_game_data = caller.get("memory").read(caller, data_offset, data_offset + length)
  with open(save_game_file, mode='wb+') as game_save:
    return game_save.write(save_game_data)


def console__onInfoMessage(caller: Caller, message_offset: int, length: int) -> None:
  """Respond to Doom reporting an info message

  Args:
      caller (Caller): Wasmtime caller, provides access to exported memory
      message_offset (int):
        byte index into Doom exported memory where the first `char` of the
        message resides, the remaining characters of the message appear
        sequentially after.
      length (int): length, in bytes, of the message
  """
  message = _read_string_from_memory(caller, message_offset, length)
  print(message, file=sys.stdout)


def console__onErrorMessage(caller: Caller, message_offset: int, length: int) -> None:
  """Respond to Doom reporting an error message

  Args:
      caller (Caller): Wasmtime caller, provides access to exported memory
      message_offset (int):
        byte index into Doom exported memory where the first `char` of the
        message resides, the remaining characters of the message appear
        sequentially after.
      length (int): length, in bytes, of the message
  """
  message = _read_string_from_memory(caller, message_offset, length)
  print(message, file=sys.stderr)


#################################################################
#
#                        Main Game Loop
#
#################################################################


def main(path_to_doom_wasm, paths_to_wad_files_from_user):
  # Cache the provided paths to WAD files, so these can be accessed
  # by implementations of the functions that Doom imports
  global paths_to_wad_files
  paths_to_wad_files = paths_to_wad_files_from_user

  store = Store()
  module = Module.from_file(store.engine, path_to_doom_wasm)

  i32 = ValType.i32()
  i64 = ValType.i64()

  # List all the implementations of the functions imported by Doom,
  # and for each one describe its signature via Wasmtime's FuncType
  func_type_for_import_impl = {
    console__onErrorMessage: FuncType([i32, i32], []),
    console__onInfoMessage: FuncType([i32, i32], []),
    gameSaving__writeSaveGame: FuncType([i32, i32, i32], [i32]),
    gameSaving__readSaveGame: FuncType([i32, i32], [i32]),
    gameSaving__sizeOfSaveGame: FuncType([i32], [i32]),
    runtimeControl__timeInMilliseconds: FuncType([], [i64]),
    ui__drawFrame: FuncType([i32], []),
    loading__readWads: FuncType([i32, i32], []),
    loading__wadSizes: FuncType([i32, i32], []),
    loading__onGameInit: FuncType([i32, i32], []),
  }

  # Provide a mapping from function name to implementation of an import
  # (e.g. 'ui__drawFrame' will map to ui__drawFrame)
  import_impl_for_func_name = { import_impl.__name__ : import_impl for import_impl in func_type_for_import_impl }

  # Which function matches up with which imports needed by the WebAssembly
  # module is determined entirely by the order of the functions provided
  # as imports when an Instance in created below.
  #
  # For that reason we need to exactly order our import implementations so
  # they match up with the imports they intend to implement.
  #
  # This is straightforward because of the function naming standard we've followed:
  #   - the implementation of "module.name" import is a function named "module__name"
  ordered_import_impls =  [
    import_impl_for_func_name[f'{an_import.module}__{an_import.name}']
    for an_import in module.imports
  ]

  # When an import implementation is provided we need to tell Wasmtime when or not to
  # pass the Wasmtime Caller to the import when the import is called.
  #
  # We'll assume that the imports that receive a Caller as their first arg are exactly
  # those imports that have a first arg named 'caller'.
  def has_first_arg_named_caller(func) -> bool:
    import inspect
    name_of_first_arg = next(iter(inspect.signature(func).parameters), None)
    return name_of_first_arg == 'caller'

  # Now we can properly construct a correctly ordered and typed list of imports to
  # provide the WebAssembly module
  imports = [
    Func(store, func_type_for_import_impl[import_impl], import_impl, access_caller=has_first_arg_named_caller(import_impl))
    for import_impl in ordered_import_impls
  ]

  instance = Instance(store, module, imports)

  init_game = instance.exports(store)["initGame"]
  tick_game = instance.exports(store)["tickGame"]
  report_key_down = instance.exports(store)["reportKeyDown"]
  report_key_up = instance.exports(store)["reportKeyUp"]

  init_game(store)

  # Main loop
  while True:
    tick_game(store)

    pending_events = pg.event.get()
    for event in pending_events:
      if event.type == pg.QUIT:
        pg.quit()
        raise SystemExit
      elif event.type in [pg.KEYUP, pg.KEYDOWN]:
        doom_key = None
        # We've associated some PyGame keys, semantically, to labels for
        # doom keys (e.g. pg.K_SPACE maps to "KEY_USE"), and we'll check
        # for such a case here.
        if event.key in _doom_key_label_from_pg_key:
          # In the case that we find doom key label associated
          # with the pressed/unpressed PyGame key, the doom key value
          # we use will be retrieved from the globals exports by Doom
          doom_key_label = _doom_key_label_from_pg_key[event.key]
          doom_key = instance.exports(store)[doom_key_label].value(store)
        elif len(event.unicode) > 0:
          # Otherwise the doom key for a given PyGame key is the unicode value
          # representing the unmodified character that would be generated by
          # pressing the key
          doom_key = ord(event.unicode)

        if doom_key is not None:
          if event.type == pg.KEYDOWN:
            report_key_down(store, doom_key)
          else:
            report_key_up(store, doom_key)


if __name__ == "__main__":
  parser = argparse.ArgumentParser(
    description='Run DOOM inside a WebAssembly runtime!')

  parser.add_argument('path_to_doom_wasm', help='Path to the WebAssembly module that contains the logic of Doom')
  parser.add_argument('--wads', nargs='*', default=[], help='Optional paths to Doom WAD files; these will be loaded in the order specified')

  args = parser.parse_args()

  main(args.path_to_doom_wasm, args.wads)
