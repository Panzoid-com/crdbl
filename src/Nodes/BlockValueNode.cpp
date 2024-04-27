#include "BlockValueNode.h"

template <class T>
void BlockValueNode<T>::serialize(IObjectSerializer & serializer) const
{
  Node::serialize(serializer);
}

template class BlockValueNode<char>;