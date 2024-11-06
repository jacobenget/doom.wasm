
#ifndef DOOM_EXPORTS_H_
#define DOOM_EXPORTS_H_

#include <stddef.h>
#include <stdint.h>

/*
  This file describes a generic interface to a runnable instance of the Doom
  WebAssembly module.

  This interface is agnostic to whatever WebAssembly interpreter is used in the
  underlying implementation.
*/

typedef struct doom_module_config {
  char **pathsToWadFiles;
  int32_t numberOfWadFiles;
} doom_module_config_t;

// Any error in the behavior of this module is surfaced as an instance of
// doom_module_error_t
typedef struct doom_module_error {
  const char *message;
} doom_module_error_t;

// Create a new instance of doom_module_error_t*.
// Caller receives ownership of the returned error.
doom_module_error_t *doom_module_error_new(const char *format, ...);

// Any instance of doom_module_error_t should be deleted by the owner by calling
// `doom_module_error_delete`
void doom_module_error_delete(doom_module_error_t *error);

////////////////////////////////////////////////////////////////
//
// doom_module_instance_t
//
// Represents an instance of the Doom WebAssembly module.
//
////////////////////////////////////////////////////////////////

typedef struct doom_module_instance doom_module_instance_t;

// Creates a new instance of the Doom WebAssembly module.
//
// It is the caller's responsibility to ensure that the `config` provided will
// outlive the created `doom_module_instance_t`.
//
// If the creation of the instance is successful then NULL is returned and the
// caller receives ownership of the produced `doom_module_instance_t`. If there
// was an error then a non-NULL error is returned, which the caller receives
// ownership of.
doom_module_error_t *doom_module_instance_new(const char *pathToWasmModule,
                                              doom_module_config_t *config,
                                              doom_module_instance_t **out);

void doom_module_instance_delete(doom_module_instance_t *instance);

//////////////////////////////////////////////////////////////////////////////
//
// doom_module_context_t
//
// Represents the context needed whenever interacting with an instance of the
// Doom WebAssembly module.
//
//////////////////////////////////////////////////////////////////////////////

typedef struct doom_module_context doom_module_context_t;

// This returned context is owned by the `doom_module_instance_t`.
doom_module_context_t *
doom_module_instance_context(doom_module_instance_t *instance);

doom_module_config_t *
doom_module_context_config(doom_module_context_t *context);

//////////////////////////////////////////////////////////////////////////////
//
// memory_reference_t
//
// Access to the `memory` exported by the Doom WebAssembly module instance is
// provided via `memory_reference_t` instances.
//
//////////////////////////////////////////////////////////////////////////////

typedef struct memory_reference memory_reference_t;

// The caller receives ownership of the returned `memory_reference_t`, which
// must be relinquished later through a call to `memory_reference_delete`.
//
// The returned `memory_reference_t` is only as long as the provided `context`
// is valid, so instance of `memory_reference_t` should be used and then
// relinquished quickly.
memory_reference_t *memory_reference_new(doom_module_context_t *context);

uint8_t *memory_reference_data(memory_reference_t *ref);

void memory_reference_delete(memory_reference_t *ref);

//////////////////////////////////////////////////////////////////////////////
//
// Hooks to access any of the game-specific functions and globals exported by
// the Doom WebAssembly module
//
//////////////////////////////////////////////////////////////////////////////

doom_module_error_t *initGame(doom_module_context_t *context);
doom_module_error_t *tickGame(doom_module_context_t *context);
doom_module_error_t *reportKeyDown(doom_module_context_t *context,
                                   int32_t doomKey);
doom_module_error_t *reportKeyUp(doom_module_context_t *context,
                                 int32_t doomKey);

// The 32-bit values accepted by `reportKeyDown` and `reportKeyUp` are one of
// these types:
//  1. The ASCII code for the printable character associated with the key (e.g.
//  the value 49 for the numerical 1 key)
//  2. A value associated with some semantic meaning
//
// Those values of type (2) are associated with their semantic meaning via
// global constants exported from the Doom WebAssembly module. For instance, the
// module exports a global constant named "KEY_FIRE" with the value of 163. This
// indicates that `reportKeyDown` and `reportKeyUp` should be sent the value 163
// when the user presses or releases a key associated with the FIRE action,
// respectively.
//
// The enum DoomKeyLabel and function `doomKeyForLabel` below exist too aid in
// the translation from keys with semantic meaning to 32-bit key values that
// should be passed to `reportKeyDown` and `reportKeyUp`.
//
// The entries in `enum DoomKeyLabel` are exactly those keys with semantic
// meaning, and the 32-bit 'key' associated with each of these semantic keys can
// be retrieved by calling `doomKeyForLabel.`

enum DoomKeyLabel {
  KEY_LEFTARROW,
  KEY_RIGHTARROW,
  KEY_UPARROW,
  KEY_DOWNARROW,
  KEY_STRAFE_L,
  KEY_STRAFE_R,
  KEY_FIRE,
  KEY_USE,
  KEY_SHIFT,
  KEY_TAB,
  KEY_ESCAPE,
  KEY_ENTER,
  KEY_BACKSPACE,
  KEY_ALT,
};

// Converts an enum DoomKeyLabel value into a doom key value (E.g. converts
// KEY_FIRE to 163)
doom_module_error_t *doomKeyForLabel(doom_module_context_t *context,
                                     enum DoomKeyLabel keyLabel, int32_t *out);

#endif
