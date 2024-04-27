#pragma once
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <emscripten/wasm_worker.h>

class ReaderWriterLock {
public:
  ReaderWriterLock() : readers(0), writer(false) {}

  void lock_shared() {
    if (emscripten_current_thread_is_wasm_worker())
    {
      emscripten_lock_waitinf_acquire(&l);
    }
    else
    {
      emscripten_lock_busyspin_waitinf_acquire(&l);
    }
  }

  void unlock_shared() {
    emscripten_lock_release(&l);
  }

  void lock() {
    if (emscripten_current_thread_is_wasm_worker())
    {
      emscripten_lock_waitinf_acquire(&l);
    }
    else
    {
      emscripten_lock_busyspin_waitinf_acquire(&l);
    }
  }

  void unlock() {
    emscripten_lock_release(&l);
  }

private:
  emscripten_lock_t l = EMSCRIPTEN_LOCK_T_STATIC_INITIALIZER;

  std::mutex m;
  std::condition_variable readers_cv;
  std::condition_variable writer_cv;
  std::atomic<int> readers;
  bool writer;
};