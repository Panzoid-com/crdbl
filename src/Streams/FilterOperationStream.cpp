#include "FilterOperationStream.h"

bool FilterOperationStream::write(const RefCounted<const LogOperation> & data)
{
  if (!filter.filter(*data))
  {
    return true;
  }

  return writeToDestination(data);
}

void FilterOperationStream::close()
{

}

OperationFilter & FilterOperationStream::getFilter()
{
  return filter;
}