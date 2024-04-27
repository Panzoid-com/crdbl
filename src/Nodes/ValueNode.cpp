#include "ValueNode.h"

template <class T>
void ValueNode<T>::serialize(IObjectSerializer & serializer) const
{
  Node::serialize(serializer);
}

template class ValueNode<int32_t>;
template class ValueNode<int64_t>;
template class ValueNode<float>;
template class ValueNode<double>;
template class ValueNode<int8_t>;
template class ValueNode<bool>;