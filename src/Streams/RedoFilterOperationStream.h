#pragma once
#include "IReadableStream.h"
#include "ReadableStreamBase.h"
#include "IWritableStream.h"
#include "../LogOperation.h"
#include "../RefCounted.h"
#include <unordered_map>

class RedoFilterOperationStream final :
  public IWritableStream<RefCounted<const LogOperation>>,
  public ReadableStreamBase<RefCounted<const LogOperation>>
{
public:
  RedoFilterOperationStream(uint32_t siteId)
    : siteId(siteId) {}

  bool write(const RefCounted<const LogOperation> & data) override;
  void close() override;

private:
  uint32_t siteId;
  bool hasOp = false;
  std::unordered_map<Timestamp, int32_t> opEffect;
};