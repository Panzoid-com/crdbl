#pragma once
#include "Core.h"
#include "OperationLog.h"
#include "Streams/ReadableStreamBase.h"
#include <string>
#include <stack>

class OperationBuilder
{
public:
  OperationBuilder(const Core * core, uint32_t siteId = 0, Tag tag = Tag::Default());

  void setEnabled(bool value)
  {
    enabled = value;
  }

  void setSiteId(uint32_t siteId)
  {
    this->siteId = siteId;
  }
  uint32_t getSiteId()
  {
    return siteId;
  }
  void setTag(const Tag & tag)
  {
    this->tag = tag;
  }
  Tag getTag()
  {
    return tag;
  }

  //NOTE: this is hacky, currently balanced between backwards compat and the
  //  the proper behavior, which is to use a log's clock somehow
  void setOperationLog(const OperationLog & log)
  {
    clock = &(log.getVectorClock());
  }

  IReadableStream<RefCounted<const LogOperation>> & getReadableStream();

  void startGroup(const OperationType & groupType);
  Timestamp commitGroup();
  void discardGroup();

  NodeId cloneAllNodes(const NodeId & rootId);

  // Timestamp applyOperations(const Operation * data, size_t length);
  Timestamp applyOperation(const Operation * op, size_t length);

  Timestamp applyOperation(const LogOperation & op);
  IWritableStream<RefCounted<const LogOperation>> * createApplyStream();
  Timestamp undoOperation(const LogOperation & op);
  IWritableStream<RefCounted<const LogOperation>> * createUndoStream();

  Timestamp setAttribute(uint8_t attributeId, const std::string & data);
  Timestamp setAttribute(uint8_t attributeId, const uint8_t * data, uint32_t length);

  NodeId createNode(const NodeType & nodeType);
  NodeId createNode(const std::string & nodeType);
  NodeId createContainerNode(const NodeType & nodeType, const NodeType & childType);
  NodeId createContainerNode(const std::string & nodeType, const std::string & childType);
  EdgeId addChild(const NodeId & parentId, const NodeId & childId);
  EdgeId addChild(const NodeId & parentId, const NodeId & childId, const std::string edgeData);
  EdgeId addChild(const NodeId & parentId, const NodeId & childId, const uint8_t * data, uint32_t length);
  Timestamp removeChild(const NodeId & parentId, const EdgeId & edgeId);

  std::string createPositionBetweenEdges(const EdgeId & firstEdgeId, const EdgeId & secondEdgeId) const;
  std::string createPositionFromIndex(const NodeId & parentId, size_t index) const;
  std::string createPositionFromEdge(const NodeId & parentId, const EdgeId & sourceEdgeId) const;
  std::string createPositionAbsolute(double position) const;
  static std::string CreatePositionAbsolute(double position);
  static std::string CreatePosition(const Position & position);

  template <typename T>
  Timestamp setValue(const NodeId & nodeId, T value);
  Timestamp setValueAuto(const NodeId & nodeId, double value);
  Timestamp setValue(const NodeId & nodeId, const uint8_t * data, uint32_t length);
  template <typename T>
  Timestamp setValuePreview(const NodeId & nodeId, T value);
  Timestamp setValuePreviewAuto(const NodeId & nodeId, double value);
  Timestamp setValuePreview(const NodeId & nodeId, const uint8_t * data, uint32_t length);
  Timestamp clearValuePreview(const NodeId & nodeId);

  void insertText(const NodeId & nodeId, size_t offset, const std::string & str);
  void insertText(const NodeId & nodeId, size_t offset, const uint8_t * data, size_t length);
  void deleteText(const NodeId & nodeId, size_t offset, size_t length);

  void insertTextAtFront(const NodeId & nodeId, const std::string & str);
  void insertTextAtFront(const NodeId & nodeId, const uint8_t * data, size_t length);
  void deleteInheritedText(const NodeId & nodeId);

  static void UndoGroup(std::basic_string<char> & buffer, const LogOperation & op);
  static void Undo(std::basic_string<char> & buffer, const Operation * op);

  static void AttributeSet(std::basic_string<char> & buffer, const uint8_t attributeId, const uint8_t * data, uint16_t length);

  static void NodeCreate(std::basic_string<char> & buffer, const uint8_t * data, uint8_t length);
  static void EdgeCreate(std::basic_string<char> & buffer, const NodeId & parentId, const NodeId & childId);
  static void EdgeDelete(std::basic_string<char> & buffer, const NodeId & parentId, const EdgeId & edgeId);

  static void ValueSet(std::basic_string<char> & buffer, const NodeId & nodeId, const uint8_t * data, uint32_t length);
  static void ValueSetPreview(std::basic_string<char> & buffer, const NodeId & nodeId, const uint8_t * data, uint32_t length);

  static void BlockValueInsertAfter(std::basic_string<char> & buffer, const NodeId & nodeId, const Timestamp & blockId, uint32_t offset, const uint8_t * data, uint32_t length);
  static void BlockValueDeleteAfter(std::basic_string<char> & buffer, const NodeId & nodeId, const Timestamp & blockId, uint32_t offset, uint32_t length);

  uint32_t siteId;
  Tag tag;

  Timestamp getNextTimestamp() const;
  Timestamp getCurrentTimestamp() const;

protected:
  const Core * core;
  const VectorTimestamp * clock;

private:
  void startLogOperation();
  Timestamp commitLogOperation();

  template <typename T>
  T * appendData();
  template <typename T>
  T * appendData(size_t size);
  template <typename T>
  static T * AppendData(std::basic_string<char> & buffer);
  template <typename T>
  static T * AppendData(std::basic_string<char> & buffer, size_t size);

  std::basic_string<char> & getOpBuffer();
  Timestamp finishOp();

  Timestamp applyOperation(const Operation & op);

  static std::basic_string<char> DoubleToValueData(NodeType baseType, double value);

  std::basic_string<char> opBuffer;

  bool enabled = true;
  bool autoApply = true;

  struct GroupContext
  {
    OperationType type;
    uint32_t bufferOffset = 0;
    uint32_t opCount = 0;
  };

  std::deque<GroupContext> groupContexts;

  ReadableStreamBase<RefCounted<const LogOperation>> readableStream;
};

template <typename T>
Timestamp OperationBuilder::setValue(const NodeId & nodeId, T value)
{
  return setValue(nodeId, reinterpret_cast<const uint8_t *>(&value), sizeof(T));
}

template <typename T>
Timestamp OperationBuilder::setValuePreview(const NodeId & nodeId, T value)
{
  return setValuePreview(nodeId, reinterpret_cast<const uint8_t *>(&value), sizeof(T));
}