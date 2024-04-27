#include "WriteFilterOperationStream.h"

bool WriteFilterOperationStream::write(const RefCounted<const LogOperation> & data)
{
  if (data->op.type == OperationType::ValuePreviewOperation)
  {
    return true;
  }

  return writeToDestination(data);
}

void WriteFilterOperationStream::close()
{

}