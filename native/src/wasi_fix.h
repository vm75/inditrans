#pragma once

#ifdef __wasi_api_h
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-noreturn"
extern "C" {
_Noreturn void __wasi_proc_exit(__wasi_exitcode_t)
    __attribute__((__import_module__("wasi_snapshot_preview1"), __import_name__("proc_exit"))) {
  return;
}
}
#pragma GCC diagnostic pop
#endif // __wasi_api_h
