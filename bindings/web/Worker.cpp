#include "Worker.h"

Worker::Worker(ProjectDB & db, std::string url)
{
  if (url.size() > sizeof(info->url))
  {
    throw std::runtime_error("url too long");
  }

  info = new WorkerInfo();
  info->db = &db;
  strncpy(info->url, url.c_str(), sizeof(info->url));

  worker = emscripten_malloc_wasm_worker(Worker::StackSize);

  emscripten_wasm_worker_post_function_vi(worker, &Worker::worker_main,
    reinterpret_cast<int>(info));
}

Worker::~Worker()
{
  emscripten_terminate_wasm_worker(worker);
  delete info;
}

val Worker::getWorker()
{
  return val::module_property("getWorker")(val(worker));
}

void Worker::worker_main(int infoPtr)
{
  WorkerInfo * info = reinterpret_cast<WorkerInfo *>(infoPtr);

  //NOTE: this is hacky and somewhat fragile
  //this registers the bound functions into the Module scope,
  //  which isn't done in wasm workers for some reason
  EM_ASM({ callRuntimeCallbacks(__ATINIT__); });

  EM_ASM({
    const database = Module.ProjectDB.unsafeFromPtr($0);

    import(UTF8ToString($1)).then((module) => {
      module.default(database);
    });
  }, info->db, info->url);
}