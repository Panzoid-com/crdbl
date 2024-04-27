#include "Event.h"

void NodeDeletedEvent::serialize(IObjectSerializer & serializer) const
{
  serializer.startObject();
  serializer.addPair("eventType", "NodeDeleted");
  serializer.addPair("nodeId", nodeId);
  serializer.endObject();
}

void NodeAddedEvent::serialize(IObjectSerializer & serializer) const
{
  serializer.startObject();
  serializer.addPair("eventType", "NodeAdded");
  serializer.addPair("parentId", parentId);
  serializer.addPair("childId", childId);
  serializer.addPair("edgeId", edgeId);
  serializer.addPair("speculative", speculative);
  serializer.endObject();
}

void NodeAddedEventOrdered::serialize(IObjectSerializer & serializer) const
{
  serializer.startObject();
  serializer.addPair("eventType", "NodeAdded");
  serializer.addPair("parentId", parentId);
  serializer.addPair("childId", childId);
  serializer.addPair("edgeId", edgeId);
  serializer.addPair("speculative", speculative);
  serializer.addPair("index", index);
  serializer.addPair("actualIndex", actualIndex);
  serializer.endObject();
}

void NodeAddedEventMapped::serialize(IObjectSerializer & serializer) const
{
  serializer.startObject();
  serializer.addPair("eventType", "NodeAdded");
  serializer.addPair("parentId", parentId);
  serializer.addPair("childId", childId);
  serializer.addPair("edgeId", edgeId);
  serializer.addPair("speculative", speculative);
  serializer.addPair("key", key);
  serializer.endObject();
}

void NodeRemovedEvent::serialize(IObjectSerializer & serializer) const
{
  serializer.startObject();
  serializer.addPair("eventType", "NodeRemoved");
  serializer.addPair("parentId", parentId);
  serializer.addPair("childId", childId);
  serializer.addPair("edgeId", edgeId);
  serializer.addPair("speculative", speculative);
  serializer.endObject();
}

void NodeRemovedEventOrdered::serialize(IObjectSerializer & serializer) const
{
  serializer.startObject();
  serializer.addPair("eventType", "NodeRemoved");
  serializer.addPair("parentId", parentId);
  serializer.addPair("childId", childId);
  serializer.addPair("edgeId", edgeId);
  serializer.addPair("speculative", speculative);
  serializer.addPair("index", index);
  serializer.addPair("actualIndex", actualIndex);
  serializer.endObject();
}

void NodeRemovedEventMapped::serialize(IObjectSerializer & serializer) const
{
  serializer.startObject();
  serializer.addPair("eventType", "NodeRemoved");
  serializer.addPair("parentId", parentId);
  serializer.addPair("childId", childId);
  serializer.addPair("edgeId", edgeId);
  serializer.addPair("speculative", speculative);
  serializer.addPair("key", key);
  serializer.endObject();
}

template <class T>
void NodeValueChangedEvent<T>::serialize(IObjectSerializer & serializer) const
{
  serializer.startObject();
  serializer.addPair("eventType", "NodeValueChanged");
  serializer.addPair("id", nodeId);
  serializer.addPair("value", newValue);
  serializer.addPair("oldValue", oldValue);
  serializer.endObject();
}

void NodeBlockValueInsertedEvent::serialize(IObjectSerializer & serializer) const
{
  serializer.startObject();
  serializer.addPair("ts", ts);
  serializer.addPair("eventType", "NodeBlockValueInserted");
  serializer.addPair("id", nodeId);
  serializer.addPair("offset", offset);
  serializer.addPair("value", std::string(str, length));
  serializer.endObject();
}

void NodeBlockValueDeletedEvent::serialize(IObjectSerializer & serializer) const
{
  serializer.startObject();
  serializer.addPair("ts", ts);
  serializer.addPair("eventType", "NodeBlockValueDeleted");
  serializer.addPair("id", nodeId);
  serializer.addPair("offset", offset);
  serializer.addPair("length", length);
  serializer.endObject();
}

template class NodeValueChangedEvent<bool>;
template class NodeValueChangedEvent<double>;
template class NodeValueChangedEvent<float>;
template class NodeValueChangedEvent<int32_t>;
template class NodeValueChangedEvent<int64_t>;
template class NodeValueChangedEvent<int8_t>;