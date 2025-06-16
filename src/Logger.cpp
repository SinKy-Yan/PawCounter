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
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>

// 静态成员初始化
Logger* Logger::_instance = nullptr;

Logger& Logger::getInstance() {
    static Logger instance;
    _instance = &instance;  // 保存实例指针用于静态回调
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
    _customFormat = false;  // 默认不使用自定义格式
    
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

void Logger::setCustomFormat(bool enable) {
    _customFormat = enable;
    if (enable) {
        esp_log_set_vprintf(customVprintf);
    } else {
        esp_log_set_vprintf(vprintf);
    }
    info(TAG_SYSTEM, "Custom log format %s", enable ? "enabled" : "disabled");
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

const char* Logger::getLevelChar(log_level_t level) {
    switch (level) {
        case LOG_LEVEL_ERROR:   return "E";
        case LOG_LEVEL_WARN:    return "W";
        case LOG_LEVEL_INFO:    return "I";
        case LOG_LEVEL_DEBUG:   return "D";
        case LOG_LEVEL_VERBOSE: return "V";
        default:                return "?";
    }
}

void Logger::error(const char* tag, const char* format, ...) {
    if (!_initialized) return;
    
    va_list args;
    va_start(args, format);
    
    if (_customFormat) {
        formatAndPrint(LOG_LEVEL_ERROR, tag, format, args);
    } else {
        char newFormat[256];
        size_t len = strlen(format);
        if (len > 0 && format[len - 1] != '\n' && len < sizeof(newFormat) - 2) {
            strcpy(newFormat, format);
            newFormat[len] = '\n';
            newFormat[len + 1] = '\0';
            format = newFormat;
        }
        esp_log_writev(ESP_LOG_ERROR, tag, format, args);
    }
    
    va_end(args);
}

void Logger::warn(const char* tag, const char* format, ...) {
    if (!_initialized) return;
    
    va_list args;
    va_start(args, format);
    
    if (_customFormat) {
        formatAndPrint(LOG_LEVEL_WARN, tag, format, args);
    } else {
        char newFormat[256];
        size_t len = strlen(format);
        if (len > 0 && format[len - 1] != '\n' && len < sizeof(newFormat) - 2) {
            strcpy(newFormat, format);
            newFormat[len] = '\n';
            newFormat[len + 1] = '\0';
            format = newFormat;
        }
        esp_log_writev(ESP_LOG_WARN, tag, format, args);
    }
    
    va_end(args);
}

void Logger::info(const char* tag, const char* format, ...) {
    if (!_initialized) return;
    
    va_list args;
    va_start(args, format);
    
    if (_customFormat) {
        formatAndPrint(LOG_LEVEL_INFO, tag, format, args);
    } else {
        char newFormat[256];
        size_t len = strlen(format);
        if (len > 0 && format[len - 1] != '\n' && len < sizeof(newFormat) - 2) {
            strcpy(newFormat, format);
            newFormat[len] = '\n';
            newFormat[len + 1] = '\0';
            format = newFormat;
        }
        esp_log_writev(ESP_LOG_INFO, tag, format, args);
    }
    
    va_end(args);
}

void Logger::debug(const char* tag, const char* format, ...) {
    if (!_initialized) return;
    
    va_list args;
    va_start(args, format);
    
    if (_customFormat) {
        formatAndPrint(LOG_LEVEL_DEBUG, tag, format, args);
    } else {
        char newFormat[256];
        size_t len = strlen(format);
        if (len > 0 && format[len - 1] != '\n' && len < sizeof(newFormat) - 2) {
            strcpy(newFormat, format);
            newFormat[len] = '\n';
            newFormat[len + 1] = '\0';
            format = newFormat;
        }
        esp_log_writev(ESP_LOG_DEBUG, tag, format, args);
    }
    
    va_end(args);
}

void Logger::verbose(const char* tag, const char* format, ...) {
    if (!_initialized) return;
    
    va_list args;
    va_start(args, format);
    
    if (_customFormat) {
        formatAndPrint(LOG_LEVEL_VERBOSE, tag, format, args);
    } else {
        char newFormat[256];
        size_t len = strlen(format);
        if (len > 0 && format[len - 1] != '\n' && len < sizeof(newFormat) - 2) {
            strcpy(newFormat, format);
            newFormat[len] = '\n';
            newFormat[len + 1] = '\0';
            format = newFormat;
        }
        esp_log_writev(ESP_LOG_VERBOSE, tag, format, args);
    }
    
    va_end(args);
}

void Logger::flush() {
    if (_config.output & LOG_OUTPUT_SERIAL) {
        Serial.flush();
    }
}

int Logger::customVprintf(const char* format, va_list args) {
    // 这个函数将被 ESP-IDF 调用，我们需要在这里拦截并重新栾式化
    if (_instance && _instance->_customFormat) {
        char buffer[512];
        int len = vsnprintf(buffer, sizeof(buffer), format, args);
        
        // 只输出到串口，不再通过 ESP-IDF 系统
        if (len > 0) {
            Serial.print(buffer);
            Serial.flush();
        }
        return len;
    }
    
    // 默认使用系统的 vprintf
    return vprintf(format, args);
}

void Logger::formatAndPrint(log_level_t level, const char* tag, const char* format, va_list args) {
    // 获取时间戳
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm* timeinfo = localtime(&tv.tv_sec);
    
    char timeStr[32];
    if (_config.showTimestamp) {
        snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d.%03ld", 
                timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, tv.tv_usec / 1000);
    } else {
        strcpy(timeStr, "");
    }
    
    // 格式化消息内容
    char message[512];
    vsnprintf(message, sizeof(message), format, args);
    
    // 移除末尾的换行符（如果有）
    size_t msgLen = strlen(message);
    if (msgLen > 0 && message[msgLen - 1] == '\n') {
        message[msgLen - 1] = '\0';
    }
    
    // 构建最终输出格式：[E] 12:34:56.789 TAG: message
    char finalOutput[1024];
    int pos = 0;
    
    // 添加颜色（如果启用）
    if (_config.colorOutput) {
        pos += snprintf(finalOutput + pos, sizeof(finalOutput) - pos, "%s", getLevelColor(level));
    }
    
    // 添加级别标识
    if (_config.showLevel) {
        pos += snprintf(finalOutput + pos, sizeof(finalOutput) - pos, "[%s] ", getLevelChar(level));
    }
    
    // 添加时间戳
    if (_config.showTimestamp && strlen(timeStr) > 0) {
        pos += snprintf(finalOutput + pos, sizeof(finalOutput) - pos, "%s ", timeStr);
    }
    
    // 添加标签
    if (_config.showTag) {
        pos += snprintf(finalOutput + pos, sizeof(finalOutput) - pos, "%s: ", tag);
    }
    
    // 添加消息内容
    pos += snprintf(finalOutput + pos, sizeof(finalOutput) - pos, "%s", message);
    
    // 重置颜色（如果启用）
    if (_config.colorOutput) {
        pos += snprintf(finalOutput + pos, sizeof(finalOutput) - pos, "\033[0m");
    }
    
    // 添加换行符
    pos += snprintf(finalOutput + pos, sizeof(finalOutput) - pos, "\n");
    
    // 输出到串口
    Serial.print(finalOutput);
    Serial.flush();
}