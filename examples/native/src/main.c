#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <wasm.h>
#include <wasmtime.h>

#include "doom_imports.h"
#include "user_input.h"
#include "wrapped_func.h"

#define ARRAY_LENGTH(array) (sizeof(array) / sizeof(*array))

struct state_for_reading_global_constants {
  wasmtime_context_t *context;
  const wasmtime_instance_t *instance;
};

static int32_t read_global_constant_i32_using_state(
    state_for_reading_global_constants_t *state, const char *nameOfConstant) {
  assert(state != NULL && state->context != NULL && state->instance != NULL);

  wasmtime_extern_t exportedGlobal;
  bool ok = wasmtime_instance_export_get(state->context, state->instance,
                                         nameOfConstant, strlen(nameOfConstant),
                                         &exportedGlobal);
  assert(ok);
  assert(exportedGlobal.kind == WASMTIME_EXTERN_GLOBAL);

  // Ensure that this global value is constant (i.e. not mutable).
  wasm_globaltype_t *globaltype =
      wasmtime_global_type(state->context, &exportedGlobal.of.global);
  assert(false == wasm_globaltype_mutability(globaltype));
  wasm_globaltype_delete(globaltype);

  wasmtime_val_t val;
  wasmtime_global_get(state->context, &exportedGlobal.of.global, &val);
  assert(val.kind == WASMTIME_I32);

  int32_t value = val.of.i32;

  wasmtime_val_unroot(state->context, &val);
  wasmtime_extern_delete(&exportedGlobal);

  return value;
}

// TODO: FIX THIS, these pointer types are not officially compatible: see
// https://stackoverflow.com/a/559671

static wasm_trap_t *
call_wrapped_func_stored_in_env(void *env, wasmtime_caller_t *caller,
                                const wasmtime_val_t *args, size_t nargs,
                                wasmtime_val_t *results, size_t nresults) {
  return wrapped_func_call((wrapped_func_t *)env, caller, args, nargs, results,
                           nresults);
}

static void delete_wrapped_func_stored_in_env(void *env) {
  return wrapped_func_delete((wrapped_func_t *)env);
}

static void exit_with_error(const char *message, wasmtime_error_t *error,
                            wasm_trap_t *trap);

static void call_func__void__return_void(wasmtime_context_t *context,
                                         const wasmtime_func_t *func) {
  wasm_trap_t *trap = NULL;
  wasmtime_error_t *error =
      wasmtime_func_call(context, func, NULL, 0, NULL, 0, &trap);
  if (error != NULL || trap != NULL) {
    exit_with_error("Failed to call function", error, trap);
  }
}

static void call_func__i32__return_void(wasmtime_context_t *context,
                                        const wasmtime_func_t *func,
                                        int32_t arg0) {
  wasmtime_val_t params[1];
  params[0].kind = WASMTIME_I32;
  params[0].of.i32 = arg0;

  wasm_trap_t *trap = NULL;
  wasmtime_error_t *error = error = wasmtime_func_call(
      context, func, params, ARRAY_LENGTH(params), NULL, 0, &trap);
  if (error != NULL || trap != NULL) {
    exit_with_error("Failed to call function", error, trap);
  }
}

// Caller takes ownership of the returned `wasmtime_extern_t`
wasmtime_extern_t retrieve_exported_function(wasmtime_context_t *context,
                                             wasmtime_instance_t *instance,
                                             const char *functionName) {
  wasmtime_extern_t exportedItem;
  bool ok = wasmtime_instance_export_get(context, instance, functionName,
                                         strlen(functionName), &exportedItem);
  assert(ok);
  assert(exportedItem.kind == WASMTIME_EXTERN_FUNC);
  return exportedItem;
}

wasmtime_error_t *register_all_needed_imports(wasmtime_linker_t *linker) {
  struct {
    const char *module;
    const char *name;
    wrapped_func_t *wrapped_func;
  } imported_funcs[] = {
      {"console", "onErrorMessage",
       wrapped_func_new__i32_i32_memory__return_void(console_onErrorMessage)},
      {"console", "onInfoMessage",
       wrapped_func_new__i32_i32_memory__return_void(console_onInfoMessage)},
      {"gameSaving", "readSaveGame",
       wrapped_func_new__i32_i32_memory__return_i32(gameSaving_readSaveGame)},
      {"gameSaving", "sizeOfSaveGame",
       wrapped_func_new__i32__return_i32(gameSaving_sizeOfSaveGame)},
      {"gameSaving", "writeSaveGame",
       wrapped_func_new__i32_i32_i32_memory__return_i32(
           gameSaving_writeSaveGame)},
      {"loading", "onGameInit",
       wrapped_func_new__i32_i32__return_void(loading_onGameInit)},
      {"loading", "readWads",
       wrapped_func_new__i32_i32_memory__return_void(loading_readWads)},
      {"loading", "wadSizes",
       wrapped_func_new__i32_i32__return_void(loading_wadSizes)},
      {"runtimeControl", "timeInMilliseconds",
       wrapped_func_new__void__return_i64(runtimeControl_timeInMilliseconds)},
      {"ui", "drawFrame",
       wrapped_func_new__i32_memory__return_void(ui_drawFrame)},
  };

  for (int i = 0; i < ARRAY_LENGTH(imported_funcs); i++) {
    wasmtime_error_t *error = wasmtime_linker_define_func(
        linker, imported_funcs[i].module, strlen(imported_funcs[i].module),
        imported_funcs[i].name, strlen(imported_funcs[i].name),
        wrapped_func_functype(imported_funcs[i].wrapped_func),
        call_wrapped_func_stored_in_env, imported_funcs[i].wrapped_func,
        delete_wrapped_func_stored_in_env);

    if (error != NULL) {
      return error;
    }
  }

  return NULL;
}

