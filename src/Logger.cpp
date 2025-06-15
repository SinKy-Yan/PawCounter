/**
 * @file Logger.cpp
 * @brief 日志系统实现文件
 * @details 基于ESP32-IDF的日志系统实现
 * 
 * @author Calculator Project
 * @date 2024-01-07
 */

#include "Logger.h"
#include <stdarg.h>

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

LoggerConfig Logger::getDefaultConfig() {
    LoggerConfig config;
    
    #ifdef DEBUG_MODE
    config.level = LOG_LEVEL_DEBUG;
    #else
    config.level = LOG_LEVEL_INFO;
    #endif
    
    config.output = LOG_OUTPUT_SERIAL;
    config.showTimestamp = true;
    config.showLevel = true;
    config.showTag = true;
    config.colorOutput = true;
    config.baudRate = 115200;
    config.bufferSize = 1024;
    
    return config;
}

bool Logger::begin(const LoggerConfig& config) {
    _config = config;
    
    // 初始化串口（如果还未初始化）
    if (!Serial) {
        Serial.begin(_config.baudRate);
        // 等待串口连接
        while (!Serial) {
            delay(10);
        }
    }
    
    // 设置ESP-IDF日志级别
    esp_log_level_set("*", toEspLogLevel(_config.level));
    
    // 配置ESP-IDF日志输出格式
    if (_config.colorOutput) {
        esp_log_set_vprintf(vprintf);
    }
    
    _initialized = true;
    
    // 输出系统启动日志
    info(TAG_SYSTEM, "Logger system initialized");
    info(TAG_SYSTEM, "Log level: %s", getLevelString(_config.level));
    info(TAG_SYSTEM, "Color output: %s", _config.colorOutput ? "enabled" : "disabled");
    info(TAG_SYSTEM, "Baud rate: %lu", _config.baudRate);
    
    return true;
}

void Logger::setLevel(log_level_t level) {
    _config.level = level;
    esp_log_level_set("*", toEspLogLevel(level));
    info(TAG_SYSTEM, "Global log level set to: %s", getLevelString(level));
}

void Logger::setTagLevel(const char* tag, log_level_t level) {
    esp_log_level_set(tag, toEspLogLevel(level));
    info(TAG_SYSTEM, "Log level for tag '%s' set to: %s", tag, getLevelString(level));
}

void Logger::setColorOutput(bool enable) {
    _config.colorOutput = enable;
    info(TAG_SYSTEM, "Color output %s", enable ? "enabled" : "disabled");
}

esp_log_level_t Logger::toEspLogLevel(log_level_t level) {
    switch (level) {
        case LOG_LEVEL_NONE:    return ESP_LOG_NONE;
        case LOG_LEVEL_ERROR:   return ESP_LOG_ERROR;
        case LOG_LEVEL_WARN:    return ESP_LOG_WARN;
        case LOG_LEVEL_INFO:    return ESP_LOG_INFO;
        case LOG_LEVEL_DEBUG:   return ESP_LOG_DEBUG;
        case LOG_LEVEL_VERBOSE: return ESP_LOG_VERBOSE;
        default:                return ESP_LOG_INFO;
    }
}

const char* Logger::getLevelString(log_level_t level) {
    switch (level) {
        case LOG_LEVEL_NONE:    return "NONE";
        case LOG_LEVEL_ERROR:   return "ERROR";
        case LOG_LEVEL_WARN:    return "WARN";
        case LOG_LEVEL_INFO:    return "INFO";
        case LOG_LEVEL_DEBUG:   return "DEBUG";
        case LOG_LEVEL_VERBOSE: return "VERBOSE";
        default:                return "UNKNOWN";
    }
}

const char* Logger::getLevelColor(log_level_t level) {
    if (!_config.colorOutput) {
        return "";
    }
    
    switch (level) {
        case LOG_LEVEL_ERROR:   return "\033[31m";  // 红色
        case LOG_LEVEL_WARN:    return "\033[33m";  // 黄色
        case LOG_LEVEL_INFO:    return "\033[32m";  // 绿色
        case LOG_LEVEL_DEBUG:   return "\033[36m";  // 青色
        case LOG_LEVEL_VERBOSE: return "\033[37m";  // 白色
        default:                return "\033[0m";   // 重置
    }
}

void Logger::error(const char* tag, const char* format, ...) {
    if (!_initialized) return;
    
    char newFormat[256];
    size_t len = strlen(format);
    if (len > 0 && format[len - 1] != '\n' && len < sizeof(newFormat) - 2) {
        strcpy(newFormat, format);
        newFormat[len] = '\n';
        newFormat[len + 1] = '\0';
        format = newFormat;
    }
    va_list args;
    va_start(args, format);
    esp_log_writev(ESP_LOG_ERROR, tag, format, args);
    va_end(args);
}

void Logger::warn(const char* tag, const char* format, ...) {
    if (!_initialized) return;
    
    char newFormat[256];
    size_t len = strlen(format);
    if (len > 0 && format[len - 1] != '\n' && len < sizeof(newFormat) - 2) {
        strcpy(newFormat, format);
        newFormat[len] = '\n';
        newFormat[len + 1] = '\0';
        format = newFormat;
    }
    va_list args;
    va_start(args, format);
    esp_log_writev(ESP_LOG_WARN, tag, format, args);
    va_end(args);
}

void Logger::info(const char* tag, const char* format, ...) {
    if (!_initialized) return;
    
    char newFormat[256];
    size_t len = strlen(format);
    if (len > 0 && format[len - 1] != '\n' && len < sizeof(newFormat) - 2) {
        strcpy(newFormat, format);
        newFormat[len] = '\n';
        newFormat[len + 1] = '\0';
        format = newFormat;
    }
    va_list args;
    va_start(args, format);
    esp_log_writev(ESP_LOG_INFO, tag, format, args);
    va_end(args);
}

void Logger::debug(const char* tag, const char* format, ...) {
    if (!_initialized) return;
    
    char newFormat[256];
    size_t len = strlen(format);
    if (len > 0 && format[len - 1] != '\n' && len < sizeof(newFormat) - 2) {
        strcpy(newFormat, format);
        newFormat[len] = '\n';
        newFormat[len + 1] = '\0';
        format = newFormat;
    }
    va_list args;
    va_start(args, format);
    esp_log_writev(ESP_LOG_DEBUG, tag, format, args);
    va_end(args);
}

void Logger::verbose(const char* tag, const char* format, ...) {
    if (!_initialized) return;
    
    char newFormat[256];
    size_t len = strlen(format);
    if (len > 0 && format[len - 1] != '\n' && len < sizeof(newFormat) - 2) {
        strcpy(newFormat, format);
        newFormat[len] = '\n';
        newFormat[len + 1] = '\0';
        format = newFormat;
    }
    va_list args;
    va_start(args, format);
    esp_log_writev(ESP_LOG_VERBOSE, tag, format, args);
    va_end(args);
}

void Logger::flush() {
    if (_config.output & LOG_OUTPUT_SERIAL) {
        Serial.flush();
    }
}