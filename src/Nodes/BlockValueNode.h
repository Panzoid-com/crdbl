#include "Node.h"
#include "BlockValue.h"
#include "IObjectSerializer.h"

template <class T>
class BlockValueNode : public Node
{
public:
  BlockValue<T> value;

  void serialize(IObjectSerializer & serializer) const;
};