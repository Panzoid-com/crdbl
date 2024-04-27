#pragma once
#include <cstdint>

#include <string>

class Effect
{
public:
    void increment();
    void decrement();
    void operator+=(int32_t delta);
    void initialize();
    void deinitialize();
    void reset();
    bool isVisible() const;
    bool isInitialized() const;

    std::string toString() const;
private:
    bool initialized = false;
    int32_t effect = 0;
};