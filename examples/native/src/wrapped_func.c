#include <stdlib.h>

#include "wrapped_func.h"

typedef void (*func_ptr)(void);

struct wrapped_func {
  wasm_functype_t *func_type;
  func_ptr func;
  bool func_receives_exported_memory;
  void (*call_func_unchecked)(func_ptr func, const wasmtime_val_t *args,
                              wasmtime_val_t *results, uint8_t *memory);
};

// This function aborts if there's a mismatch between any of these details of
// wrappedFunc and the info provided by caller:
//  1. Number of args
//  2. Type of each arg
//  3. Number of result values
wasm_trap_t *wrapped_func_call(wrapped_func_t *wrappedFunc,
                               wasmtime_caller_t *caller,
                               const wasmtime_val_t *args, size_t nargs,
                               wasmtime_val_t *results, size_t nresults) {

  // Ensure that the number of args provided and the type of each of them
  // completely match what is expected by the underlying function.
  const wasm_valtype_vec_t *expectedParams =
      wasm_functype_params(wrappedFunc->func_type);
  if (expectedParams->size != nargs) {
    const char *errorMsg =
        "Number of args provided did not match the number expected!";
    return wasmtime_trap_new(errorMsg, strlen(errorMsg));
  }
  for (int i = 0; i < nargs; i++) {
    if (wasm_valtype_kind(expectedParams->data[i]) != args[i].kind) {
      const char *errorMsg = "The kind of one of the args did not match the "
                             "type expected for that arg!";
      return wasmtime_trap_new(errorMsg, strlen(errorMsg));
    }
  }

  // Ensure that the number of the results expected to be produced matches
  // the number of results that will be actually produced by the underlying
  // function.
  const wasm_valtype_vec_t *expectedResults =
      wasm_functype_results(wrappedFunc->func_type);
  if (expectedResults->size != nresults) {
    const char *errorMsg =
        "Number of return values expected to be produced did not match the "
        "number that will be produced by the underlying function!";
    return wasmtime_trap_new(errorMsg, strlen(errorMsg));
  }

  uint8_t *memory = NULL;
  wasmtime_extern_t exportedMemory;
  if (wrappedFunc->func_receives_exported_memory) {
    // Retrieve exported memory
    const char *nameOfMemory = "memory";
    bool ok = wasmtime_caller_export_get(caller, nameOfMemory,
                                         strlen(nameOfMemory), &exportedMemory);
    if (!ok) {
      const char *errorMsg = "Failure to retrieve an export named `memory`!";
      return wasmtime_trap_new(errorMsg, strlen(errorMsg));
    }
    if (exportedMemory.kind != WASMTIME_EXTERN_MEMORY) {
      const char *errorMsg =
          "The export named `memory` is not a WebAssembly memory!";
      return wasmtime_trap_new(errorMsg, strlen(errorMsg));
    }
    wasmtime_context_t *context = wasmtime_caller_context(caller);
    memory = wasmtime_memory_data(context, &exportedMemory.of.memory);
  }

  // Call the underlying function.
  (*wrappedFunc->call_func_unchecked)(wrappedFunc->func, args, results, memory);

  if (wrappedFunc->func_receives_exported_memory) {
    wasmtime_extern_delete(&exportedMemory);
  }

  return NULL;
}

wasm_functype_t *wrapped_func_functype(wrapped_func_t *wrappedFunc) {
  return wrappedFunc->func_type;
}

void wrapped_func_delete(wrapped_func_t *wrappedFunc) {
  wasm_functype_delete(wrappedFunc->func_type);
  free(wrappedFunc);
}

// Support wrapping functions with a signature of: () -> (i64)

static void unwrap_and_call__void__return_i64(func_ptr func,
                                              const wasmtime_val_t *args,
                                              wasmtime_val_t *results,
                                              uint8_t *_) {
  results[0].of.i64 = ((int64_t(*)())(func))();
  results[0].kind = WASMTIME_I64;
}

wrapped_func_t *wrapped_func_new__void__return_i64(int64_t (*func)()) {
  wrapped_func_t *result = malloc(sizeof(wrapped_func_t));
  result->func_type = wasm_functype_new_0_1(wasm_valtype_new(WASM_I64));
  result->func = (func_ptr)func;
  result->func_receives_exported_memory = false;
  result->call_func_unchecked = unwrap_and_call__void__return_i64;
  return result;
}

// Support wrapping functions with a signature of: (132) -> () Needs access to
// memory

static void unwrap_and_call__i32_memory__return_void(func_ptr func,
                                                     const wasmtime_val_t *args,
                                                     wasmtime_val_t *results,
                                                     uint8_t *memory) {
  ((void (*)(int32_t, uint8_t *))(func))(args[0].of.i32, memory);
}

wrapped_func_t *
wrapped_func_new__i32_memory__return_void(void (*func)(int32_t, uint8_t *)) {
  wrapped_func_t *result = malloc(sizeof(wrapped_func_t));
  result->func_type = wasm_functype_new_1_0(wasm_valtype_new(WASM_I32));
  result->func = (func_ptr)func;
  result->func_receives_exported_memory = true;
  result->call_func_unchecked = unwrap_and_call__i32_memory__return_void;
  return result;
}

