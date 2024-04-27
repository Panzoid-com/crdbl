#pragma once
#include "IReadableStream.h"
#include "ReadableStreamBase.h"
#include "IWritableStream.h"
#include "../OperationFilter.h"
#include "../LogOperation.h"
#include "../RefCounted.h"

class FilterOperationStream final :
  public IWritableStream<RefCounted<const LogOperation>>,
  public ReadableStreamBase<RefCounted<const LogOperation>>
{
public:
  bool write(const RefCounted<const LogOperation> & data) override;
  void close() override;
  OperationFilter & getFilter();
private:
  OperationFilter filter;
};