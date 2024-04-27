#pragma once
#include <string>
#include <cstring>
#include <cstdint>
#include <map>
#include <unordered_map>
#include <set>
#include <vector>
#include "CoreInit.h"
#include "Event.h"
#include "Promise.h"
#include "PromiseAll.h"
#include "Timestamp.h"
#include "VectorTimestamp.h"
#include "Nodes/Node.h"
#include "PrimitiveNodeTypes.h"
#include "Nodes/ContainerNode.h"
#include "Nodes/SetNode.h"
#include "Nodes/ListNode.h"
#include "Nodes/MapNode.h"
#include "Nodes/ReferenceNode.h"
#include "Nodes/OrderedFloat64MapNode.h"
#include "Nodes/ValueNode.h"
#include "Nodes/BlockValueNode.h"
#include "BlockValueCacheItem.h"
#include "Operation.h"
#include "LogOperation.h"
#include "OperationIterator.h"
#include "InheritanceContext.h"
#include "PairHash.h"
#include "Position.h"
#include "Attribute.h"
#include "Streams/IWritableStream.h"
#include "RefCounted.h"

class Core
{
public:
  Core();
  Core(CoreInit coreInit);
  ~Core();

  void applyOperation(const RefCounted<const LogOperation> & op);
  IWritableStream<RefCounted<const LogOperation>> * createApplyStream();
  void unapplyOperation(const RefCounted<const LogOperation> & op);
  IWritableStream<RefCounted<const LogOperation>> * createUnapplyStream();

  void resolveTypeSpec(const std::string & type, const Operation * ops, size_t length);

  void serializeNode(IObjectSerializer & serializer, const NodeId & nodeId) const;
  void serializeNodeChildren(IObjectSerializer & serializer, const NodeId & nodeId, bool includePending) const;

  double getNodeValue(const NodeId & nodeId) const;
  void getNodeBlockValue(std::string & outString, const NodeId & nodeId) const;

  VectorTimestamp clock;

  const Node * getExistingNode(const NodeId & nodeId) const;

private:
  CoreInit coreInit;
  std::unordered_map<NodeType, Promise<std::tuple<const Operation *, size_t>>> getTypeSpecPromises;
  std::unordered_map<NodeId, Promise<void>> nodeTypeReadyPromises;
  std::unordered_map<NodeId, Promise<void>> nodeReadyPromises;
  std::unordered_map<NodeId, Node *> nodes;

  std::unordered_map<std::pair<NodeType, uint32_t>, BlockValueCacheItem, PairHash> blockValueCache;

  char * createBlockValueData(const uint8_t * data, uint32_t length);
  std::vector<char *> blockValueData;

  void processTypeRequests();
  bool isProcessingTypeRequests = false;
  std::vector<NodeType> typeRequests;

  void setUpBuiltInNodes();

  Promise<std::tuple<const Operation *, size_t>> getTypeSpec(NodeType nodeType);

  bool isNodeTypeReady(const Node * node) const;
  bool isNodeReady(const Node * node) const;
  Promise<void> waitForNodeTypeReady(const NodeId & nodeId);
  Promise<void> waitForNodeReady(const NodeId & nodeId);

  Node * getNode(const NodeId & nodeId);

  template <class T>
  Node * swapNode(Node * node);
  void assignContainerNodeAttributes(Node * node, const AttributeMap * attributes);
  void setNodeType(const NodeId & nodeId, const PrimitiveNodeType & type,
    const AttributeMap * attributes);

  NodeId transformNodeId(InheritanceContext * inheritanceContext, const NodeId & nodeId);
  EdgeId transformEdgeId(InheritanceContext * inheritanceContext, const NodeId & edgeId);
  Timestamp transformTimestamp(InheritanceContext * inheritanceContext, const Timestamp & timestamp);
  Timestamp transformTimestamp(InheritanceContext * inheritanceContext, const Timestamp & blockId, const Timestamp & timestamp);
  Timestamp transformBlockId(InheritanceContext * inheritanceContext, const NodeId & nodeId, const Timestamp & blockId);

  Promise<void> inheritType(const NodeId & nodeId, NodeType type,
    InheritanceContext * prevInheritanceContext, AttributeMap * attributes);
  Promise<void> applyOperations(OperationIterator * it, InheritanceContext * inheritanceContext);

  Promise<void> applyOperation(const Timestamp & ts, const Operation * op,
    InheritanceContext * inheritanceContext);

