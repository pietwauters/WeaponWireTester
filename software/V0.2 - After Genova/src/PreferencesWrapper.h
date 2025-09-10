#pragma once

#include <cstring>
#include <string>

#include "nvs.h"
#include "nvs_flash.h"
constexpr nvs_handle_t INVALID_NVS_HANDLE = 0;

class PreferencesWrapper {
   public:
    PreferencesWrapper() : _handle(INVALID_NVS_HANDLE), _isOpen(false) {}

    bool begin(const char* namespace_name, bool readOnly = false) {
        if (strlen(namespace_name) >= NVS_KEY_NAME_MAX_SIZE)
            return false;

        esp_err_t err = nvs_flash_init();
        if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            ESP_ERROR_CHECK(nvs_flash_erase());
            err = nvs_flash_init();
        }

        err = nvs_open(namespace_name, readOnly ? NVS_READONLY : NVS_READWRITE, &_handle);
        _isOpen = (err == ESP_OK);
        return _isOpen;
    }

    void end() {
        if (_isOpen && _handle) {
            nvs_close(_handle);
            _isOpen = false;
            _handle = INVALID_NVS_HANDLE;
        }
    }

    bool putInt(const char* key, int32_t value) { return put(key, nvs_set_i32, value); }

    int32_t getInt(const char* key, int32_t defaultValue = 0) { return get(key, nvs_get_i32, defaultValue); }

    bool putUInt(const char* key, uint32_t value) { return put(key, nvs_set_u32, value); }

    uint32_t getUInt(const char* key, uint32_t defaultValue = 0) { return get(key, nvs_get_u32, defaultValue); }

    bool putBool(const char* key, bool value) { return put(key, nvs_set_u8, static_cast<uint8_t>(value)); }

    bool getBool(const char* key, bool defaultValue = false) {
        return get(key, nvs_get_u8, static_cast<uint8_t>(defaultValue)) != 0;
    }

    bool putFloat(const char* key, float value) { return put(key, nvs_set_blob, &value, sizeof(float)); }

    float getFloat(const char* key, float defaultValue = 0.0f) {
        float value = 0;
        size_t required_size = sizeof(value);
        if (!_isOpen)
            return defaultValue;
        esp_err_t err = nvs_get_blob(_handle, key, &value, &required_size);
        return (err == ESP_OK) ? value : defaultValue;
    }

    bool putString(const char* key, const std::string& value) { return put(key, nvs_set_str, value.c_str()); }

    std::string getString(const char* key, const std::string& defaultValue = "") {
        if (!_isOpen)
            return defaultValue;
        size_t required_size = 0;
        esp_err_t err = nvs_get_str(_handle, key, nullptr, &required_size);
        if (err != ESP_OK)
            return defaultValue;

        char* buffer = new char[required_size];
        err = nvs_get_str(_handle, key, buffer, &required_size);
        std::string result = (err == ESP_OK) ? std::string(buffer) : defaultValue;
        delete[] buffer;
        return result;
    }

    bool remove(const char* key) {
        if (!_isOpen)
            return false;
        esp_err_t err = nvs_erase_key(_handle, key);
        if (err != ESP_OK)
            return false;
        return nvs_commit(_handle) == ESP_OK;
    }

   private:
    nvs_handle_t _handle;
    bool _isOpen;

    // Overloads for put/get helpers
    template <typename T, typename Func>
    bool put(const char* key, Func func, T value) {
        if (!_isOpen || strlen(key) >= NVS_KEY_NAME_MAX_SIZE)
            return false;
        esp_err_t err = func(_handle, key, value);
        if (err != ESP_OK)
            return false;
        return nvs_commit(_handle) == ESP_OK;
    }

    template <typename T, typename Func>
    T get(const char* key, Func func, T defaultValue) {
        if (!_isOpen || strlen(key) >= NVS_KEY_NAME_MAX_SIZE)
            return defaultValue;
        T value;
        esp_err_t err = func(_handle, key, &value);
        return (err == ESP_OK) ? value : defaultValue;
    }

    // Special case for float blobs
    bool put(const char* key, esp_err_t (*func)(nvs_handle_t, const char*, const void*, size_t), const void* data,
             size_t len) {
        if (!_isOpen || strlen(key) >= NVS_KEY_NAME_MAX_SIZE)
            return false;
        esp_err_t err = func(_handle, key, data, len);
        if (err != ESP_OK)
            return false;
        return nvs_commit(_handle) == ESP_OK;
    }

    // Special case for strings
    bool put(const char* key, esp_err_t (*func)(nvs_handle_t, const char*, const char*), const char* str) {
        if (!_isOpen || strlen(key) >= NVS_KEY_NAME_MAX_SIZE)
            return false;
        esp_err_t err = func(_handle, key, str);
        if (err != ESP_OK)
            return false;
        return nvs_commit(_handle) == ESP_OK;
    }
};
