#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <wasm.h>
#include <wasmtime.h>
#include <stdarg.h>
#include <inttypes.h>

#include <doom_imports.h>
#include <doom_exports.h>
#include <doom_utils.h>

#include "wrapped_func.h"

/*
  This file uses the Wasmtime WebAssembly interpreter to implement all functions
  in doom_exports.h
*/

struct doom_module_instance {
  doom_module_context_t *context;
  wasm_engine_t *engine;
  wasmtime_linker_t *linker;
  wasmtime_store_t *store;
  wasmtime_module_t *module;
  wasmtime_instance_t instance;
};

// Define a function type which generically allows the retrieval, by name, of an
// item exported from a module.
//
// A generic approach is useful because, with Wasmtime, how an module-exported
// item is retrieved is different based on where the code doing the retrieval is
// running:
//
//   1. In code running inside of an imported function called by Wasmtime
//      - You retrieve an export by calling `wasmtime_caller_export_get` with an
//      instance of `wasmtime_caller_t`
//
//   2. In code NOT running inside of an imported function called by Wasmtime
//      - You retrieve an export by calling `wasmtime_instance_export_get` with
//      an instance of `wasmtime_instance_t`
//
// The differences between these two approaches can be hidden behind an
// implementation of `export_getter` and hidden in code that specifies the `env`
// value that's passed to this function.
//
// By hiding these details the code that actually triggers the retrieval of an
// export doesn't need to know which of the two types of code mentioned above it
// is.
typedef bool export_getter(void *env, wasmtime_context_t *context,
                           const char *name, size_t name_len,
                           wasmtime_extern_t *item);

struct doom_module_context {
  doom_module_config_t *config;
  wasmtime_context_t *wasm_context;
  void *env_for_export_get;
  export_getter *export_get;
};

// Implementation of `export_getter` for when the export is retrieved via a
// `wasmtime_caller_t` (case 1 above).
static bool export_get_via_wasmtime_caller(void *env,
                                           wasmtime_context_t *context,
                                           const char *name, size_t name_len,
                                           wasmtime_extern_t *item) {
  wasmtime_caller_t *caller = (wasmtime_caller_t *)env;
  return wasmtime_caller_export_get(caller, name, name_len, item);
}

// Implementation of `export_getter` for when the export is retrieved via a
// `wasmtime_instance_t` (case 2 above).
static bool export_get_via_wasmtime_instance(void *env,
                                             wasmtime_context_t *context,
                                             const char *name, size_t name_len,
                                             wasmtime_extern_t *item) {
  wasmtime_instance_t *instance = (wasmtime_instance_t *)env;
  return wasmtime_instance_export_get(context, instance, name, name_len, item);
}

doom_module_error_t *doom_module_error_new(const char *format, ...) {
  va_list args;
  va_start(args, format);

  char *message = vsprintf_with_malloc(format, args);

  va_end(args);

  doom_module_error_t *result = malloc(sizeof(doom_module_error_t));
  result->message = message;
  return result;
}

