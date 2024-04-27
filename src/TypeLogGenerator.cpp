#include "TypeLogGenerator.h"
#include "Streams/CallbackWritableStream.h"
#include "Serialization/ILogOperationSerializer.h"
#include "Serialization/LogOperationSerialization.h"
#include <type_traits>

using NoFilter = int;

TypeLogGenerator::TypeLogGenerator(const Core * core)
  : core(core) {}

void TypeLogGenerator::addNode(const NodeId & nodeId)
{
  const Node * node = core->getExistingNode(nodeId);
  if (node == nullptr || node->isAbstractType())
  {
    return;
  }

  nodeMap.insert(std::make_pair(nodeId, NodeId::Null));

  if (rootId.isNull())
  {
    rootId = nodeId;
  }
}

void TypeLogGenerator::addAllNodes(const NodeId & rootId)
{
  findAllNodes<NoFilter>(core, rootId, nodeMap, 0);

  if (this->rootId.isNull())
  {
    this->rootId = rootId;
  }
}

void TypeLogGenerator::addAllNodesWithFilter(const NodeId & rootId, FilterFn filterFn)
{
  findAllNodes<FilterFn>(core, rootId, nodeMap, filterFn);

  if (this->rootId.isNull())
  {
    this->rootId = rootId;
  }
}

void TypeLogGenerator::generate(IWritableStream<RefCounted<const LogOperation>> & logStream)
{
  if (rootId.isNull())
  {
    return;
  }

  TypeLogGenerator::generateTypeLog(core, nodeMap, rootId, ts, logStream);
}

template <class T>
void TypeLogGenerator::findAllNodes(const Core * core, const NodeId & rootId,
  std::unordered_map<NodeId, NodeId> & nodeMap, T filterFn)
{
  if (rootId.isNull())
  {
    return;
  }

  const Node * node = core->getExistingNode(rootId);
  if (node == nullptr || node->isAbstractType())
  {
    return;
  }

  if constexpr (std::is_same<T, FilterFn>::value)
  {
    if (!filterFn(rootId))
    {
      return;
    }
  }

  if (!nodeMap.insert(std::make_pair(rootId, NodeId::Null)).second)
  {
    //item already exists
    return;
  }

  NodeType baseType = node->getBaseType();

  if (baseType == PrimitiveNodeTypes::Set())
  {
    auto setNode = static_cast<const SetNode *>(node);
    for (auto edge = setNode->children; edge != nullptr; edge = edge->next)
    {
      if (edge->childId.isPending())
      {
        continue;
      }

      TypeLogGenerator::findAllNodes(core, edge->childId, nodeMap, filterFn);
    }
  }
  else if (baseType == PrimitiveNodeTypes::List())
  {
    auto listNode = static_cast<const ListNode *>(node);
    for (auto edge = listNode->children; edge != nullptr; edge = edge->nextChild)
    {
      if (edge->childId.isPending())
      {
        continue;
      }

      TypeLogGenerator::findAllNodes(core, edge->childId, nodeMap, filterFn);
    }
  }
  else if (baseType == PrimitiveNodeTypes::Map())
  {
    auto mapNode = static_cast<const MapNode *>(node);
    for (auto it = mapNode->children.begin(); it != mapNode->children.end(); ++it)
    {
      MapEdge * edge = it->second.front();

      if (edge->childId.isPending())
      {
        continue;
      }

      TypeLogGenerator::findAllNodes(core, edge->childId, nodeMap, filterFn);
    }
  }
  else if (baseType == PrimitiveNodeTypes::Reference())
  {
    auto referenceNode = static_cast<const ReferenceNode *>(node);
    for (auto edge = referenceNode->children; edge != nullptr; edge = edge->next)
    {
      //in references, deleted items are still in the children list,
      //  so visibility must be checked (in other containers, this is not true)
      if (edge->childId.isPending() || !edge->effect.isVisible())
      {
        continue;
      }

      TypeLogGenerator::findAllNodes(core, edge->childId, nodeMap, filterFn);

      //only add the first visible item
      break;
    }
  }
  else if (baseType == PrimitiveNodeTypes::OrderedFloat64Map())
  {
    auto mapNode = static_cast<const OrderedFloat64MapNode *>(node);
    for (auto it = mapNode->children.begin(); it != mapNode->children.end(); ++it)
    {
      EdgeId edgeId = it->second.front();
      Edge * edge = mapNode->getExistingEdge(edgeId);

      if (edge->childId.isPending())
      {
        continue;
      }

      TypeLogGenerator::findAllNodes(core, edge->childId, nodeMap, filterFn);
    }
  }
}