int main(int argc, char **argv) {

  if (argc < 2) {
    printf("Usage: %s <path-to-Doom-WebAssembly-module>\n", argv[0]);
    return 1;
  }

  const char *pathToDoomWasmModule = argv[1];

  // Read the Doom WebAssembly module
  FILE *file = fopen(pathToDoomWasmModule, "rb");
  assert(file != NULL);
  fseek(file, 0L, SEEK_END);
  size_t file_size = ftell(file);
  fseek(file, 0L, SEEK_SET);
  wasm_byte_vec_t wasm;
  wasm_byte_vec_new_uninitialized(&wasm, file_size);
  if (fread(wasm.data, file_size, 1, file) != 1) {
    printf("Error loading Doom Web Assembly module at `%s`!\n",
           pathToDoomWasmModule);
    return 1;
  }
  fclose(file);

  // Create the WASM engine, linker, and store we'll need to instantiate and
  // interact with the WebAssembly module.
  printf("Initializing core WebAssembly environment...\n");
  wasm_engine_t *engine = wasm_engine_new();
  assert(engine != NULL);
  wasmtime_linker_t *linker = wasmtime_linker_new(engine);
  wasmtime_store_t *store = wasmtime_store_new(engine, NULL, NULL);
  assert(linker != NULL && store != NULL);

  printf("Compiling WebAssembly module...\n");
  wasmtime_module_t *module = NULL;
  wasmtime_error_t *error =
      wasmtime_module_new(engine, (uint8_t *)wasm.data, wasm.size, &module);
  wasm_byte_vec_delete(&wasm);
  if (error != NULL) {
    exit_with_error("failed to compile module", error, NULL);
  }

  printf("Register all needed imports with the linker...\n");
  error = register_all_needed_imports(linker);
  if (error != NULL) {
    exit_with_error("failed to register all imports to linker", error, NULL);
  }

  // With our callback function we can now instantiate the compiled module,
  // giving us an instance we can then execute exports from. Note that
  // instantiation can trap due to execution of the `start` function, so we need
  // to handle that here too.
  printf("Instantiating module...\n");
  wasm_trap_t *trap = NULL;
  wasmtime_context_t *context = wasmtime_store_context(store);
  wasmtime_instance_t instance;
  error =
      wasmtime_linker_instantiate(linker, context, module, &instance, &trap);
  if (error != NULL || trap != NULL) {
    exit_with_error("failed to instantiate", error, trap);
  }

  // Lookup all exported functions
  wasmtime_extern_t initGame =
      retrieve_exported_function(context, &instance, "initGame");
  wasmtime_extern_t tickGame =
      retrieve_exported_function(context, &instance, "tickGame");
  wasmtime_extern_t reportKeyUp =
      retrieve_exported_function(context, &instance, "reportKeyUp");
  wasmtime_extern_t reportKeyDown =
      retrieve_exported_function(context, &instance, "reportKeyDown");

  printf("Calling initGame...\n");
  call_func__void__return_void(context, &initGame.of.func);

  bool exitDetected = false;
  while (!exitDetected) {
    call_func__void__return_void(context, &tickGame.of.func);

    input_event_t event;

    state_for_reading_global_constants_t state = {context, &instance};
    while (
        pollUserInput(&event, &state, read_global_constant_i32_using_state)) {

      if (event.eventType == EXIT) {
        exitDetected = true;
        break;
      } else if (event.eventType == KEY_STATE_CHANGE) {
        const wasmtime_func_t *keyEventFunc =
            (event.keyStateChange.newState == KEY_IS_DOWN)
                ? &reportKeyDown.of.func
                : &reportKeyUp.of.func;
        call_func__i32__return_void(context, keyEventFunc,
                                    event.keyStateChange.doomKey);
      }
    }
  }

  // Clean up after ourselves at this point, deleting everything we created, in
  // reverse order
  printf("About to exit, but first some cleanup!\n");

  wasmtime_extern_delete(&reportKeyDown);
  wasmtime_extern_delete(&reportKeyUp);
  wasmtime_extern_delete(&tickGame);
  wasmtime_extern_delete(&initGame);
  // Hmmm, there doesn't seem to be a way to 'delete' a wasmtime_instance_t. Is
  // that right?
  wasmtime_module_delete(module);
  wasmtime_linker_delete(linker);
  wasmtime_store_delete(store);
  wasm_engine_delete(engine);

  return 0;
}

static void exit_with_error(const char *message, wasmtime_error_t *error,
                            wasm_trap_t *trap) {
  fprintf(stderr, "error: %s\n", message);
  wasm_byte_vec_t error_message;
  if (error != NULL) {
    wasmtime_error_message(error, &error_message);
    wasmtime_error_delete(error);
  } else {
    wasm_trap_message(trap, &error_message);
    wasm_trap_delete(trap);
  }
  fprintf(stderr, "%.*s\n", (int)error_message.size, error_message.data);
  wasm_byte_vec_delete(&error_message);
  exit(1);
}
