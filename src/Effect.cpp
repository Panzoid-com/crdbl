#include "Effect.h"

void Effect::increment()
{
  effect += 1;
}

void Effect::decrement()
{
  effect -= 1;
}

void Effect::operator+=(int32_t delta)
{
  effect += delta;
}

void Effect::initialize()
{
  if (initialized)
    return;

  initialized = true;
  effect += 1;
}

void Effect::deinitialize()
{
  if (!initialized)
    return;

  initialized = false;
  effect -= 1;
}

void Effect::reset()
{
  initialized = false;
  effect = 0;
}

bool Effect::isVisible() const
{
  return effect > 0 && initialized;
}

bool Effect::isInitialized() const
{
  return initialized;
}

std::string Effect::toString() const
{
  std::string out;
  out += std::to_string(effect);
  out += ":";
  out += std::to_string(initialized);
  out += " ";
  out += std::to_string(isVisible());
  return out;
}