#include "RedoFilterOperationStream.h"

bool RedoFilterOperationStream::write(const RefCounted<const LogOperation> & data)
{
  if (hasOp)
  {
    return false;
  }

  if (data->ts.site != siteId)
  {
    return true;
  }

  std::unordered_map<Timestamp, int32_t>::iterator it;

  if (data->op.isUndo())
  {
    auto undoOp = reinterpret_cast<const UndoGroupOperation *>(&data->op);
    it = opEffect.insert(std::make_pair(undoOp->prevTs, 0)).first;
    --std::get<1>(*it);
  }
  else if (data->op.isRedo())
  {
    auto undoOp = reinterpret_cast<const UndoGroupOperation *>(&data->op);
    it = opEffect.insert(std::make_pair(undoOp->prevTs, 0)).first;
    ++std::get<1>(*it);
    return true;
  }
  else
  {
    return false;
  }

  if (std::get<1>(*it) < 0)
  {
    writeToDestination(data);
    hasOp = true;
    return false;
  }

  return true;
}

void RedoFilterOperationStream::close()
{
  
}