// Allow creation of a doom_module_error_t from:
//  - A possibly NULL `wasmtime_error_t*`
//  - A possibly NULL `wasm_trap_t*`
//  - A non-NULL `doom_module_error_t*`, which this function will consume
//
// This function consumes any non-NULL `wasmtime_error_t*` or `wasm_trap_t*`
// provided.
static doom_module_error_t *
doom_module_error_new_with_context(wasmtime_error_t *error, wasm_trap_t *trap,
                                   doom_module_error_t *context) {
  wasm_byte_vec_t error_message = {0};
  wasm_byte_vec_t trap_message = {0};

  if (error != NULL) {
    wasmtime_error_message(error, &error_message);
    wasmtime_error_delete(error);
  }

  if (trap != NULL) {
    wasm_trap_message(trap, &trap_message);
    wasm_trap_delete(trap);
  }

  char *aggregateMessage = NULL;

  if (trap_message.data == NULL) {
    if (error_message.data == NULL) {
      aggregateMessage = sprintf_with_malloc("%s", context->message);
    } else {
      aggregateMessage = sprintf_with_malloc(
          "%s\nUnderlying Wasmtime error: %.*s", context->message,
          (int)error_message.size, error_message.data);
    }
  } else {
    if (error_message.data == NULL) {
      aggregateMessage = sprintf_with_malloc(
          "%s\nUnderlying Wasmtime trap: %.*s", context->message,
          (int)trap_message.size, trap_message.data);
    } else {
      aggregateMessage = sprintf_with_malloc(
          "%s\nUnderlying Wasmtime error: %.*s\nUnderlying Wasmtime trap: %.*s",
          context->message, (int)error_message.size, error_message.data,
          (int)trap_message.size, trap_message.data);
    }
  }

  if (error_message.data) {
    wasm_byte_vec_delete(&error_message);
  }
  if (trap_message.data) {
    wasm_byte_vec_delete(&trap_message);
  }

  doom_module_error_delete(context);

  doom_module_error_t *result = malloc(sizeof(doom_module_error_t));
  result->message = aggregateMessage;
  return result;
}

void doom_module_error_delete(doom_module_error_t *error) {
  free((char *)error->message);
  free(error);
}

// Retrieve an exported item by leveraging these two things present on an
// instance of doom_module_context_t:
//  - An implementation of `export_getter`
//  - A void* value to pass to the implementation of `export_getter`
//
// Returns NULL upon successful retrieval of the export, and an instance of
// `doom_module_error_t` otherwise.
//
// Upon success, caller accepts ownership of the resultant `wasmtime_extern_t`.
static doom_module_error_t *retrieve_export(doom_module_context_t *context,
                                            const char *name,
                                            wasmtime_extern_kind_t expectedKind,
                                            wasmtime_extern_t *item) {
  bool ok =
      context->export_get(context->env_for_export_get, context->wasm_context,
                          name, strlen(name), item);
  if (!ok) {
    return doom_module_error_new("Failed to retrieve the export `%s`", name);
  } else if (item->kind != expectedKind) {
    doom_module_error_t *error =
        doom_module_error_new("Export `%s` had the type `%" PRIu8
                              "` instead of the expected type, `%" PRIu8 "`",
                              name, item->kind, expectedKind);
    wasmtime_extern_delete(item);
    return error;
  } else {
    return NULL;
  }
}

doom_module_config_t *
doom_module_context_config(doom_module_context_t *context) {
  return context->config;
}

doom_module_context_t *
doom_module_instance_context(doom_module_instance_t *instance) {
  return instance->context;
}

// Define the data that's stored along with each registered import.
// This data provides exactly what's needed to call an implementation of that
// import.
typedef struct registered_import_env {
  wrapped_func_t *wrapped_import_impl;
  doom_module_config_t *config;
} registered_import_env_t;

// Generic callback that handles the calling of all imported functions.
// This function matches the signature defined by `wasmtime_func_callback_t`.
static wasm_trap_t *
call_wrapped_func_stored_in_env(void *env, wasmtime_caller_t *caller,
                                const wasmtime_val_t *args, size_t nargs,
                                wasmtime_val_t *results, size_t nresults) {
  registered_import_env_t *rie = (registered_import_env_t *)env;

  // Set up the needed "context" (i.e.
  // `doom_module_context_t`) to pass to the imported function as it's
  // called.
  doom_module_context_t context;
  context.config = rie->config;
  context.wasm_context = wasmtime_caller_context(caller);
  context.env_for_export_get = caller;
  context.export_get = export_get_via_wasmtime_caller;

  return wrapped_func_call(rie->wrapped_import_impl, &context, args, nargs,
                           results, nresults);
}

