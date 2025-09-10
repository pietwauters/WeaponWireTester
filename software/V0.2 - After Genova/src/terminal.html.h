#pragma once

const char terminal_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Web Terminal</title>
    <style>
        body { font-family: monospace; background: #111; color: #0f0; padding: 10px; }
        #output { height: 90vh; overflow-y: scroll; white-space: pre-wrap; }
        input { width: 100%; background: #000; color: #0f0; border: none; padding: 10px; }
    </style>
</head>
<body>
    <div id="output"></div>
    <input id="input" placeholder="Type command...">
    <script>
        const input = document.getElementById('input');
        const output = document.getElementById('output');
        const ws = new WebSocket('ws://' + location.host + '/ws');

        ws.onmessage = (e) => {
            output.innerHTML += e.data.replace(/\n/g, '<br>') + '<br>';
            output.scrollTop = output.scrollHeight;
        };

        input.addEventListener('keydown', (e) => {
            if (e.key === 'Enter') {
                ws.send(input.value);
                input.value = '';
            }
        });
    </script>
</body>
</html>
)rawliteral";
