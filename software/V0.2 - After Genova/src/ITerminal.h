#pragma once

#include <Arduino.h>

class ITerminal {
   public:
    virtual ~ITerminal() = default;
    virtual void printf(const char* format, ...) = 0;
    virtual void send(const String& message) = 0;
    virtual void println(const String& message) = 0;
};