// Support wrapping functions with a signature of: (i32) -> (i32)

static void unwrap_and_call__i32__return_i32(func_ptr func,
                                             const wasmtime_val_t *args,
                                             wasmtime_val_t *results,
                                             uint8_t *_) {
  results[0].of.i32 = ((int32_t(*)(int32_t))(func))(args[0].of.i32);
  results[0].kind = WASMTIME_I32;
}

wrapped_func_t *wrapped_func_new__i32__return_i32(int32_t (*func)(int32_t)) {
  wrapped_func_t *result = malloc(sizeof(wrapped_func_t));
  result->func_type = wasm_functype_new_1_1(wasm_valtype_new(WASM_I32),
                                            wasm_valtype_new(WASM_I32));
  result->func = (func_ptr)func;
  result->func_receives_exported_memory = false;
  result->call_func_unchecked = unwrap_and_call__i32__return_i32;
  return result;
}

// Support wrapping functions with a signature of: (i32, i32) -> ()

static void unwrap_and_call__i32_i32__return_void(func_ptr func,
                                                  const wasmtime_val_t *args,
                                                  wasmtime_val_t *results,
                                                  uint8_t *_) {
  ((void (*)(int32_t, int32_t))(func))(args[0].of.i32, args[1].of.i32);
}

wrapped_func_t *wrapped_func_new__i32_i32__return_void(void (*func)(int32_t,
                                                                    int32_t)) {
  wrapped_func_t *result = malloc(sizeof(wrapped_func_t));
  result->func_type = wasm_functype_new_2_0(wasm_valtype_new(WASM_I32),
                                            wasm_valtype_new(WASM_I32));
  result->func = (func_ptr)func;
  result->func_receives_exported_memory = false;
  result->call_func_unchecked = unwrap_and_call__i32_i32__return_void;
  return result;
}

// Support wrapping functions with a signature of: (i32, i32) -> ()  Needs
// access to memory

static void unwrap_and_call__i32_i32_memory__return_void(
    func_ptr func, const wasmtime_val_t *args, wasmtime_val_t *results,
    uint8_t *memory) {
  ((void (*)(int32_t, int32_t, uint8_t *))(func))(args[0].of.i32,
                                                  args[1].of.i32, memory);
}

wrapped_func_t *wrapped_func_new__i32_i32_memory__return_void(
    void (*func)(int32_t, int32_t, uint8_t *)) {
  wrapped_func_t *result = malloc(sizeof(wrapped_func_t));
  result->func_type = wasm_functype_new_2_0(wasm_valtype_new(WASM_I32),
                                            wasm_valtype_new(WASM_I32));
  result->func = (func_ptr)func;
  result->func_receives_exported_memory = true;
  result->call_func_unchecked = unwrap_and_call__i32_i32_memory__return_void;
  return result;
}

// Support wrapping functions with a signature of: (i32, i32) -> (i32)  Needs
// access to memory

static void unwrap_and_call__i32_i32_memory__return_i32(
    func_ptr func, const wasmtime_val_t *args, wasmtime_val_t *results,
    uint8_t *memory) {
  results[0].of.i32 = ((int32_t(*)(int32_t, int32_t, uint8_t *))(func))(
      args[0].of.i32, args[1].of.i32, memory);
  results[0].kind = WASMTIME_I32;
}

wrapped_func_t *wrapped_func_new__i32_i32_memory__return_i32(
    int32_t (*func)(int32_t, int32_t, uint8_t *)) {
  wrapped_func_t *result = malloc(sizeof(wrapped_func_t));
  result->func_type = wasm_functype_new_2_1(wasm_valtype_new(WASM_I32),
                                            wasm_valtype_new(WASM_I32),
                                            wasm_valtype_new(WASM_I32));
  result->func = (func_ptr)func;
  result->func_receives_exported_memory = true;
  result->call_func_unchecked = unwrap_and_call__i32_i32_memory__return_i32;
  return result;
}

// Support wrapping functions with a signature of: (i32, i32, i32) -> (i32)
// Needs access to memory

static void unwrap_and_call__i32_i32_i32_memory__return_i32(
    func_ptr func, const wasmtime_val_t *args, wasmtime_val_t *results,
    uint8_t *memory) {
  results[0].of.i32 = ((int32_t(*)(int32_t, int32_t, int32_t, uint8_t *))(
      func))(args[0].of.i32, args[1].of.i32, args[2].of.i32, memory);
  results[0].kind = WASMTIME_I32;
}

wrapped_func_t *wrapped_func_new__i32_i32_i32_memory__return_i32(
    int32_t (*func)(int32_t, int32_t, int32_t, uint8_t *)) {
  wrapped_func_t *result = malloc(sizeof(wrapped_func_t));
  result->func_type = wasm_functype_new_3_1(
      wasm_valtype_new(WASM_I32), wasm_valtype_new(WASM_I32),
      wasm_valtype_new(WASM_I32), wasm_valtype_new(WASM_I32));
  result->func = (func_ptr)func;
  result->func_receives_exported_memory = true;
  result->call_func_unchecked = unwrap_and_call__i32_i32_i32_memory__return_i32;
  return result;
}
