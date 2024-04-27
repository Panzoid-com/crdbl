#pragma once
#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/wasm_worker.h>
#include <Core.h>
#include <OperationFilter.h>
#include <OperationBuilder.h>
#include <OperationIterator.h>
#include <TypeLogGenerator.h>
#include <RefCounted.h>
#include "StringIds.h"
#include "JsObjectSerializer.h"
#include "JsonSerializer.h"
#include "TypeLogGeneratorWrapper.h"
#include "ConcurrentQueue.h"
#include "ReaderWriterLock.h"
#include "Worker.h"

#include <string>
#include <functional>
using namespace std::placeholders;

#include <unordered_map>

using namespace emscripten;

enum QueueItemType {
  Apply,
  Unapply,
  ResolveTypeSpec
};

struct OpQueueItem {
  QueueItemType type;
  RefCounted<const LogOperation> op;
  std::string typeId;
};

class ProjectDB
{
public:
  ProjectDB(val getTypeSpecCallback, val eventRaisedCallback);
  ~ProjectDB();

  void resolveTypeSpec(std::string type);

  val getNode(uint32_t clock, uint32_t site, uint32_t child);
  val getNodeChildren(uint32_t clock, uint32_t site, uint32_t child, bool includePending = false);
  double getNodeValue(uint32_t clock, uint32_t site, uint32_t child);
  std::string getNodeBlockValue(uint32_t clock, uint32_t site, uint32_t child);

  OperationBuilder * createOperationBuilder();
  TypeLogGeneratorWrapper * createTypeLogGenerator();
  Worker * createWorker(std::string url);

  VectorTimestamp * getVectorClock();

  IWritableStream<RefCounted<const LogOperation>> * createApplyStream();
  IWritableStream<RefCounted<const LogOperation>> * createUnapplyStream();

  static void addType(std::string type, std::string format, val data);
  static void deleteType(std::string type);
  static void clearTypes();

  static StringNodeId NULLID();
  static StringNodeId ROOTID();
  static StringNodeId PENDINGID();
  static StringNodeId INITID();

private:
  CoreInit coreInit;
  Core core;

  val getTypeSpecCallback;
  val eventRaisedCallback;

  ReaderWriterLock lock;

  emscripten_wasm_worker_t worker = EMSCRIPTEN_WASM_WORKER_ID_PARENT;

  static std::unordered_map<std::string, std::basic_string<uint8_t>> * typeCache;
  static ReaderWriterLock typeCacheLock;

  static void getTypeSpecHandler(ProjectDB * ctx, std::string type);
  static void eventRaisedHandler(ProjectDB * ctx, const Event & event);

  friend class OperationBuilderWrapper;
  friend class TransformOperationStreamWrapper;
};