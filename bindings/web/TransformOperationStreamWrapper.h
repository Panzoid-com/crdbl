#pragma once
#include "ProjectDB.h"
#include <Streams/TransformOperationStream.h>

class TransformOperationStreamWrapper
{
  public:
    static TransformOperationStream * makeTransformOperationStream(bool local = false, const ProjectDB * database = nullptr)
    {
      return new TransformOperationStream(local, &database->core);
    }
};