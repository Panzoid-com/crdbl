#pragma once
#include "IReadableStream.h"
#include "ReadableStreamBase.h"
#include "IWritableStream.h"
#include "../LogOperation.h"
#include "../RefCounted.h"

class WriteFilterOperationStream final :
  public IWritableStream<RefCounted<const LogOperation>>,
  public ReadableStreamBase<RefCounted<const LogOperation>>
{
public:
  WriteFilterOperationStream() {}

  bool write(const RefCounted<const LogOperation> & data) override;
  void close() override;
};