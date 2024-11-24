#include <stdlib.h>

#include "wrapped_func.h"

// NOTE: Did you know that `void*` is not guaranteed to be large enough to hold
// a function pointer? See https://stackoverflow.com/a/44130181
//
// That reason is why we're instead using `void (*)(void)` to hold a function
// pointer.
typedef void (*func_ptr)(void);

struct wrapped_func {
  wasm_functype_t *func_type;
  func_ptr func;
  void (*call_func_unchecked)(func_ptr func, doom_module_context_t *context,
                              const wasmtime_val_t *args,
                              wasmtime_val_t *results);
};

// Calls the function that's wrapped up in a `wrapped_func_t`.
//
// This function returns a `wasm_trap_t` if there's a mismatch between any of
// these details of the given `wrapped_func_t` and the info provided by caller:
//  1. Number of args
//  2. Type of each arg
//  3. Number of result values
//
// NOTE: it may be true that any of Wasmtime's use of a function that implements
// `wasmtime_func_callback_t` (which includes all functions that call this
// function here) are already only called when some or all three of the
// above details are guaranteed to match, and so such checks made by this
// function here for mismatches are superfluous. (The documentation here
// somewhat hints at this:
// https://docs.wasmtime.dev/c-api/func_8h.html#af90ff6b594f05f23e381ac16c22b70cf)
// But this detail isn't clear crystal clear from the Wasmtime documentation, so
// we're performing all three of these checks here just to be safe.
wasm_trap_t *wrapped_func_call(wrapped_func_t *wrappedFunc,
                               doom_module_context_t *context,
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

  // Call the underlying function.
  (*wrappedFunc->call_func_unchecked)(wrappedFunc->func, context, args,
                                      results);

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
                                              doom_module_context_t *context,
                                              const wasmtime_val_t *args,
                                              wasmtime_val_t *results) {
  func_ptr__void__return_i64 *cast_func = (func_ptr__void__return_i64 *)func;
  results[0].of.i64 = cast_func(context);
  results[0].kind = WASMTIME_I64;
}

wrapped_func_t *
wrapped_func_new__void__return_i64(func_ptr__void__return_i64 *func) {
  wrapped_func_t *result = malloc(sizeof(wrapped_func_t));
  result->func_type = wasm_functype_new_0_1(wasm_valtype_new(WASM_I64));
  result->func = (func_ptr)func;
  result->call_func_unchecked = unwrap_and_call__void__return_i64;
  return result;
}

// Support wrapping functions with a signature of: (132) -> ()

static void unwrap_and_call__i32__return_void(func_ptr func,
                                              doom_module_context_t *context,
                                              const wasmtime_val_t *args,
                                              wasmtime_val_t *results) {
  func_ptr__i32__return_void *cast_func = (func_ptr__i32__return_void *)func;
  cast_func(context, args[0].of.i32);
}

wrapped_func_t *
wrapped_func_new__i32__return_void(func_ptr__i32__return_void *func) {
  wrapped_func_t *result = malloc(sizeof(wrapped_func_t));
  result->func_type = wasm_functype_new_1_0(wasm_valtype_new(WASM_I32));
  result->func = (func_ptr)func;
  result->call_func_unchecked = unwrap_and_call__i32__return_void;
  return result;
}

// Support wrapping functions with a signature of: (i32) -> (i32)

static void unwrap_and_call__i32__return_i32(func_ptr func,
                                             doom_module_context_t *context,
                                             const wasmtime_val_t *args,
                                             wasmtime_val_t *results) {
  func_ptr__i32__return_i32 *cast_func = (func_ptr__i32__return_i32 *)func;
  results[0].of.i32 = cast_func(context, args[0].of.i32);
  results[0].kind = WASMTIME_I32;
}

wrapped_func_t *
wrapped_func_new__i32__return_i32(func_ptr__i32__return_i32 *func) {
  wrapped_func_t *result = malloc(sizeof(wrapped_func_t));
  result->func_type = wasm_functype_new_1_1(wasm_valtype_new(WASM_I32),
                                            wasm_valtype_new(WASM_I32));
  result->func = (func_ptr)func;
  result->call_func_unchecked = unwrap_and_call__i32__return_i32;
  return result;
}

// Support wrapping functions with a signature of: (i32, i32) -> ()

static void unwrap_and_call__i32_i32__return_void(
    func_ptr func, doom_module_context_t *context, const wasmtime_val_t *args,
    wasmtime_val_t *results) {
  func_ptr__i32_i32__return_void *cast_func =
      (func_ptr__i32_i32__return_void *)func;
  cast_func(context, args[0].of.i32, args[1].of.i32);
}

wrapped_func_t *
wrapped_func_new__i32_i32__return_void(func_ptr__i32_i32__return_void *func) {
  wrapped_func_t *result = malloc(sizeof(wrapped_func_t));
  result->func_type = wasm_functype_new_2_0(wasm_valtype_new(WASM_I32),
                                            wasm_valtype_new(WASM_I32));
  result->func = (func_ptr)func;
  result->call_func_unchecked = unwrap_and_call__i32_i32__return_void;
  return result;
}

// Support wrapping functions with a signature of: (i32, i32) -> (i32)

static void unwrap_and_call__i32_i32__return_i32(func_ptr func,
                                                 doom_module_context_t *context,
                                                 const wasmtime_val_t *args,
                                                 wasmtime_val_t *results) {
  func_ptr__i32_i32__return_i32 *cast_func =
      (func_ptr__i32_i32__return_i32 *)func;
  results[0].of.i32 = cast_func(context, args[0].of.i32, args[1].of.i32);
  results[0].kind = WASMTIME_I32;
}

wrapped_func_t *
wrapped_func_new__i32_i32__return_i32(func_ptr__i32_i32__return_i32 *func) {
  wrapped_func_t *result = malloc(sizeof(wrapped_func_t));
  result->func_type = wasm_functype_new_2_1(wasm_valtype_new(WASM_I32),
                                            wasm_valtype_new(WASM_I32),
                                            wasm_valtype_new(WASM_I32));
  result->func = (func_ptr)func;
  result->call_func_unchecked = unwrap_and_call__i32_i32__return_i32;
  return result;
}

// Support wrapping functions with a signature of: (i32, i32, i32) -> (i32)

static void unwrap_and_call__i32_i32_i32__return_i32(
    func_ptr func, doom_module_context_t *context, const wasmtime_val_t *args,
    wasmtime_val_t *results) {
  func_ptr__i32_i32_i32__return_i32 *cast_func =
      (func_ptr__i32_i32_i32__return_i32 *)func;
  results[0].of.i32 =
      cast_func(context, args[0].of.i32, args[1].of.i32, args[2].of.i32);
  results[0].kind = WASMTIME_I32;
}

wrapped_func_t *wrapped_func_new__i32_i32_i32__return_i32(
    func_ptr__i32_i32_i32__return_i32 *func) {
  wrapped_func_t *result = malloc(sizeof(wrapped_func_t));
  result->func_type = wasm_functype_new_3_1(
      wasm_valtype_new(WASM_I32), wasm_valtype_new(WASM_I32),
      wasm_valtype_new(WASM_I32), wasm_valtype_new(WASM_I32));
  result->func = (func_ptr)func;
  result->call_func_unchecked = unwrap_and_call__i32_i32_i32__return_i32;
  return result;
}