// Finalizer for an instance of `registered_import_env_t`.
static void delete_wrapped_func_stored_in_env(void *env) {
  registered_import_env_t *ce = (registered_import_env_t *)env;
  wrapped_func_delete(ce->wrapped_import_impl);
  free(ce);
}

// A utility for registering all imports needed by Doom, which are all declared
// in `doom_imports.h`, to a provided linker.
static wasmtime_error_t *
register_all_needed_imports(wasmtime_linker_t *linker,
                            doom_module_config_t *config) {
  struct {
    const char *module;
    const char *name;
    wrapped_func_t *wrapped_import_impl;
  } imported_funcs[] = {
      {"console", "onErrorMessage",
       wrapped_func_new__i32_i32__return_void(console_onErrorMessage)},
      {"console", "onInfoMessage",
       wrapped_func_new__i32_i32__return_void(console_onInfoMessage)},
      {"gameSaving", "readSaveGame",
       wrapped_func_new__i32_i32__return_i32(gameSaving_readSaveGame)},
      {"gameSaving", "sizeOfSaveGame",
       wrapped_func_new__i32__return_i32(gameSaving_sizeOfSaveGame)},
      {"gameSaving", "writeSaveGame",
       wrapped_func_new__i32_i32_i32__return_i32(gameSaving_writeSaveGame)},
      {"loading", "onGameInit",
       wrapped_func_new__i32_i32__return_void(loading_onGameInit)},
      {"loading", "readWads",
       wrapped_func_new__i32_i32__return_void(loading_readWads)},
      {"loading", "wadSizes",
       wrapped_func_new__i32_i32__return_void(loading_wadSizes)},
      {"runtimeControl", "timeInMilliseconds",
       wrapped_func_new__void__return_i64(runtimeControl_timeInMilliseconds)},
      {"ui", "drawFrame", wrapped_func_new__i32__return_void(ui_drawFrame)},
  };

  for (int i = 0; i < ARRAY_LENGTH(imported_funcs); i++) {
    const char *module = imported_funcs[i].module;
    const char *name = imported_funcs[i].name;
    registered_import_env_t *rie = malloc(sizeof(registered_import_env_t));
    rie->wrapped_import_impl = imported_funcs[i].wrapped_import_impl;
    rie->config = config;

    wasmtime_error_t *error = wasmtime_linker_define_func(
        linker, module, strlen(module), name, strlen(name),
        wrapped_func_functype(rie->wrapped_import_impl),
        call_wrapped_func_stored_in_env, rie,
        delete_wrapped_func_stored_in_env);

    if (error != NULL) {
      return error;
    }
  }

  return NULL;
}

