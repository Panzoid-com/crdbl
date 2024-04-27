#pragma once
#include <OperationBuilder.h>
#include <TypeLogGenerator.h>
#include <Serialization/LogOperationSerialization.h>
#include <string>
#include <emscripten/bind.h>
#include "StringIds.h"
#include "ProjectDB.h"

using namespace emscripten;

class OperationBuilderWrapper : public OperationBuilder
{
public:
  static IReadableStream<RefCounted<const LogOperation>> * getReadableStream(OperationBuilder & ref)
  {
    return &ref.getReadableStream();
  }

  static StringNodeId cloneAllNodes(OperationBuilder & ref, const StringNodeId & rootId)
  {
    auto applyStream = std::unique_ptr<IWritableStream<RefCounted<const LogOperation>>>(
      ref.createApplyStream());

    NodeId cloneRootId = { ref.getNextTimestamp(), 0 };

    TypeLogGenerator typeLogGenerator(static_cast<OperationBuilderWrapper *>(&ref)->core);
    typeLogGenerator.addAllNodes(stringToNodeId(rootId));
    typeLogGenerator.generate(*applyStream);

    applyStream->close();

    return cloneRootId.toString();
  }

  static StringNodeId cloneAllNodesFrom(OperationBuilder & ref, const ProjectDB & db, const StringNodeId & rootId)
  {
    auto applyStream = std::unique_ptr<IWritableStream<RefCounted<const LogOperation>>>(
      ref.createApplyStream());

    NodeId cloneRootId = { ref.getNextTimestamp(), 0 };

    TypeLogGenerator typeLogGenerator(&db.core);
    typeLogGenerator.addAllNodes(stringToNodeId(rootId));
    typeLogGenerator.generate(*applyStream);

    applyStream->close();

    return cloneRootId.toString();
  }

  static void startGroup(OperationBuilder & ref)
  {
    return ref.startGroup(OperationType::GroupOperation);
  }

  static Timestamp applyOperations(OperationBuilder & ref, std::string format, val data)
  {
    auto deserializer = std::unique_ptr<ILogOperationDeserializer>(
      LogOperationSerialization::CreateDeserializer(format, DeserializeDirection::Forward));
    if (deserializer == nullptr)
    {
      return Timestamp::Null;
    }

    Timestamp ts = ref.getNextTimestamp();

    auto applyStream = std::unique_ptr<IWritableStream<RefCounted<const LogOperation>>>(
      ref.createApplyStream());
    deserializer->pipeTo(*applyStream);

    const auto size = data["length"].as<unsigned>();
    uint8_t * _data = new uint8_t[size];

    val memoryView{typed_memory_view(size, _data)};
    memoryView.call<void>("set", data);

    deserializer->write(std::string_view(reinterpret_cast<const char *>(_data), size));

    delete[] _data;

    deserializer->close();
    applyStream->close();

    return ts;
  }

  static StringNodeId createNode(OperationBuilder & ref, const std::string & nodeType)
  {
    return ref.createNode(nodeType).toString();
  }

  static StringNodeId createContainerNode(OperationBuilder & ref, const std::string & nodeType, const std::string & childType)
  {
    return ref.createContainerNode(nodeType, childType).toString();
  }

  static StringNodeId addChild(OperationBuilder & ref, StringNodeId parentNodeId, StringNodeId childNodeId, val data)
  {
    NodeId _parentNodeId = stringToNodeId(parentNodeId);
    NodeId _childNodeId = stringToNodeId(childNodeId);

    size_t size = 0;
    uint8_t * _data = nullptr;

    if (!data.isNull())
    {
      size = data["length"].as<unsigned>();
     _data = new uint8_t[size];

      val memoryView{typed_memory_view(size, _data)};
      memoryView.call<void>("set", data);
    }

    EdgeId result = ref.addChild(_parentNodeId, _childNodeId, _data, size);

    if (_data != nullptr)
      delete[] _data;

    return result.toString();
  }

  static Timestamp removeChild(OperationBuilder & ref, StringNodeId parentNodeId, StringEdgeId childEdgeId)
  {
    NodeId _parentNodeId = stringToNodeId(parentNodeId);
    NodeId _childEdgeId = stringToNodeId(childEdgeId);

    return ref.removeChild(_parentNodeId, _childEdgeId);
  }

  static val createKey(OperationBuilder & ref, std::string key)
  {
    val in_arr = val(typed_memory_view(key.size(), key.data()));
    val out_arr = val::global("Uint8Array").new_(in_arr);
    return out_arr;
  }

  static val createFloat64Key(OperationBuilder & ref, double key)
  {
    val in_arr = val(typed_memory_view(sizeof(double), reinterpret_cast<const char *>(&key)));
    val out_arr = val::global("Uint8Array").new_(in_arr);
    return out_arr;
  }

