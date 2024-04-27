#pragma once
#include <emscripten/val.h>
#include <emscripten/wasm_worker.h>
#include <string>

using namespace emscripten;

class ProjectDB;

struct WorkerInfo
{
  ProjectDB * db;
  char url[255];
};

class Worker
{
public:
  Worker(ProjectDB & db, std::string url);
  ~Worker();
  val getWorker();

  static void worker_main(int infoPtr);
private:
  static const int StackSize = 1024 * 256; //arbitrary but should be large enough

  emscripten_wasm_worker_t worker;
  WorkerInfo * info;
};