// Creates a new `doom_module_instance_t`.
doom_module_error_t *doom_module_instance_new(const char *pathToWasmModule,
                                              doom_module_config_t *config,
                                              doom_module_instance_t **out) {

  doom_module_context_t *context = malloc(sizeof(doom_module_context_t));
  // Initialize entire context to 0
  *context = (doom_module_context_t){0};

  doom_module_instance_t *doom_instance =
      malloc(sizeof(doom_module_instance_t));
  // Initialize entire instance to 0
  *doom_instance = (doom_module_instance_t){0};
  doom_instance->context = context;

  // Create the WASM engine, linker, and store we'll need to instantiate and
  // interact with the WebAssembly module.
  printf("Initializing core WebAssembly environment...\n");
  doom_instance->engine = wasm_engine_new();
  if (doom_instance->engine == NULL) {
    doom_module_instance_delete(doom_instance);
    return doom_module_error_new("Failed to create WASM engine");
  }
  doom_instance->linker = wasmtime_linker_new(doom_instance->engine);
  doom_instance->store = wasmtime_store_new(doom_instance->engine, NULL, NULL);
  if (doom_instance->linker == NULL || doom_instance->store == NULL) {
    doom_module_instance_delete(doom_instance);
    return doom_module_error_new("Failed to create WASM linker or store");
  }

  // Read the Doom WebAssembly module from disk
  FILE *file = fopen(pathToWasmModule, "rb");
  if (file == NULL) {
    doom_module_instance_delete(doom_instance);
    return doom_module_error_new("Failed to open Doom WebAssembly module file");
  }
  fseek(file, 0L, SEEK_END);
  size_t file_size = ftell(file);
  fseek(file, 0L, SEEK_SET);
  wasm_byte_vec_t wasm;
  wasm_byte_vec_new_uninitialized(&wasm, file_size);
  size_t objectsRead = fread(wasm.data, file_size, 1, file);
  fclose(file);
  if (objectsRead != 1) {
    wasm_byte_vec_delete(&wasm);
    doom_module_instance_delete(doom_instance);
    return doom_module_error_new(
        "Error reading Doom WebAssembly module from disk");
  }

  printf("Compiling WebAssembly module...\n");
  wasmtime_error_t *error =
      wasmtime_module_new(doom_instance->engine, (uint8_t *)wasm.data,
                          wasm.size, &doom_instance->module);
  wasm_byte_vec_delete(&wasm);
  if (error != NULL) {
    doom_module_instance_delete(doom_instance);
    doom_module_error_t *context =
        doom_module_error_new("Failed to compile module");
    return doom_module_error_new_with_context(error, NULL, context);
  }

  printf("Registering all needed imports with the linker...\n");
  error = register_all_needed_imports(doom_instance->linker, config);
  if (error != NULL) {
    doom_module_instance_delete(doom_instance);
    doom_module_error_t *context =
        doom_module_error_new("Failed to register all imports to linker");
    return doom_module_error_new_with_context(error, NULL, context);
  }

  printf("Instantiating module...\n");
  wasm_trap_t *trap = NULL;
  wasmtime_context_t *wasm_context =
      wasmtime_store_context(doom_instance->store);
  error = wasmtime_linker_instantiate(doom_instance->linker, wasm_context,
                                      doom_instance->module,
                                      &doom_instance->instance, &trap);
  if (error != NULL || trap != NULL) {
    doom_module_instance_delete(doom_instance);
    doom_module_error_t *context =
        doom_module_error_new("Failed to instantiate the module");
    return doom_module_error_new_with_context(error, trap, context);
  }

  // Correctly set up the `context` of this instance
  context->config = config;
  context->wasm_context = wasm_context;
  context->env_for_export_get = &doom_instance->instance;
  context->export_get = export_get_via_wasmtime_instance;

  // Ensure that the module instance has all the expected exports
  struct {
    const char *name;
    wasmtime_extern_kind_t kind;
  } requiredExports[] = {
      {"initGame", WASMTIME_EXTERN_FUNC},
      {"tickGame", WASMTIME_EXTERN_FUNC},
      {"reportKeyDown", WASMTIME_EXTERN_FUNC},
      {"reportKeyUp", WASMTIME_EXTERN_FUNC},
      {"memory", WASMTIME_EXTERN_MEMORY},
      {"KEY_ALT", WASMTIME_EXTERN_GLOBAL},
      {"KEY_BACKSPACE", WASMTIME_EXTERN_GLOBAL},
      {"KEY_DOWNARROW", WASMTIME_EXTERN_GLOBAL},
      {"KEY_ENTER", WASMTIME_EXTERN_GLOBAL},
      {"KEY_ESCAPE", WASMTIME_EXTERN_GLOBAL},
      {"KEY_FIRE", WASMTIME_EXTERN_GLOBAL},
      {"KEY_LEFTARROW", WASMTIME_EXTERN_GLOBAL},
      {"KEY_RIGHTARROW", WASMTIME_EXTERN_GLOBAL},
      {"KEY_SHIFT", WASMTIME_EXTERN_GLOBAL},
      {"KEY_STRAFE_L", WASMTIME_EXTERN_GLOBAL},
      {"KEY_STRAFE_R", WASMTIME_EXTERN_GLOBAL},
      {"KEY_TAB", WASMTIME_EXTERN_GLOBAL},
      {"KEY_UPARROW", WASMTIME_EXTERN_GLOBAL},
      {"KEY_USE", WASMTIME_EXTERN_GLOBAL},
  };

  for (int i = 0; i < ARRAY_LENGTH(requiredExports); i++) {
    wasmtime_extern_t export;
    doom_module_error_t *error = retrieve_export(
        context, requiredExports[i].name, requiredExports[i].kind, &export);
    if (error)
      return error;
    wasmtime_extern_delete(&export);
  }

  *out = doom_instance;
  return NULL;
}

