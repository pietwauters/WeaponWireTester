#include "SettingsManager.h"


static uint32_t hashKey(const String& key) {
    uint32_t hash = 5381;
    for (int i = 0; i < key.length(); i++) {
        hash = ((hash << 5) + hash) + key[i]; // hash * 33 + c
    }
    return hash;
}
String SettingsManager::getPrefKey(const String& key) {
    const size_t MAX_KEY_LENGTH = 15;
    if (key.length() <= MAX_KEY_LENGTH) {
        return key;  // use original key if short
    } else {
        // prefix with k_ to avoid purely numeric keys, convert to uppercase hex
        char buf[11];
        snprintf(buf, sizeof(buf), "k_%08X", hashKey(key));
        return String(buf);
    }
}


void SettingsManager::addBool(const char* key, const char* label, bool* value) {
  settings.push_back({String(key), String(label), BOOL, value, 0});
}

void SettingsManager::addInt(const char* key, const char* label, int* value) {
  settings.push_back({String(key), String(label), INT, value, 0});
}

void SettingsManager::addFloat(const char* key, const char* label, float* value) {
  settings.push_back({String(key), String(label), FLOAT, value, 0});
}

void SettingsManager::addString(const char* key, const char* label, String* value) {
  settings.push_back({String(key), String(label), STRING, value, 0});
}

void SettingsManager::addIntArray(const char* key, const char* label, int* array, size_t size) {
  settings.push_back({String(key), String(label), ARRAY_INT, array, size});
}

void SettingsManager::addFloatArray(const char* key, const char* label, float* array, size_t size) {
  settings.push_back({String(key), String(label), ARRAY_FLOAT, array, size});
}

void SettingsManager::begin(const String& ns) {
  namespaceStr = ns;
  prefs.begin(namespaceStr.c_str(), false);
}

void SettingsManager::load() {
    prefs.begin(namespaceStr.c_str(), false);
    for (auto& s : settings) {
        String key = getPrefKey(s.key);

        if (s.type == BOOL) *(bool*)s.value = prefs.getBool(key.c_str(), false);
        else if (s.type == INT) *(int*)s.value = prefs.getInt(key.c_str(), 0);
        else if (s.type == FLOAT) *(float*)s.value = prefs.getFloat(key.c_str(), 0.0f);
        else if (s.type == STRING) *(String*)s.value = prefs.getString(key.c_str(), "");

        else if (s.type == ARRAY_INT) {
            int* arr = (int*)s.value;
            for (size_t i = 0; i < s.size; i++) {
                String indexedKey = getPrefKey(s.key + "_" + String(i));
                arr[i] = prefs.getInt(indexedKey.c_str(), 0);
            }
        }
        else if (s.type == ARRAY_FLOAT) {
            float* arr = (float*)s.value;
            for (size_t i = 0; i < s.size; i++) {
                String indexedKey = getPrefKey(s.key + "_" + String(i));
                arr[i] = prefs.getFloat(indexedKey.c_str(), 0.0f);
            }
        }
    }
    prefs.end();
}

void SettingsManager::save() {
    prefs.begin(namespaceStr.c_str(), false);
    for (auto& s : settings) {
        String key = getPrefKey(s.key);

        if (s.type == BOOL) prefs.putBool(key.c_str(), *(bool*)s.value);
        else if (s.type == INT) prefs.putInt(key.c_str(), *(int*)s.value);
        else if (s.type == FLOAT) prefs.putFloat(key.c_str(), *(float*)s.value);
        else if (s.type == STRING) prefs.putString(key.c_str(), *(String*)s.value);

        else if (s.type == ARRAY_INT) {
            int* arr = (int*)s.value;
            for (size_t i = 0; i < s.size; i++) {
                String indexedKey = getPrefKey(s.key + "_" + String(i));
                prefs.putInt(indexedKey.c_str(), arr[i]);
            }
        }
        else if (s.type == ARRAY_FLOAT) {
            float* arr = (float*)s.value;
            for (size_t i = 0; i < s.size; i++) {
                String indexedKey = getPrefKey(s.key + "_" + String(i));
                prefs.putFloat(indexedKey.c_str(), arr[i]);
            }
        }
    }
    prefs.end();
    //ESP.restart();
}


uint32_t SettingsManager::fnv1aHash(const String& str) {
  const uint32_t FNV_PRIME = 0x01000193;
  uint32_t hash = 0x811c9dc5;

  for (size_t i = 0; i < str.length(); i++) {
    hash ^= (uint8_t)str[i];
    hash *= FNV_PRIME;
  }
  return hash;
}

String SettingsManager::makeHashedKey(const String& base, size_t index) {
  String full = base + "_" + String(index);
  uint32_t hash = fnv1aHash(full);
  char buf[16];
  snprintf(buf, sizeof(buf), "k_%08X", hash);
  return String(buf);
}

