#include "USBSerialTerminal.h"

#include <stdarg.h>

USBSerialTerminal::USBSerialTerminal() : echoEnabled(true), prompt("tester> ") {
    inputBuffer.reserve(256);  // Reserve space for input buffer
}

void USBSerialTerminal::begin() {
    // Serial should already be initialized in main setup()
    Serial.println("\n=== USB Serial Terminal Ready ===");
    showPrompt();
}

void USBSerialTerminal::loop() {
    while (Serial.available()) {
        char c = Serial.read();

        if (c == '\r' || c == '\n') {
            if (inputBuffer.length() > 0) {
                if (echoEnabled) {
                    Serial.println();  // New line after command
                }
                processCommand(inputBuffer);
                inputBuffer = "";
                showPrompt();
            } else if (echoEnabled) {
                Serial.println();
                showPrompt();
            }
        } else if (c == '\b' || c == 127) {  // Backspace or DEL
            if (inputBuffer.length() > 0) {
                inputBuffer.remove(inputBuffer.length() - 1);
                if (echoEnabled) {
                    Serial.print("\b \b");  // Erase character on terminal
                }
            }
        } else if (c >= 32 && c <= 126) {  // Printable characters
            inputBuffer += c;
            if (echoEnabled) {
                Serial.print(c);
            }
        }
        // Ignore other control characters
    }
}

void USBSerialTerminal::printf(const char* format, ...) {
    va_list args;
    va_start(args, format);

    char buffer[512];
    vsnprintf(buffer, sizeof(buffer), format, args);

    // Convert \n to \r\n for proper line endings
    String output = String(buffer);
    output.replace("\n", "\r\n");
    Serial.print(output);

    va_end(args);
}

void USBSerialTerminal::send(const String& message) {
    // Convert \n to \r\n for proper line endings
    String output = message;
    output.replace("\n", "\r\n");
    Serial.print(output);
    Serial.print("\r\n");
}

void USBSerialTerminal::println(const String& message) {
    // Print message with proper CR+LF ending
    String output = message;
    output.replace("\n", "\r\n");
    Serial.print(output);
    Serial.print("\r\n");
}

void USBSerialTerminal::registerCommand(const String& command, CommandCallback callback) {
    commands[command] = callback;
}

void USBSerialTerminal::showPrompt() { Serial.print(prompt); }

void USBSerialTerminal::processCommand(const String& command) {
    std::vector<String> args = parseCommand(command);

    if (args.empty()) {
        return;
    }

    String cmd = args[0];
    cmd.toLowerCase();

    // Remove the command from args, leaving only the arguments
    args.erase(args.begin());

    auto it = commands.find(cmd);
    if (it != commands.end()) {
        it->second(this, args);
    } else {
        printf("Unknown command: %s\n", cmd.c_str());
        printf("Type 'help' for available commands.\n");
    }
}

std::vector<String> USBSerialTerminal::parseCommand(const String& command) {
    std::vector<String> args;
    String current = "";
    bool inQuotes = false;

    for (int i = 0; i < command.length(); i++) {
        char c = command.charAt(i);

        if (c == '"') {
            inQuotes = !inQuotes;
        } else if (c == ' ' && !inQuotes) {
            if (current.length() > 0) {
                args.push_back(current);
                current = "";
            }
        } else {
            current += c;
        }
    }

    if (current.length() > 0) {
        args.push_back(current);
    }

    return args;
}