void doom_module_instance_delete(doom_module_instance_t *instance) {
  // Note: There is no destructor associated with wasmtime_instance_t

  if (instance->module) {
    wasmtime_module_delete(instance->module);
  }
  if (instance->linker) {
    wasmtime_linker_delete(instance->linker);
  }
  if (instance->store) {
    wasmtime_store_delete(instance->store);
  }
  if (instance->engine) {
    wasm_engine_delete(instance->engine);
  }

  free(instance->context);
  free(instance);
}

// Define a few utilities to aid in the calling, by name, of an exported
// function.
//
// The names of these function follow the pattern
// `call_exported_func__X__return_Y`, where:
//  - X represents the input types, in order, accepted by the exported function.
//  - Y represents the output types, in order, produced by the exported
//  function.

static doom_module_error_t *
call_exported_func__void__return_void(doom_module_context_t *context,
                                      const char *name) {
  wasmtime_extern_t exportedFunction;
  doom_module_error_t *error =
      retrieve_export(context, name, WASMTIME_EXTERN_FUNC, &exportedFunction);
  if (!error) {
    wasm_trap_t *trap = NULL;
    wasmtime_error_t *wasmtime_error =
        wasmtime_func_call(context->wasm_context, &exportedFunction.of.func,
                           NULL, 0, NULL, 0, &trap);
    wasmtime_extern_delete(&exportedFunction);
    if (wasmtime_error || trap) {
      doom_module_error_t *context =
          doom_module_error_new("Error while calling function `%s`", name);
      error = doom_module_error_new_with_context(wasmtime_error, trap, context);
    }
  }

  return error;
}

static doom_module_error_t *
call_exported_func__i32__return_void(doom_module_context_t *context,
                                     const char *name, int32_t arg0) {
  wasmtime_val_t params[1];
  params[0].kind = WASMTIME_I32;
  params[0].of.i32 = arg0;

  wasmtime_extern_t exportedFunction;
  doom_module_error_t *error =
      retrieve_export(context, name, WASMTIME_EXTERN_FUNC, &exportedFunction);
  if (!error) {
    wasm_trap_t *trap = NULL;
    wasmtime_error_t *wasmtime_error =
        wasmtime_func_call(context->wasm_context, &exportedFunction.of.func,
                           params, ARRAY_LENGTH(params), NULL, 0, &trap);
    wasmtime_extern_delete(&exportedFunction);
    if (wasmtime_error || trap) {
      doom_module_error_t *context =
          doom_module_error_new("Error while calling function `%s`", name);
      error = doom_module_error_new_with_context(wasmtime_error, trap, context);
    }
  }

  return error;
}

// Define hooks to call any of the functions exported by the Doom WebAssembly
// module

doom_module_error_t *initGame(doom_module_context_t *context) {
  return call_exported_func__void__return_void(context, "initGame");
}

doom_module_error_t *tickGame(doom_module_context_t *context) {
  return call_exported_func__void__return_void(context, "tickGame");
}

doom_module_error_t *reportKeyDown(doom_module_context_t *context,
                                   int32_t doomKey) {
  return call_exported_func__i32__return_void(context, "reportKeyDown",
                                              doomKey);
}

