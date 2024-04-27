#include "ContainerNode.h"

template <class T>
ContainerNodeImpl<T>::~ContainerNodeImpl()
{
  for (auto it = edges.begin(); it != edges.end(); ++it)
  {
    delete it->second;
  }
}

template <class T>
T * ContainerNodeImpl<T>::getEdge(const EdgeId & edgeId)
{
  T * edge = nullptr;

  auto it = edges.find(edgeId);
  if (it != edges.end())
  {
    edge = it->second;
  }

  if (edge == nullptr)
  {
    edge = new T();
    edges[edgeId] = edge;
  }

  return edge;
}

template <class T>
T * ContainerNodeImpl<T>::getExistingEdge(const EdgeId & edgeId) const
{
  T * edge = nullptr;

  auto it = edges.find(edgeId);
  if (it != edges.end())
  {
    edge = it->second;
  }

  return edge;
}

template <class T>
void ContainerNodeImpl<T>::deleteEdge(const EdgeId & edgeId)
{
  auto it = edges.find(edgeId);
  if (it != edges.end())
  {
    delete it->second;
    edges.erase(it);
  }
}

#include "SetNode.h"
#include "MapNode.h"
#include "ListNode.h"
#include "ReferenceNode.h"
#include "OrderedFloat64MapNode.h"
template class ContainerNodeImpl<SetEdge>;
template class ContainerNodeImpl<ListEdge>;
template class ContainerNodeImpl<MapEdge>;
template class ContainerNodeImpl<ReferenceEdge>;
template class ContainerNodeImpl<OrderedFloat64MapEdge>;