#pragma once
#include <emscripten/bind.h>
#include <Core.h>
#include <TypeLogGenerator.h>
#include <Serialization/LogOperationSerialization.h>
#include <Streams/CallbackWritableStream.h>
#include "StringIds.h"
#include <string>

using namespace emscripten;

class TypeLogGeneratorWrapper
{
public:
  TypeLogGeneratorWrapper(const Core * core)
    : generator(core) {}

  void addNode(const StringNodeId & nodeId)
  {
    generator.addNode(stringToNodeId(nodeId));
  }

  void addAllNodes(const StringNodeId & rootId)
  {
    generator.addAllNodes(stringToNodeId(rootId));
  }

  void addAllNodesWithFilter(const StringNodeId & rootId, val filterFn)
  {
    auto filterFnInternal = [&](const NodeId & nodeId) -> bool
    {
      return filterFn(nodeId.toString()).isTrue();
    };

    generator.addAllNodesWithFilter(stringToNodeId(rootId), filterFnInternal);
  }

  void generate(IWritableStream<RefCounted<const LogOperation>> * logStream)
  {
    generator.generate(*logStream);
  }

  val generateToBuffer(std::string format)
  {
    auto serializer = LogOperationSerialization::CreateSerializer(format);
    if (serializer == nullptr)
    {
      return val::null();
    }

    std::basic_string<char> output;

    CallbackWritableStream<std::string_view> callbackStream(
    [&](const std::string_view & data)
    {
      output.append(data);
    });
    serializer->pipeTo(callbackStream);

    generate(serializer);

    serializer->close();
    callbackStream.close();

    size_t size = output.size();
    const char * data = output.data();
    val in_arr = val(typed_memory_view(size, data));
    val out_arr = val::global("Uint8Array").new_(in_arr);

    delete serializer;

    return out_arr;
  }

private:
  TypeLogGenerator generator;
};