doom_module_error_t *reportKeyUp(doom_module_context_t *context,
                                 int32_t doomKey) {
  return call_exported_func__i32__return_void(context, "reportKeyUp", doomKey);
}

struct memory_reference {
  wasmtime_extern_t exportedMemory;
  wasmtime_context_t *wasm_context;
};

memory_reference_t *memory_reference_new(doom_module_context_t *context) {
  memory_reference_t *ref = malloc(sizeof(memory_reference_t));

  doom_module_error_t *error = retrieve_export(
      context, "memory", WASMTIME_EXTERN_MEMORY, &ref->exportedMemory);
  // Due to checks made when the related wasmtime_instance_t was created,
  // we are confident that the retrieval 'memory' should have been successful,
  // and so we just `assert` here.
  assert(!error);

  ref->wasm_context = context->wasm_context;

  return ref;
}

uint8_t *memory_reference_data(memory_reference_t *ref) {
  return wasmtime_memory_data(ref->wasm_context,
                              &ref->exportedMemory.of.memory);
}

void memory_reference_delete(memory_reference_t *ref) {
  wasmtime_extern_delete(&ref->exportedMemory);
  free(ref);
}

#define CASE__RETURN_VALUE_AS_STRING(x)                                        \
  case x:                                                                      \
    return #x

// Converts an enum DoomKeyLabel value to a string that matches its name.
// E.g. The value KEY_FIRE is converted to "KEY_FIRE".
//
// Returns NULL in the case that the provided value isn't recognized.
static const char *nameForDoomKeyLabel(enum DoomKeyLabel keyLabel) {
  switch (keyLabel) {
    CASE__RETURN_VALUE_AS_STRING(KEY_ALT);
    CASE__RETURN_VALUE_AS_STRING(KEY_BACKSPACE);
    CASE__RETURN_VALUE_AS_STRING(KEY_DOWNARROW);
    CASE__RETURN_VALUE_AS_STRING(KEY_ENTER);
    CASE__RETURN_VALUE_AS_STRING(KEY_ESCAPE);
    CASE__RETURN_VALUE_AS_STRING(KEY_FIRE);
    CASE__RETURN_VALUE_AS_STRING(KEY_LEFTARROW);
    CASE__RETURN_VALUE_AS_STRING(KEY_RIGHTARROW);
    CASE__RETURN_VALUE_AS_STRING(KEY_SHIFT);
    CASE__RETURN_VALUE_AS_STRING(KEY_STRAFE_L);
    CASE__RETURN_VALUE_AS_STRING(KEY_STRAFE_R);
    CASE__RETURN_VALUE_AS_STRING(KEY_TAB);
    CASE__RETURN_VALUE_AS_STRING(KEY_UPARROW);
    CASE__RETURN_VALUE_AS_STRING(KEY_USE);
  default:
    return NULL;
  }
}

doom_module_error_t *doomKeyForLabel(doom_module_context_t *context,
                                     enum DoomKeyLabel keyLabel, int32_t *out) {
  const char *name = nameForDoomKeyLabel(keyLabel);
  if (name == NULL) {
    return doom_module_error_new(
        "DoomKeyLabel value `%d` has no associated name", keyLabel);
  }

  wasmtime_extern_t exportedGlobal;
  doom_module_error_t *error =
      retrieve_export(context, name, WASMTIME_EXTERN_GLOBAL, &exportedGlobal);
  if (!error) {
    wasmtime_val_t val;
    wasmtime_global_get(context->wasm_context, &exportedGlobal.of.global, &val);

    if (val.kind == WASMTIME_I32) {
      *out = val.of.i32;
    } else {
      error = doom_module_error_new(
          "Doom key value was not a i32 value, instead it was kind `%" PRIu8
          "`",
          val.kind);
    }

    wasmtime_val_unroot(context->wasm_context, &val);
    wasmtime_extern_delete(&exportedGlobal);
  }

  return error;
}