// For completeness: you should implement setupWeb() and generateHTML() to create your webpage,
// including form elements for all your settings, mapping to their keys for get/post handling.
// This code focuses on storage and key handling as requested.


void SettingsManager::setPostSaveCallback(std::function<void()> callback) {
  postSaveCallback = callback;
}

void SettingsManager::addWebEndpoints(AsyncWebServer& server) {
  server.on("/settings", HTTP_GET, [this](AsyncWebServerRequest* request) {
  String html = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
      <meta charset="UTF-8">
      <meta name="viewport" content="width=device-width, initial-scale=1">
      <title>ESP32 Settings</title>
      <style>
        body {
          font-family: sans-serif;
          margin: 20px;
          max-width: 600px;
          width: 100%;
        }
        form {
          display: flex;
          flex-direction: column;
          gap: 12px;
        }
        label {
          display: flex;
          flex-direction: column;
          font-size: 1rem;
        }
        input[type="text"],
        input[type="number"],
        input[type="checkbox"] {
          font-size: 1rem;
          padding: 8px;
          width: 100%;
          box-sizing: border-box;
        }
        input[type="submit"] {
          padding: 10px;
          font-size: 1rem;
          background-color: #4CAF50;
          color: white;
          border: none;
          cursor: pointer;
        }
        input[type="submit"]:hover {
          background-color: #45a049;
        }
      </style>
    </head>
    <body>
      <h2>ESP32 Settings</h2>
      <form method='POST' action='/settings'>
  )rawliteral";

  // Dynamically generate the input fields
  for (const auto& s : settings) {
    if (s.type == BOOL) {
      bool val = *(bool*)s.value;
      html += "<label>" + s.label + "<input type='checkbox' name='" + s.key + "'";
      if (val) html += " checked";
      html += "></label>\n";
    } else if (s.type == INT) {
      int val = *(int*)s.value;
      html += "<label>" + s.label + "<input type='number' name='" + s.key + "' value='" + String(val) + "'></label>\n";
    } else if (s.type == FLOAT) {
      float val = *(float*)s.value;
      html += "<label>" + s.label + "<input type='number' step='any' name='" + s.key + "' value='" + String(val) + "'></label>\n";
    } else if (s.type == STRING) {
      String val = *(String*)s.value;
      html += "<label>" + s.label + "<input type='text' name='" + s.key + "' value='" + val + "'></label>\n";
    } else if (s.type == ARRAY_INT) {
      int* arr = (int*)s.value;
      for (size_t i = 0; i < s.size; i++) {
        String indexedKey = s.key + "_" + String(i);
        html += "<label>" + s.label + " [" + String(i) + "]<input type='number' name='" + indexedKey + "' value='" + String(arr[i]) + "'></label>\n";
      }
    } else if (s.type == ARRAY_FLOAT) {
      float* arr = (float*)s.value;
      for (size_t i = 0; i < s.size; i++) {
        String indexedKey = s.key + "_" + String(i);
        html += "<label>" + s.label + " [" + String(i) + "]<input type='number' step='any' name='" + indexedKey + "' value='" + String(arr[i]) + "'></label>\n";
      }
    }
  }

  html += "<input type='submit' value='Save Settings'>";
  html += "</form></body></html>";

  request->send(200, "text/html", html);
});


  server.on("/settings", HTTP_POST, [this](AsyncWebServerRequest* request) {
    for (auto& s : settings) {
      if (s.type == BOOL) {
        *(bool*)s.value = request->hasParam(s.key, true);
      }
      else if (s.type == INT) {
        if (request->hasParam(s.key, true))
          *(int*)s.value = request->getParam(s.key, true)->value().toInt();
      }
      else if (s.type == STRING) {
        if (request->hasParam(s.key, true))
          *(String*)s.value = request->getParam(s.key, true)->value();
      }
      else if (s.type == ARRAY_INT) {
        int* arr = (int*)s.value;
        for (size_t i = 0; i < s.size; i++) {
          String param = s.key + "_" + String(i);
          if (request->hasParam(param, true))
            arr[i] = request->getParam(param, true)->value().toInt();
        }
      }
      else if (s.type == ARRAY_FLOAT) {
        float* arr = (float*)s.value;
        for (size_t i = 0; i < s.size; i++) {
          String param = s.key + "_" + String(i);
          if (request->hasParam(param, true))
            arr[i] = request->getParam(param, true)->value().toFloat();
        }
      }
    }

    save();  // Save updated values
    
    // Call post-save callback if set
    if (postSaveCallback) {
      postSaveCallback();
    }
    
    request->redirect("/settings");
  });
}
