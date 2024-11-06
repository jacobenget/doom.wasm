#ifndef FUNCTION_WRAPPER_H_
#define FUNCTION_WRAPPER_H_

#include <wasm.h>
#include <wasmtime.h>

#include <doom_exports.h>

// With the vanilla C interface provide by Wasmtime, interacting with functions
// imported into the WebAssembly module involves writing custom code for each
// import to marshall `wasmtime_val_t *` values to native C values and back.
//
// To avoid this boilerplate code being repeated, spread out, and harder to
// audit and extent, we'll provide a generic interface (`wrapped_func_t`) for
// calling a C function via Wasmtime. This approach neatly hides the boilerplate
// code in the implementation of this interface.
//
// `wrapped_func_t` generically represents a function that can be called, and
// calling this function is done via `wrapped_func_call`
typedef struct wrapped_func wrapped_func_t;

// Provide support for creating an instance of `wrapped_func_t` from a C
// function, for each of the function signatures used by a function imported by
// the Doom WebAssembly module.
//
// Each of these wrapped function, along with accepting their own custom arg
// types, will accept an instance of `doom_module_context_t` as their first arg.
// By having access to a `doom_module_context_t*` these wrapped functions have
// access to all features exported by the Doom WebAssembly, e.g. the exported
// memory.
//
// Currently each function imported by the Doom WebAssembly module has one of
// these function signatures:
//      - () -> (i64)
//      - (i32) -> ()
//      - (i32) -> (i32)
//      - (i32, i32) -> ()
//      - (i32, i32) -> (i32)
//      - (i32, i32, i32) -> (i32)
//
// And so we'll only provide support for creating instances of `wrapped_func_t`
// from C functions with exactly these signatures, respectively:
//      - int64_t func(doom_module_context_t *);
//      - void func(doom_module_context_t *, int32_t);
//      - int32_t func(doom_module_context_t *, int32_t);
//      - void func(doom_module_context_t *, int32_t, int32_t);
//      - int32_t func(doom_module_context_t *, int32_t, int32_t);
//      - int32_t func(doom_module_context_t *, int32_t, int32_t, int32_t);

// Declare function types for all function signatures currently supported.
//
// The names of these function types follow the pattern
// `func_ptr__X__return_Y`, where:
//      - X represents the input types, in order, accepted by the wrapped
//      function.
//      - Y represents the output types, in order, produced by the wrapped
//      function.

typedef int64_t func_ptr__void__return_i64(doom_module_context_t *context);
typedef void func_ptr__i32__return_void(doom_module_context_t *context,
                                        int32_t);
typedef int32_t func_ptr__i32__return_i32(doom_module_context_t *context,
                                          int32_t);
typedef void func_ptr__i32_i32__return_void(doom_module_context_t *context,
                                            int32_t, int32_t);
typedef int32_t func_ptr__i32_i32__return_i32(doom_module_context_t *context,
                                              int32_t, int32_t);
typedef int32_t
func_ptr__i32_i32_i32__return_i32(doom_module_context_t *context, int32_t,
                                  int32_t, int32_t);

// Declare functions for creating a new `wrapped_func_t` for all function
// signatures currently supported.
//
// The names of these functions follow the pattern
// `wrapped_func_new__X__return_Y`, where:
//      - X represents the input types, in order, accepted by the wrapped
//      function.
//      - Y represents the output types, in order, produced by the wrapped
//      function.
//
// Caller receives ownership of the returned `wrapped_func_t`, which must be
// relinquished by a call to `wrapped_func_delete`
wrapped_func_t *
wrapped_func_new__void__return_i64(func_ptr__void__return_i64 *func);
wrapped_func_t *
wrapped_func_new__i32__return_void(func_ptr__i32__return_void *func);
wrapped_func_t *
wrapped_func_new__i32__return_i32(func_ptr__i32__return_i32 *func);
wrapped_func_t *
wrapped_func_new__i32_i32__return_void(func_ptr__i32_i32__return_void *func);
wrapped_func_t *
wrapped_func_new__i32_i32__return_i32(func_ptr__i32_i32__return_i32 *func);
wrapped_func_t *wrapped_func_new__i32_i32_i32__return_i32(
    func_ptr__i32_i32_i32__return_i32 *func);

// Call the function wrapped up in an instance of `wrapped_func_t`.
//
// Arguments to the underlying function are provided via args/nargs.
// Values produced by the underlying function are provided via results.
//
// This function returns NULL on success, and returns a non-NULL `wasm_trap_t`
// in the case of failure (which includes the case where the provided args
// doesn't match the number or types of args expected by the underlying
// function, or the number of results doesn't match the number of results
// produced by the underlying function).
//
// The caller receives ownership of the returned `wasm_trap_t` in the case of
// failure.
wasm_trap_t *wrapped_func_call(wrapped_func_t *wrappedFunc,
                               doom_module_context_t *context,
                               const wasmtime_val_t *args, size_t nargs,
                               wasmtime_val_t *results, size_t nresults);

wasm_functype_t *wrapped_func_functype(wrapped_func_t *wrappedFunc);

void wrapped_func_delete(wrapped_func_t *wrappedFunc);

#endif
