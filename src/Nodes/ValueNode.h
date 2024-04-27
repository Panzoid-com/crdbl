#pragma once
#include "Node.h"
#include "Value.h"
#include "IObjectSerializer.h"

template <class T>
class ValueNode : public Node
{
public:
  Value<T> value;

  void serialize(IObjectSerializer & serializer) const;
};