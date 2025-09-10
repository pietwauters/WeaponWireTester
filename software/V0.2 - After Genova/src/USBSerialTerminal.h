#ifndef USBSERIALTERMINAL_H
#define USBSERIALTERMINAL_H

#include <functional>
#include <map>
#include <vector>

#include "ITerminal.h"

typedef std::function<void(ITerminal*, const std::vector<String>&)> CommandCallback;

class USBSerialTerminal : public ITerminal {
   private:
    std::map<String, CommandCallback> commands;
    String inputBuffer;
    bool echoEnabled;
    String prompt;

    void processCommand(const String& command);
    std::vector<String> parseCommand(const String& command);
    void showPrompt();

   public:
    USBSerialTerminal();

    // ITerminal interface implementation
    void printf(const char* format, ...) override;
    void send(const String& message) override;
    void println(const String& message) override;

    // Terminal management
    void begin();
    void loop();
    void registerCommand(const String& command, CommandCallback callback);
    void setEcho(bool enabled) { echoEnabled = enabled; }
    void setPrompt(const String& newPrompt) { prompt = newPrompt; }
};

#endif