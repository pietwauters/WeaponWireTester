#include "WebTerminal.h"

#include "terminal.html.h"  // HTML page

WebTerminal::WebTerminal(AsyncWebServer& srv) : ws("/ws"), server(srv) {}

void WebTerminal::begin() {
    server.addHandler(&ws);

    server.on("/", HTTP_GET,
              [this](AsyncWebServerRequest* request) { request->send(200, "text/html", FPSTR(terminal_html)); });

    ws.onEvent([this](AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data,
                      size_t len) {
        if (type == WS_EVT_DATA) {
            AwsFrameInfo* info = (AwsFrameInfo*)arg;
            if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
                String msg = "";
                for (size_t i = 0; i < len; i++) {
                    msg += (char)data[i];
                }
                handleCommand(msg);
            }
        }
    });
}

void WebTerminal::loop() {}

void WebTerminal::send(const String& msg) {
    for (auto& client : ws.getClients()) {
        if (client.status() == WS_CONNECTED && client.canSend()) {
            client.text(msg);
        }
    }
}

void WebTerminal::printf(const char* format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    send(String(buffer));
}

void WebTerminal::registerCommand(const String& name, CommandCallback callback) {
    commands.push_back({name, callback});
}

void WebTerminal::handleCommand(const String& input) {
    if (input.length() == 0)
        return;

    int start = 0;
    int end = input.indexOf(' ');
    String cmd = (end == -1) ? input : input.substring(0, end);

    std::vector<String> args;
    while (end != -1) {
        start = end + 1;
        end = input.indexOf(' ', start);
        args.push_back(input.substring(start, end == -1 ? input.length() : end));
    }

    for (const auto& c : commands) {
        if (c.name == cmd) {
            c.callback(this, args);
            return;
        }
    }

    printf("[!] Unknown command: %s\n", cmd.c_str());
}

void WebTerminal::println(const String& message) { send(message + "\n"); }
