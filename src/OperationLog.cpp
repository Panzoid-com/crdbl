#include "OperationLog.h"
#include "OperationIterator.h"
#include "Streams/CallbackWritableStream.h"

Timestamp OperationLog::GetFinalTimestamp(const LogOperation & op)
{
  if (op.op.type == OperationType::ValuePreviewOperation)
  {
    return Timestamp::Null;
  }

  Timestamp ts;

  if (op.op.type == OperationType::GroupOperation)
  {
    auto groupOp = reinterpret_cast<const GroupOperation *>(&op.op);
    OperationIterator groupIt(
      reinterpret_cast<const Operation *>(&groupOp->data), groupOp->length);

    if (*groupIt == nullptr)
    {
      return Timestamp::Null;
    }

    ts = op.ts;
    //the first child operation has the same clock as the group
    ts.clock--;

    while (*groupIt)
    {
      ts.clock++;
      ++groupIt;
    }
  }
  else if (op.op.type == OperationType::UndoGroupOperation ||
    op.op.type == OperationType::RedoGroupOperation)
  {
    auto groupOp = reinterpret_cast<const UndoGroupOperation *>(&op.op);
    OperationIterator groupIt(
      reinterpret_cast<const Operation *>(&groupOp->data), groupOp->length);

    if (*groupIt == nullptr)
    {
      return Timestamp::Null;
    }

    ts = op.ts;
    //the first child operation has the same clock as the group
    ts.clock--;

    while (*groupIt)
    {
      ts.clock++;
      ++groupIt;
    }
  }
  else
  {
    ts = op.ts;
  }

  return ts;
}

const VectorTimestamp & OperationLog::getVectorClock() const
{
  return clock;
}

void OperationLog::applyOperation(const RefCounted<const LogOperation> & op)
{
  if (op->ts.clock > clock.getMaxClock() + 1)
  {
    // NOTE: this check is disabled for now, but perhaps it could be optional
    // in certain contexts, we might want to enforce that ops received from
    // a given site are strictly sequential; this e.g. prevents abuse by maxing
    // out the clock
    // however, it's a normal pattern to filter/skip certain ops, leaving gaps
    // that would be blocked by this check

    // return;
  }

  if (!(clock < op->ts))
  {
    // we've presumably already seen this op
    return;
  }

  if (op->op.type != OperationType::ValuePreviewOperation)
  {
    Timestamp ts = OperationLog::GetFinalTimestamp(*op);
    clock.update(ts);
  }

  for (destIter = readStreams.begin(); destIter != readStreams.end(); ++destIter)
  {
    destIter->writeToDestination(op);
  }
}

IWritableStream<RefCounted<const LogOperation>> * OperationLog::createApplyStream()
{
  auto * callbackStream = new CallbackWritableStream<RefCounted<const LogOperation>>(
    [&](const RefCounted<const LogOperation> & op)
    {
      applyOperation(op);
    },
    []()
    {

    });

  return callbackStream;
}

IReadableStream<RefCounted<const LogOperation>> * OperationLog::createReadStream()
{
  readStreams.push_back(ReadableStreamBase<RefCounted<const LogOperation>>{});
  return &readStreams.back();
}

void OperationLog::cancelReadStream(IReadableStream<RefCounted<const LogOperation>> * readStream)
{
  for (auto it = readStreams.begin(); it != readStreams.end(); ++it)
  {
    if (&(*it) == readStream)
    {
      if (destIter == it)
      {
        destIter = std::prev(it);
      }
      readStreams.erase(it);
      break;
    }
  }
}