#pragma once
#include "VectorTimestamp.h"
#include "LogOperation.h"
#include "Streams/IReadableStream.h"
#include "Streams/IWritableStream.h"
#include "Streams/ReadableStreamBase.h"
#include "RefCounted.h"
#include <list>

class OperationLog
{
public:
  void applyOperation(const RefCounted<const LogOperation> & op);
  IWritableStream<RefCounted<const LogOperation>> * createApplyStream();

  IReadableStream<RefCounted<const LogOperation>> * createReadStream();
  void cancelReadStream(IReadableStream<RefCounted<const LogOperation>> * readStream);

  const VectorTimestamp & getVectorClock() const;

  static Timestamp GetFinalTimestamp(const LogOperation & op);

private:
  VectorTimestamp clock;
  std::list<ReadableStreamBase<RefCounted<const LogOperation>>> readStreams;
  std::list<ReadableStreamBase<RefCounted<const LogOperation>>>::iterator destIter;
};