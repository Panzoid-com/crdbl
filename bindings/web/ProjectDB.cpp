#include "ProjectDB.h"
#include <Serialization/LogOperationSerialization.h>
#include <Streams/CallbackWritableStream.h>
#include <OperationLog.h>

ReaderWriterLock ProjectDB::typeCacheLock;
std::unordered_map<std::string, std::basic_string<uint8_t>> * ProjectDB::typeCache;

ProjectDB::ProjectDB(val getTypeSpecCallback,
  val eventRaisedCallback)
  : coreInit(
      [this](const std::string & type){ return ProjectDB::getTypeSpecHandler(this, type); },
      [this](const Event & evt){ return ProjectDB::eventRaisedHandler(this, evt); }),
    core(coreInit),
    getTypeSpecCallback(getTypeSpecCallback),
    eventRaisedCallback(eventRaisedCallback)
{
  if (typeCache == nullptr)
  {
    typeCache = new std::unordered_map<std::string, std::basic_string<uint8_t>>();
  }
}

ProjectDB::~ProjectDB()
{
  if (worker != EMSCRIPTEN_WASM_WORKER_ID_PARENT)
  {
    emscripten_terminate_wasm_worker(worker);
  }
}

//static
void ProjectDB::getTypeSpecHandler(ProjectDB * ctx, std::string type)
{
  typeCacheLock.lock();
  bool hasData = typeCache->contains(type);
  if (hasData)
  {
    std::basic_string<uint8_t> & cachedData = typeCache->at(type);
    typeCacheLock.unlock();
    ctx->core.resolveTypeSpec(type, reinterpret_cast<const Operation *>(cachedData.c_str()),
      cachedData.size());
    return;
  }
  typeCacheLock.unlock();

  ctx->getTypeSpecCallback(type, true);
}

//static
void ProjectDB::eventRaisedHandler(ProjectDB * ctx, const Event & event)
{
  if (emscripten_current_thread_is_wasm_worker())
  {
    throw std::runtime_error("eventRaisedHandler called from worker thread");
  }

  JsObjectSerializer serializer;
  event.serialize(serializer);

  ctx->lock.unlock();
  ctx->eventRaisedCallback(serializer.result());
  ctx->lock.lock();
}

//static
void ProjectDB::addType(std::string type, std::string format, val data)
{
  const auto size = data["length"].as<unsigned>();
  uint8_t * _data = new uint8_t[size];

  val memoryView{typed_memory_view(size, _data)};
  memoryView.call<void>("set", data);

  typeCacheLock.lock();

  std::basic_string<uint8_t> & cachedData = (*typeCache)[type];
  cachedData.clear();

  auto deserializer = std::unique_ptr<ILogOperationDeserializer>(
    LogOperationSerialization::CreateDeserializer(format,
      DeserializeDirection::Forward));
  CallbackWritableStream<RefCounted<const LogOperation>> callbackStream(
    [&](const RefCounted<const LogOperation> & data)
    {
      cachedData.append(reinterpret_cast<const uint8_t *>(&data->op), data->op.getSize());
    },
    [&](){});
  deserializer->pipeTo(callbackStream);

  deserializer->write(std::string_view(reinterpret_cast<const char *>(_data), size));
  deserializer->close();

  typeCacheLock.unlock();

  delete[] _data;
}

//static
void ProjectDB::deleteType(std::string type)
{
  typeCacheLock.lock();
  typeCache->erase(type);
  typeCacheLock.unlock();
}

//static
void ProjectDB::clearTypes()
{
  typeCacheLock.lock();
  typeCache->clear();
  typeCacheLock.unlock();
}

void ProjectDB::resolveTypeSpec(std::string type)
{
  if (emscripten_current_thread_is_wasm_worker())
  {
    throw std::runtime_error("resolveTypeSpec cannot be used in worker thread");
  }

  typeCacheLock.lock();
  std::basic_string<uint8_t> & cachedData = typeCache->at(type);
  typeCacheLock.unlock();

  lock.lock();
  core.resolveTypeSpec(type,
    reinterpret_cast<const Operation *>(cachedData.c_str()),
    cachedData.size());
  lock.unlock();
}

val ProjectDB::getNode(uint32_t clock, uint32_t site, uint32_t child)
{
  NodeId _nodeId { Timestamp { clock, site }, child };

  JsObjectSerializer serializer;
  lock.lock_shared();
  core.serializeNode(serializer, _nodeId);
  lock.unlock_shared();

  return serializer.result();
}

val ProjectDB::getNodeChildren(uint32_t clock, uint32_t site, uint32_t child, bool includePending)
{
  NodeId _nodeId { Timestamp { clock, site }, child };

  JsObjectSerializer serializer;
  lock.lock_shared();
  core.serializeNodeChildren(serializer, _nodeId, includePending);
  lock.unlock_shared();

  return serializer.result();
}

double ProjectDB::getNodeValue(uint32_t clock, uint32_t site, uint32_t child)
{
  NodeId _nodeId { Timestamp { clock, site }, child };

  lock.lock_shared();
  auto value = core.getNodeValue(_nodeId);
  lock.unlock_shared();

  return value;
}

std::string ProjectDB::getNodeBlockValue(uint32_t clock, uint32_t site, uint32_t child)
{
  NodeId _nodeId { Timestamp { clock, site }, child };

  std::string value;
  lock.lock_shared();
  core.getNodeBlockValue(value, _nodeId);
  lock.unlock_shared();

  return value;
}

OperationBuilder * ProjectDB::createOperationBuilder()
{
  if (emscripten_current_thread_is_wasm_worker())
  {
    throw std::runtime_error("createOperationBuilder cannot be used in worker thread");
  }

  return new OperationBuilder(&core);
}

VectorTimestamp * ProjectDB::getVectorClock()
{
  return &core.clock;
}

IWritableStream<RefCounted<const LogOperation>> * ProjectDB::createApplyStream()
{
  if (emscripten_current_thread_is_wasm_worker())
  {
    throw std::runtime_error("createApplyStream cannot be used in worker thread");
  }

  auto * callbackStream = new CallbackWritableStream<RefCounted<const LogOperation>>(
    [&, this](const RefCounted<const LogOperation> & op)
    {
      lock.lock();
      core.applyOperation(op);
      lock.unlock();
    },
    []()
    {

    });

  return callbackStream;
}

IWritableStream<RefCounted<const LogOperation>> * ProjectDB::createUnapplyStream()
{
  if (emscripten_current_thread_is_wasm_worker())
  {
    throw std::runtime_error("createUnapplyStream cannot be used in worker thread");
  }

  auto * callbackStream = new CallbackWritableStream<RefCounted<const LogOperation>>(
    [&, this](const RefCounted<const LogOperation> & op)
    {
      lock.lock();
      core.unapplyOperation(op);
      lock.unlock();
    },
    []()
    {

    });

  return callbackStream;
}

TypeLogGeneratorWrapper * ProjectDB::createTypeLogGenerator()
{
  if (emscripten_current_thread_is_wasm_worker())
  {
    throw std::runtime_error("createTypeLogGenerator cannot be used in worker thread");
  }

  return new TypeLogGeneratorWrapper(&core);
}

Worker * ProjectDB::createWorker(std::string url)
{
  return new Worker(*this, url);
}

StringNodeId ProjectDB::NULLID()
{
  return NodeId::Null.toString();
}

StringNodeId ProjectDB::ROOTID()
{
  return NodeId::Root.toString();
}

StringNodeId ProjectDB::PENDINGID()
{
  return NodeId::Pending.toString();
}

StringNodeId ProjectDB::INITID()
{
  return NodeId::SiteRoot.toString();
}