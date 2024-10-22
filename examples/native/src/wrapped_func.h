#ifndef FUNCTION_WRAPPER_H_
#define FUNCTION_WRAPPER_H_

#include <wasm.h>
#include <wasmtime.h>

typedef struct wrapped_func wrapped_func_t;

// () -> (i64)
// (i32) -> ()  Needs access to memory
// (i32) -> (i32)
// (i32, i32) -> ()
// (i32, i32) -> ()  Needs access to memory
// (i32, i32) -> (i32)  Needs access to memory
// (i32, i32, i32) -> (i32)  Needs access to memory

wrapped_func_t *wrapped_func_new__void__return_i64(int64_t (*func)());
wrapped_func_t *
wrapped_func_new__i32_memory__return_void(void (*func)(int32_t, uint8_t *));
wrapped_func_t *wrapped_func_new__i32__return_i32(int32_t (*func)(int32_t));
wrapped_func_t *wrapped_func_new__i32_i32__return_void(void (*func)(int32_t,
                                                                    int32_t));
wrapped_func_t *wrapped_func_new__i32_i32_memory__return_void(
    void (*func)(int32_t, int32_t, uint8_t *));
wrapped_func_t *wrapped_func_new__i32_i32_memory__return_i32(
    int32_t (*func)(int32_t, int32_t, uint8_t *));
wrapped_func_t *wrapped_func_new__i32_i32_i32_memory__return_i32(
    int32_t (*func)(int32_t, int32_t, int32_t, uint8_t *));

wasm_trap_t *wrapped_func_call(wrapped_func_t *wrappedFunc,
                               wasmtime_caller_t *caller,
                               const wasmtime_val_t *args, size_t nargs,
                               wasmtime_val_t *results, size_t nresults);

wasm_functype_t *wrapped_func_functype(wrapped_func_t *wrappedFunc);

void wrapped_func_delete(wrapped_func_t *wrappedFunc);

#endif