  Promise<void> applyOperation(const Timestamp & ts, const UndoGroupOperation * op,
    InheritanceContext * inheritanceContext);
  Promise<void> applyOperation(const Timestamp & ts, const GroupOperation * op,
    InheritanceContext * inheritanceContext);
  Promise<void> applyOperation(const Timestamp & ts, const AtomicGroupOperation * op,
    InheritanceContext * inheritanceContext);
  Promise<void> applyOperation(const Timestamp & ts, const NodeCreateOperation * op,
    InheritanceContext * inheritanceContext, AttributeMap * attributes);
  Promise<void> applyOperation(const Timestamp & ts, const EdgeCreateOperation * op,
    InheritanceContext * inheritanceContext, const AttributeMap * attributes);
  Promise<void> applyOperation(const Timestamp & ts, const EdgeDeleteOperation * op,
    InheritanceContext * inheritanceContext);
  Promise<void> applyOperation(const Timestamp & ts, const ValueSetOperation * op,
    InheritanceContext * inheritanceContext);
  Promise<void> applyOperation(const Timestamp & ts, const BlockValueInsertAfterOperation * op,
    InheritanceContext * inheritanceContext);
  Promise<void> applyOperation(const Timestamp & ts, const BlockValueDeleteAfterOperation * op,
    InheritanceContext * inheritanceContext);

  Promise<void> applyUndoOperation(const Timestamp & ts, const Timestamp & prevTs,
    const Operation * op, bool isUndo);

  Promise<void> applyUndoOperation(const Timestamp & ts, const Timestamp & prevTs,
    const UndoEdgeCreateOperation * op, bool isUndo);
  Promise<void> applyUndoOperation(const Timestamp & ts, const Timestamp & prevTs,
    const UndoEdgeDeleteOperation * op, bool isUndo);
  Promise<void> applyUndoOperation(const Timestamp & ts, const Timestamp & prevTs,
    const UndoValueSetOperation * op, bool isUndo);
  Promise<void> applyUndoOperation(const Timestamp & ts, const Timestamp & prevTs,
    const UndoBlockValueInsertAfterOperation * op, bool isUndo);
  Promise<void> applyUndoOperation(const Timestamp & ts, const Timestamp & prevTs,
    const UndoBlockValueDeleteAfterOperation * op, bool isUndo);

  void unapplyOperation(const Timestamp & ts, const Operation * op);

  void unapplyOperation(const Timestamp & ts, const UndoGroupOperation * op);
  void unapplyOperation(const Timestamp & ts, const GroupOperation * op);
  void unapplyOperation(const Timestamp & ts, const AtomicGroupOperation * op);
  void unapplyOperation(const Timestamp & ts, const NodeCreateOperation * op);
  void unapplyOperation(const Timestamp & ts, const EdgeCreateOperation * op);
  void unapplyOperation(const Timestamp & ts, const EdgeDeleteOperation * op);
  void unapplyOperation(const Timestamp & ts, const ValueSetOperation * op);
  void unapplyOperation(const Timestamp & ts, const BlockValueInsertAfterOperation * op);
  void unapplyOperation(const Timestamp & ts, const BlockValueDeleteAfterOperation * op);

  void unapplyUndoOperation(const Timestamp & ts, const Timestamp & prevTs,
    const Operation * op, bool isUndo);

  void unapplyUndoOperation(const Timestamp & ts, const Timestamp & prevTs,
    const UndoEdgeCreateOperation * op, bool isUndo);
  void unapplyUndoOperation(const Timestamp & ts, const Timestamp & prevTs,
    const UndoEdgeDeleteOperation * op, bool isUndo);
  void unapplyUndoOperation(const Timestamp & ts, const Timestamp & prevTs,
    const UndoValueSetOperation * op, bool isUndo);
  void unapplyUndoOperation(const Timestamp & ts, const Timestamp & prevTs,
    const UndoBlockValueInsertAfterOperation * op, bool isUndo);
  void unapplyUndoOperation(const Timestamp & ts, const Timestamp & prevTs,
    const UndoBlockValueDeleteAfterOperation * op, bool isUndo);

  template<typename T>
  static int valueNodeChangedCallback(Core * core, const NodeId & nodeId,
    bool generateEvent, const T & newValue, const T & oldValue);
  static int blockValueNodeChangedCallback(Core * core, const Timestamp & ts,
    const NodeId & nodeId, size_t offset, const char * data, uint32_t length);
};