  static val createPositionBetweenEdges(OperationBuilder & ref, val firstEdgeId, val secondEdgeId)
  {
    EdgeId _firstEdgeId;
    EdgeId _secondEdgeId;

    if (firstEdgeId.isString())
    {
      _firstEdgeId = stringToNodeId(firstEdgeId.as<std::string>());
    }
    else
    {
      _firstEdgeId = EdgeId::Null;
    }

    if (secondEdgeId.isString())
    {
      _secondEdgeId = stringToNodeId(secondEdgeId.as<std::string>());
    }
    else
    {
      _secondEdgeId = EdgeId::Null;
    }

    auto data = ref.createPositionBetweenEdges(_firstEdgeId, _secondEdgeId);

    val in_arr = val(typed_memory_view(data.size(), data.data()));
    val out_arr = val::global("Uint8Array").new_(in_arr);

    return out_arr;
  }

  static val createPositionFromIndex(OperationBuilder & ref, StringNodeId parentNodeId, size_t index)
  {
    NodeId _parentNodeId = stringToNodeId(parentNodeId);

    auto data = ref.createPositionFromIndex(_parentNodeId, index);

    val in_arr = val(typed_memory_view(data.size(), data.data()));
    val out_arr = val::global("Uint8Array").new_(in_arr);

    return out_arr;
  }

  static val createPositionFromEdge(OperationBuilder & ref, StringNodeId parentNodeId, StringEdgeId sourceEdgeId)
  {
    NodeId _parentNodeId = stringToNodeId(parentNodeId);
    NodeId _sourceEdgeId = stringToNodeId(sourceEdgeId);

    auto data = ref.createPositionFromEdge(_parentNodeId, _sourceEdgeId);

    val in_arr = val(typed_memory_view(data.size(), data.data()));
    val out_arr = val::global("Uint8Array").new_(in_arr);

    return out_arr;
  }

  static val createPositionAbsolute(OperationBuilder & ref, double position)
  {
    auto data = ref.CreatePositionAbsolute(position);

    val in_arr = val(typed_memory_view(data.size(), data.data()));
    val out_arr = val::global("Uint8Array").new_(in_arr);

    return out_arr;
  }

  static Timestamp setValueRaw(OperationBuilder & ref, const StringNodeId & nodeId, val data)
  {
    const auto size = data["length"].as<unsigned>();
    uint8_t * _data = new uint8_t[size];

    val memoryView{typed_memory_view(size, _data)};
    memoryView.call<void>("set", data);

    auto result = ref.setValue(stringToNodeId(nodeId), _data, size);

    delete[] _data;

    return result;
  }

  static Timestamp setValuePreviewRaw(OperationBuilder & ref, const StringNodeId & nodeId, val data)
  {
    const auto size = data["length"].as<unsigned>();
    uint8_t * _data = new uint8_t[size];

    val memoryView{typed_memory_view(size, _data)};
    memoryView.call<void>("set", data);

    auto result = ref.setValuePreview(stringToNodeId(nodeId), _data, size);

    delete[] _data;

    return result;
  }

  template <typename T>
  static Timestamp setValue(OperationBuilder & ref, const StringNodeId & nodeId, T value)
  {
    return ref.setValue(stringToNodeId(nodeId), value);
  }

  template <typename T>
  static Timestamp setValuePreview(OperationBuilder & ref, const StringNodeId & nodeId, T value)
  {
    return ref.setValuePreview(stringToNodeId(nodeId), value);
  }

  static Timestamp setValueAuto(OperationBuilder & ref, const StringNodeId & nodeId, double value)
  {
    return ref.setValueAuto(stringToNodeId(nodeId), value);
  }

  static Timestamp setValuePreviewAuto(OperationBuilder & ref, const StringNodeId & nodeId, double value)
  {
    return ref.setValuePreviewAuto(stringToNodeId(nodeId), value);
  }

  static Timestamp clearValuePreview(OperationBuilder & ref, const StringNodeId & nodeId)
  {
    return ref.clearValuePreview(stringToNodeId(nodeId));
  }

  static StringNodeId getNextNodeId(OperationBuilder & ref)
  {
    NodeId ret;
    ret.ts = ref.getNextTimestamp();
    ret.child = 0;
    return ret.toString();
  }

  static void insertText(OperationBuilder & ref, const StringNodeId & nodeId, size_t offset, const std::string & str)
  {
    ref.insertText(stringToNodeId(nodeId), offset, reinterpret_cast<const uint8_t *>(str.c_str()), str.length());
  }

  static void insertTextAtFront(OperationBuilder & ref, const StringNodeId & nodeId, const std::string & str)
  {
    ref.insertTextAtFront(stringToNodeId(nodeId), reinterpret_cast<const uint8_t *>(str.c_str()), str.length());
  }

  static void deleteText(OperationBuilder & ref, const StringNodeId & nodeId, size_t offset, size_t length)
  {
    ref.deleteText(stringToNodeId(nodeId), offset, length);
  }

  static void deleteInheritedText(OperationBuilder & ref, const StringNodeId & nodeId)
  {
    ref.deleteInheritedText(stringToNodeId(nodeId));
  }
};