RefCounted<LogOperation> TypeLogGenerator::getOpBuffer(size_t opSize)
{
  size_t size = LogOperation::getSizeWithoutOp() + opSize;
  auto op = reinterpret_cast<LogOperation *>(new uint8_t[size]);
  op->tag = Tag::Default();
  op->ts = Timestamp::Null;
  return RefCounted<LogOperation>(op);
}

NodeId TypeLogGenerator::generateTypeLog(const Core * core,
  std::unordered_map<NodeId, NodeId> & addedNodes, const NodeId & nodeId,
  Timestamp & ts, IWritableStream<RefCounted<const LogOperation>> & logStream)
{
  auto existingNode = addedNodes.find(nodeId);
  if (existingNode == addedNodes.end())
  {
    return NodeId::Null;
  }
  else if (existingNode->second != NodeId::Null)
  {
    return existingNode->second;
  }

  const Node * node = core->getExistingNode(nodeId);
  NodeId nodeIdTransformed;
  NodeType baseType = node->getBaseType();

  if (!nodeId.isInherited() ||
    addedNodes.find({ nodeId.ts, node->createdByRootOffset }) == addedNodes.end())
  {
    nodeIdTransformed = { ++ts, 0 };

    NodeType nodeType = node->getType();

    if (PrimitiveNodeTypes::isContainerNodeType(node->getBaseType()) &&
      static_cast<const ContainerNode *>(node)->childType.definedByType == PrimitiveNodeTypes::Null())
    {
      auto container = static_cast<const ContainerNode *>(node);
      std::string nodeTypeString = nodeType.toString();
      std::string childTypeString = container->childType.value.toString();

      size_t size =
        sizeof(AtomicGroupOperation) +
        sizeof(NodeCreateOperation) + nodeTypeString.length() +
        sizeof(SetAttributeOperation) + childTypeString.length();

      auto tsOp = getOpBuffer(size);
      auto groupOp = reinterpret_cast<AtomicGroupOperation *>(&tsOp->op);
      groupOp->type = OperationType::AtomicGroupOperation;
      groupOp->length = size - sizeof(AtomicGroupOperation);
      auto createOp = reinterpret_cast<NodeCreateOperation *>(&groupOp->data);
      createOp->type = OperationType::NodeCreateOperation;
      createOp->nodeTypeLength = nodeTypeString.length();
      auto attrOp = reinterpret_cast<SetAttributeOperation *>(
        reinterpret_cast<char *>(&groupOp->data) +
        sizeof(NodeCreateOperation) + nodeTypeString.length());
      attrOp->type = OperationType::SetAttributeOperation;
      attrOp->attributeId = ContainerNode::AttributeType::ChildType;
      attrOp->length = childTypeString.length();

      std::memcpy(createOp->data, nodeTypeString.c_str(), nodeTypeString.length());
      std::memcpy(attrOp->data, childTypeString.c_str(), childTypeString.length());

      logStream.write(*reinterpret_cast<RefCounted<const LogOperation> *>(&tsOp));
    }
    else
    {
      std::string nodeTypeString = nodeType.toString();

      size_t size = sizeof(NodeCreateOperation) + nodeTypeString.length();
      auto tsOp = getOpBuffer(size);
      auto op = reinterpret_cast<NodeCreateOperation *>(&tsOp->op);
      op->type = OperationType::NodeCreateOperation;
      op->nodeTypeLength = nodeTypeString.length();
      std::memcpy(op->data, nodeTypeString.c_str(), nodeTypeString.length());
      logStream.write(*reinterpret_cast<RefCounted<const LogOperation> *>(&tsOp));
    }
  }
  else
  {
    auto rootId = TypeLogGenerator::generateTypeLog(core, addedNodes, { nodeId.ts, node->createdByRootOffset }, ts, logStream);
    nodeIdTransformed = { rootId.ts, rootId.child + (nodeId.child - node->createdByRootOffset) };
  }

  addedNodes[nodeId] = nodeIdTransformed;

  if (PrimitiveNodeTypes::isValueNodeType(baseType))
  {
    bool hasAllRootTypeData = !nodeId.isInherited() ||
      addedNodes.find(nodeId.getInheritanceRoot()) != addedNodes.end();

    auto generateValueSetOp = [&logStream, &ts, &nodeId, hasAllRootTypeData, &nodeIdTransformed]<typename T>(const ValueNode<T> * valueNode)
    {
      if (valueNode->value.isModifiedSince(nodeId.ts) ||
        (!hasAllRootTypeData && valueNode->value.isModified()))
      {
        T value = valueNode->value.getValue();

        ++ts;
        constexpr size_t size = sizeof(ValueSetOperation) + Value<T>::valueSize();
        auto tsOp = getOpBuffer(size);
        auto op = reinterpret_cast<ValueSetOperation *>(&tsOp->op);
        op->type = OperationType::ValueSetOperation;
        op->nodeId = nodeIdTransformed;
        op->length = Value<T>::valueSize();
        std::memcpy(op->data, &value, op->length);
        logStream.write(*reinterpret_cast<RefCounted<const LogOperation> *>(&tsOp));
      }
    };

    auto valueType = PrimitiveNodeTypes::nodeTypeToPrimitiveType(baseType);
    switch (valueType)
    {
      case PrimitiveNodeTypes::PrimitiveType::BoolValue:
        generateValueSetOp(static_cast<const ValueNode<bool> *>(node));
        break;
      case PrimitiveNodeTypes::PrimitiveType::DoubleValue:
        generateValueSetOp(static_cast<const ValueNode<double> *>(node));
        break;
      case PrimitiveNodeTypes::PrimitiveType::FloatValue:
        generateValueSetOp(static_cast<const ValueNode<float> *>(node));
        break;
      case PrimitiveNodeTypes::PrimitiveType::Int32Value:
        generateValueSetOp(static_cast<const ValueNode<int32_t> *>(node));
        break;
      case PrimitiveNodeTypes::PrimitiveType::Int64Value:
        generateValueSetOp(static_cast<const ValueNode<int64_t> *>(node));
        break;
      case PrimitiveNodeTypes::PrimitiveType::Int8Value:
        generateValueSetOp(static_cast<const ValueNode<int8_t> *>(node));
        break;
      default:
        break;
    }
  }
  else if (PrimitiveNodeTypes::isBlockValueNodeType(baseType))
  {
    if (baseType == PrimitiveNodeTypes::StringValue())
    {
      auto blockValueNode = static_cast<const BlockValueNode<char> *>(node);
      const BlockData<char> * data = blockValueNode->value.getChildren();
      std::string currentBlock;
      uint32_t offset = 0;

      std::function<void ()> flushBlock = [&logStream, &ts, &nodeIdTransformed, &currentBlock, &offset]()
      {
        if (currentBlock.size() == 0)
        {
          return;
        }

        ++ts;
        size_t size = sizeof(BlockValueInsertAfterOperation) + currentBlock.length();
        auto tsOp = getOpBuffer(size);
        auto op = reinterpret_cast<BlockValueInsertAfterOperation *>(&tsOp->op);
        op->type = OperationType::BlockValueInsertAfterOperation;
        op->nodeId = nodeIdTransformed;
        op->blockId = (offset == 0) ? Timestamp::Null : nodeIdTransformed.ts;
        op->offset = offset;
        op->length = currentBlock.length();
        std::memcpy(op->data, currentBlock.c_str(), currentBlock.length());
        logStream.write(*reinterpret_cast<RefCounted<const LogOperation> *>(&tsOp));

        currentBlock.clear();
      };

      bool hasAllRootTypeData = !nodeId.isInherited() ||
        addedNodes.find(nodeId.getInheritanceRoot()) != addedNodes.end();
      bool mightHaveAnyRootTypeData = nodeIdTransformed.isInherited() ||
        !PrimitiveNodeTypes::isPrimitiveNodeType(node->getType());

      if (mightHaveAnyRootTypeData && !hasAllRootTypeData)
      {
        //for now, since it can't be determined what data we have, just delete
        //any inherited data
        auto inheritedBlock = blockValueNode->value.getExistingBlock(nodeId.ts);
        if (inheritedBlock != nullptr)
        {
          ++ts;
          size_t size = sizeof(BlockValueDeleteAfterOperation);
          auto tsOp = getOpBuffer(size);
          auto op = reinterpret_cast<BlockValueDeleteAfterOperation *>(&tsOp->op);
          op->type = OperationType::BlockValueDeleteAfterOperation;
          op->nodeId = nodeIdTransformed;
          op->blockId = nodeIdTransformed.ts;
          op->offset = 0;
          op->length = BlockData<char>::maxLength;
          logStream.write(*reinterpret_cast<RefCounted<const LogOperation> *>(&tsOp));
        }
      }

      while (data != nullptr)
      {
        if (data->effect.isVisible())
        {
          if (data->id == nodeId.ts && hasAllRootTypeData)
          {
            flushBlock();
            offset += data->length;
          }
          else
          {
            currentBlock.append(data->value, data->length);
          }
        }
        else
        {
          if (data->id == nodeId.ts && hasAllRootTypeData)
          {
            ++ts;
            size_t size = sizeof(BlockValueDeleteAfterOperation);
            auto tsOp = getOpBuffer(size);
            auto op = reinterpret_cast<BlockValueDeleteAfterOperation *>(&tsOp->op);
            op->type = OperationType::BlockValueDeleteAfterOperation;
            op->nodeId = nodeIdTransformed;
            op->blockId = nodeIdTransformed.ts;
            op->offset = data->offset;
            op->length = data->length;
            logStream.write(*reinterpret_cast<RefCounted<const LogOperation> *>(&tsOp));

            offset += data->length;
          }
        }

        data = data->nextSibling;
      }

      flushBlock();
    }
  }
  else if (PrimitiveNodeTypes::isContainerNodeType(baseType))
  {
    if (baseType == PrimitiveNodeTypes::Set())
    {
      auto setNode = static_cast<const SetNode *>(node);

      EdgeId prevEdgeId = EdgeId::Null;

      //the edge list must be reversed in order to preserve the set insert order
      std::vector<const SetEdge *> edges;
      for (auto edge = setNode->children; edge != nullptr; edge = edge->next)
      {
        edges.push_back(edge);
      }

      for (auto it = edges.rbegin(); it != edges.rend(); it++)
      {
        const SetEdge * edge = *it;

        //ignore speculative edges
        if (edge->childId.isPending())
        {
          continue;
        }

        NodeId childIdTransformed = TypeLogGenerator::generateTypeLog(core, addedNodes, edge->childId, ts, logStream);

        if (edge->edgeId.isInherited() &&
          addedNodes.find({ edge->edgeId.ts, edge->createdByRootOffset }) != addedNodes.end())
        {
          //already added by inheritance
          NodeId edgeRootNodeId = TypeLogGenerator::generateTypeLog(core, addedNodes, { edge->edgeId.ts, edge->createdByRootOffset }, ts, logStream);
          prevEdgeId = { edgeRootNodeId.ts, edgeRootNodeId.child + edge->edgeId.child - edge->createdByRootOffset };
          continue;
        }

        ++ts;

        size_t size = sizeof(EdgeCreateOperation);

        auto tsOp = getOpBuffer(size);
        auto createOp = reinterpret_cast<EdgeCreateOperation *>(&tsOp->op);
        createOp->type = OperationType::EdgeCreateOperation;
        createOp->parentId = nodeIdTransformed;
        createOp->childId = childIdTransformed;

        logStream.write(*reinterpret_cast<RefCounted<const LogOperation> *>(&tsOp));

        prevEdgeId = NodeId::inheritanceRootFor(ts);
      }
    }
    else if (baseType == PrimitiveNodeTypes::List())
    {
      auto listNode = static_cast<const ListNode *>(node);

      EdgeId prevEdgeId = EdgeId::Null;

      for (auto edge = listNode->children; edge != nullptr; edge = edge->nextChild)
      {
        //ignore speculative edges
        if (edge->childId.isPending())
        {
          continue;
        }

        NodeId childIdTransformed = TypeLogGenerator::generateTypeLog(core, addedNodes, edge->childId, ts, logStream);

        if (edge->edgeId.isInherited() &&
          addedNodes.find({ edge->edgeId.ts, edge->createdByRootOffset }) != addedNodes.end())
        {
          //already added by inheritance
          NodeId edgeRootNodeId = TypeLogGenerator::generateTypeLog(core, addedNodes, { edge->edgeId.ts, edge->createdByRootOffset }, ts, logStream);
          prevEdgeId = { edgeRootNodeId.ts, edgeRootNodeId.child + edge->edgeId.child - edge->createdByRootOffset };
          continue;
        }

        ++ts;

        size_t size =
          sizeof(AtomicGroupOperation) +
          sizeof(EdgeCreateOperation) +
          sizeof(SetAttributeOperation) + sizeof(EdgeId);

        auto tsOp = getOpBuffer(size);
        auto groupOp = reinterpret_cast<AtomicGroupOperation *>(&tsOp->op);
        groupOp->type = OperationType::AtomicGroupOperation;
        groupOp->length = size - sizeof(AtomicGroupOperation);
        auto createOp = reinterpret_cast<EdgeCreateOperation *>(&groupOp->data);
        createOp->type = OperationType::EdgeCreateOperation;
        createOp->parentId = nodeIdTransformed;
        createOp->childId = childIdTransformed;
        auto attrOp = reinterpret_cast<SetAttributeOperation *>(
          reinterpret_cast<char *>(&groupOp->data) +
          sizeof(EdgeCreateOperation));
        attrOp->type = OperationType::SetAttributeOperation;
        attrOp->attributeId = ContainerNode::AttributeType::ChildType;
        attrOp->length = sizeof(EdgeId);
        std::memcpy(attrOp->data, &prevEdgeId, sizeof(EdgeId));

        logStream.write(*reinterpret_cast<RefCounted<const LogOperation> *>(&tsOp));

        prevEdgeId = NodeId::inheritanceRootFor(ts);
      }
    }
    else if (baseType == PrimitiveNodeTypes::Map())
    {
      auto mapNode = static_cast<const MapNode *>(node);
      for (auto it = mapNode->children.begin(); it != mapNode->children.end(); ++it)
      {
        // find the first non-pending item
        MapEdge * edge = nullptr;
        for (auto & edgeIt : it->second)
        {
          if (!edgeIt->childId.isPending())
          {
            edge = edgeIt;
            break;
          }
        }

        if (edge == nullptr)
        {
          continue;
        }

        NodeId childIdTransformed = TypeLogGenerator::generateTypeLog(core, addedNodes, edge->childId, ts, logStream);

        if (edge->edgeId.isInherited() &&
          addedNodes.find({ edge->edgeId.ts, edge->createdByRootOffset }) != addedNodes.end())
        {
          //already added by inheritance
          continue;
        }

        ++ts;

        size_t size =
          sizeof(AtomicGroupOperation) +
          sizeof(EdgeCreateOperation) +
          sizeof(SetAttributeOperation) + edge->key.length();

        auto tsOp = getOpBuffer(size);
        auto groupOp = reinterpret_cast<AtomicGroupOperation *>(&tsOp->op);
        groupOp->type = OperationType::AtomicGroupOperation;
        groupOp->length = size - sizeof(AtomicGroupOperation);
        auto createOp = reinterpret_cast<EdgeCreateOperation *>(&groupOp->data);
        createOp->type = OperationType::EdgeCreateOperation;
        createOp->parentId = nodeIdTransformed;
        createOp->childId = childIdTransformed;
        auto attrOp = reinterpret_cast<SetAttributeOperation *>(
          reinterpret_cast<char *>(&groupOp->data) +
          sizeof(EdgeCreateOperation));
        attrOp->type = OperationType::SetAttributeOperation;
        attrOp->attributeId = ContainerNode::AttributeType::ChildType;
        attrOp->length = edge->key.length();
        std::memcpy(attrOp->data, edge->key.c_str(), edge->key.length());

        logStream.write(*reinterpret_cast<RefCounted<const LogOperation> *>(&tsOp));
      }
    }
    else if (baseType == PrimitiveNodeTypes::Reference())
    {
      auto referenceNode = static_cast<const ReferenceNode *>(node);

      for (auto edge = referenceNode->children; edge != nullptr; edge = edge->next)
      {
        if (edge->childId.isPending() || !edge->effect.isVisible())
        {
          continue;
        }

        NodeId childIdTransformed = TypeLogGenerator::generateTypeLog(core, addedNodes, edge->childId, ts, logStream);

        if (edge->edgeId.isInherited() &&
          addedNodes.find({ edge->edgeId.ts, edge->createdByRootOffset }) != addedNodes.end())
        {
          //already added by inheritance
          break;
        }

        ++ts;

        size_t size = sizeof(EdgeCreateOperation);
        auto tsOp = getOpBuffer(size);
        auto op = reinterpret_cast<EdgeCreateOperation *>(&tsOp->op);
        op->type = OperationType::EdgeCreateOperation;
        op->parentId = nodeIdTransformed;
        op->childId = childIdTransformed;
        logStream.write(*reinterpret_cast<RefCounted<const LogOperation> *>(&tsOp));

        //only add the first visible item
        break;
      }
    }
    else if (baseType == PrimitiveNodeTypes::OrderedFloat64Map())
    {
      auto mapNode = static_cast<const OrderedFloat64MapNode *>(node);
      for (auto it = mapNode->children.begin(); it != mapNode->children.end(); ++it)
      {
        EdgeId edgeId = it->second.front();
        Edge * edge = mapNode->getExistingEdge(edgeId);

        if (edge->childId.isPending())
        {
          continue;
        }

        NodeId childIdTransformed = TypeLogGenerator::generateTypeLog(core, addedNodes, edge->childId, ts, logStream);

        if (edgeId.isInherited() &&
          addedNodes.find({ edgeId.ts, edge->createdByRootOffset }) != addedNodes.end())
        {
          //already added by inheritance
          continue;
        }

        auto mapEdge = static_cast<OrderedFloat64MapEdge *>(edge);

        ++ts;

        size_t size =
          sizeof(AtomicGroupOperation) +
          sizeof(EdgeCreateOperation) +
          sizeof(SetAttributeOperation) + sizeof(double);

        auto tsOp = getOpBuffer(size);
        auto groupOp = reinterpret_cast<AtomicGroupOperation *>(&tsOp->op);
        groupOp->type = OperationType::AtomicGroupOperation;
        groupOp->length = size - sizeof(AtomicGroupOperation);
        auto createOp = reinterpret_cast<EdgeCreateOperation *>(&groupOp->data);
        createOp->type = OperationType::EdgeCreateOperation;
        createOp->parentId = nodeIdTransformed;
        createOp->childId = childIdTransformed;
        auto attrOp = reinterpret_cast<SetAttributeOperation *>(
          reinterpret_cast<char *>(&groupOp->data) +
          sizeof(EdgeCreateOperation));
        attrOp->type = OperationType::SetAttributeOperation;
        attrOp->attributeId = ContainerNode::AttributeType::ChildType;
        attrOp->length = sizeof(double);
        std::memcpy(attrOp->data, &mapEdge->key, sizeof(double));

        logStream.write(*reinterpret_cast<RefCounted<const LogOperation> *>(&tsOp));
      }
    }

    auto containerNode = static_cast<const ContainerNodeImpl<Edge> *>(node);
    for (auto it = containerNode->edges.begin(); it != containerNode->edges.end(); ++it)
    {
      Edge * edge = it->second;

      //delete edges that are inherited but also should be deleted
      if (it->first.isInherited() &&
        addedNodes.find({ it->first.ts, edge->createdByRootOffset }) != addedNodes.end() &&
        edge->effect.isVisible() == false &&
        edge->effect.isInitialized() == true)
      {
        NodeId edgeRootNodeId = TypeLogGenerator::generateTypeLog(core, addedNodes,
          { it->first.ts, edge->createdByRootOffset }, ts, logStream);

        ++ts;
        size_t size = sizeof(EdgeDeleteOperation);
        auto tsOp = getOpBuffer(size);
        auto op = reinterpret_cast<EdgeDeleteOperation *>(&tsOp->op);
        op->type = OperationType::EdgeDeleteOperation;
        op->parentId = nodeIdTransformed;
        op->edgeId = { edgeRootNodeId.ts, edgeRootNodeId.child + it->first.child - edge->createdByRootOffset };
        logStream.write(*reinterpret_cast<RefCounted<const LogOperation> *>(&tsOp));
      }
    }
  }

  return nodeIdTransformed;
}