#pragma once
#include <string>
#include <type_traits>
#include "NodeId.h"
#include "EdgeId.h"
#include "IObjectSerializer.h"

class Event
{
public:
  virtual ~Event() = default;

  virtual void serialize(IObjectSerializer & serializer) const = 0;
  virtual Event * clone() const = 0;
};

template<typename Base, typename Derived>
class Cloneable : public Base {
public:
    static_assert(std::is_base_of<Event, Base>::value, "Base must be a base of Derived");

    Base * clone() const override {
        return new Derived(static_cast<Derived const&>(*this));
    }
};

//raised when all references to a node are removed (possibly could rename to make that more clear)
//this is so listeners can release/garbage collect a node that is not being used
//NOTE: this isn't currently implemented
class NodeDeletedEvent
{
public:
  NodeId nodeId;

  void serialize(IObjectSerializer & serializer) const;
};

class EdgeEvent : public Event
{
public:
  NodeId parentId;
  EdgeId edgeId;
  NodeId childId;
  bool speculative;

  virtual ~EdgeEvent() = default;
  virtual void serialize(IObjectSerializer & serializer) const = 0;
  virtual Event * clone() const = 0;
};

class NodeAddedEvent : public Cloneable<EdgeEvent, NodeAddedEvent>
{
public:
  void serialize(IObjectSerializer & serializer) const;
};

class NodeAddedEventOrdered : public Cloneable<EdgeEvent, NodeAddedEventOrdered>
{
public:
  size_t index;
  size_t actualIndex;

  void serialize(IObjectSerializer & serializer) const;
};

class NodeAddedEventMapped : public Cloneable<EdgeEvent, NodeAddedEventMapped>
{
public:
  std::string key;

  void serialize(IObjectSerializer & serializer) const;
};

class NodeRemovedEvent : public Cloneable<EdgeEvent, NodeRemovedEvent>
{
public:
  void serialize(IObjectSerializer & serializer) const;
};

class NodeRemovedEventOrdered : public Cloneable<EdgeEvent, NodeRemovedEventOrdered>
{
public:
  size_t index;
  size_t actualIndex;

  void serialize(IObjectSerializer & serializer) const;
};

class NodeRemovedEventMapped : public Cloneable<EdgeEvent, NodeRemovedEventMapped>
{
public:
  std::string key;

  void serialize(IObjectSerializer & serializer) const;
};

template <class T>
class NodeValueChangedEvent : public Cloneable<Event, NodeValueChangedEvent<T>>
{
public:
  NodeId nodeId;
  T oldValue;
  T newValue;

  void serialize(IObjectSerializer & serializer) const;
};

class NodeBlockValueEvent : public Event
{
public:
  Timestamp ts;
  NodeId nodeId;
  uint32_t offset;

  virtual ~NodeBlockValueEvent() = default;
  virtual void serialize(IObjectSerializer & serializer) const = 0;
  virtual Event * clone() const = 0;
};

class NodeBlockValueInsertedEvent : public Cloneable<NodeBlockValueEvent, NodeBlockValueInsertedEvent>
{
public:
  const char * str;
  uint32_t length;

  void serialize(IObjectSerializer & serializer) const;
};

class NodeBlockValueDeletedEvent : public Cloneable<NodeBlockValueEvent, NodeBlockValueDeletedEvent>
{
public:
  uint32_t length;

  void serialize(IObjectSerializer & serializer) const;
};
