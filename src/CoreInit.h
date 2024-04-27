#pragma once
#include "Nodes/Node.h"
#include "Operation.h"
#include "Event.h"
#include "LogOperation.h"
#include <string>

using getTypeSpecFn = std::function<void(const std::string & type)>;
using eventRaisedFn = std::function<void(const Event & event)>;

class Core;

class CoreInit
{
public:
  CoreInit()
  {
    getTypeSpec = [](const std::string & type){};
    eventRaised = [](const Event & event){};
  }
  CoreInit(getTypeSpecFn getTypeSpec, eventRaisedFn eventRaised)
    : getTypeSpec(getTypeSpec), eventRaised(eventRaised) {}

protected:
  getTypeSpecFn getTypeSpec;
  eventRaisedFn eventRaised;

  friend class Core;
};