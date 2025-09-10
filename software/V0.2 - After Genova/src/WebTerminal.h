#pragma once

#include <Arduino.h>
#include <ESPAsyncWebServer.h>

#include <functional>
#include <vector>

#include "ITerminal.h"

class WebTerminal : public ITerminal {
   public:
    WebTerminal(AsyncWebServer& server);

    void begin();
    void loop();
    void send(const String& msg);
    void printf(const char* format, ...) override;
    void handleCommand(const String& input);

    using CommandCallback = std::function<void(ITerminal*, const std::vector<String>&)>;
    void registerCommand(const String& name, CommandCallback callback);

    void println(const String& message) override;

   private:
    AsyncWebSocket ws;
    AsyncWebServer& server;

    struct Command {
        String name;
        CommandCallback callback;
    };

    std::vector<Command> commands;
};
