#pragma once

#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>

#include <functional>
#include <vector>

class SettingsManager {
   public:
    enum SettingType { BOOL, INT, FLOAT, STRING, ARRAY_INT, ARRAY_FLOAT };

    struct Setting {
        String key;    // key for preferences (base key)
        String label;  // label for webpage
        SettingType type;
        void* value;  // pointer to the variable or array
        size_t size;  // for arrays, number of elements
    };

    SettingsManager() = default;

    // Add settings
    void addBool(const char* key, const char* label, bool* value);
    void addInt(const char* key, const char* label, int* value);
    void addFloat(const char* key, const char* label, float* value);
    void addString(const char* key, const char* label, String* value);
    void addIntArray(const char* key, const char* label, int* array, size_t size);
    void addFloatArray(const char* key, const char* label, float* array, size_t size);
    String getPrefKey(const String& key);

    void begin(const String& ns);  // namespace for Preferences
    void load();
    void save();

    void addWebEndpoints(AsyncWebServer& server);

    // Add callback for when settings are saved via web interface
    void setPostSaveCallback(std::function<void()> callback);

   private:
    Preferences prefs;
    String namespaceStr;
    std::function<void()> postSaveCallback = nullptr;

    std::vector<Setting> settings;

    // Hash-based safe key generator
    String makeHashedKey(const String& base, size_t index);

    // FNV-1a hash function for strings
    uint32_t fnv1aHash(const String& str);

    String generateHTML();
};
