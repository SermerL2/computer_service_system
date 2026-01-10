#pragma once
#include <string>
#include <chrono>
#include <fstream>
#include <mutex>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <memory>
#include <iostream>
#include <unordered_map>
#include <curl/curl.h>

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR
};

// Структура для меток Loki
struct LokiLabels {
    std::unordered_map<std::string, std::string> labels;
    
    LokiLabels& add(const std::string& key, const std::string& value) {
        labels[key] = value;
        return *this;
    }
};

class Logger {
private:
    std::ofstream logFile;
    LogLevel minLevel;
    std::mutex logMutex;
    bool fileEnabled;
    bool consoleEnabled;
    bool lokiFormatEnabled;
    std::string logFilePath;
    std::string serviceName;
    
    // Loki settings
    std::string lokiHost;
    int lokiPort;
    
    Logger() : minLevel(LogLevel::DEBUG), fileEnabled(false), 
               consoleEnabled(true), lokiFormatEnabled(false),
               logFilePath("logs/app.log"), serviceName("service_system"),
               lokiHost("localhost"), lokiPort(3100) {}
    
    ~Logger() {
        if (logFile.is_open()) {
            logFile.close();
        }
        curl_global_cleanup();
    }
    
    std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::stringstream ss;
        std::tm* tm_info = std::localtime(&time);
        ss << std::put_time(tm_info, "%Y-%m-%dT%H:%M:%S");
        ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
        ss << "Z";
        return ss.str();
    }
    
    std::string levelToString(LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO: return "INFO";
            case LogLevel::WARNING: return "WARNING";
            case LogLevel::ERROR: return "ERROR";
            default: return "UNKNOWN";
        }
    }
    
    // Конвертация LogLevel в строку для Loki (строчные буквы)
    std::string levelToLokiString(LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG: return "debug";
            case LogLevel::INFO: return "info";
            case LogLevel::WARNING: return "warning";
            case LogLevel::ERROR: return "error";
            default: return "info";
        }
    }
    
    void write(LogLevel level, const std::string& message) {
        if (level < minLevel) return;
        
        std::lock_guard<std::mutex> lock(logMutex);
        
        std::string timestamp = getCurrentTimestamp();
        std::string levelStr = levelToString(level);
        
        // Стандартный формат
        std::stringstream logEntry;
        logEntry << "[" << timestamp << "] ";
        logEntry << "[" << levelStr << "] ";
        logEntry << message;
        
        std::string entryStr = logEntry.str();
        
        if (consoleEnabled) {
            if (level == LogLevel::ERROR) {
                std::cerr << entryStr << std::endl;
            } else {
                std::cout << entryStr << std::endl;
            }
        }
        
        if (fileEnabled && logFile.is_open()) {
            logFile << entryStr << std::endl;
            logFile.flush();
        }
    }
    
    // Отправка лога в Loki
    bool sendToLoki(const std::string& level, const std::string& message, const LokiLabels& labels) {
        CURL* curl = curl_easy_init();
        if (!curl) return false;
        
        // Формирование JSON для Loki
        std::stringstream json_ss;
        json_ss << R"({"streams":[{"stream":{"app":")" << serviceName << R"(",")";
        json_ss << R"("level":")" << level << R"(",)";
        
        // Добавляем метки
        for (const auto& kv : labels.labels) {
            json_ss << "\"" << kv.first << "\":\"" << kv.second << "\",";
        }
        
        json_ss << R"("job":"service_system"},"values":[{"ts":")" << getCurrentTimestamp() << R"(", "line":")";
        
        // Экранируем спецсимволы в сообщении
        std::string escapedMessage = message;
        for (size_t i = 0; i < escapedMessage.size(); ++i) {
            if (escapedMessage[i] == '"') {
                escapedMessage[i] = '\\';
            } else if (escapedMessage[i] == '\\') {
                escapedMessage.insert(escapedMessage.begin() + i, '\\');
                ++i;
            } else if (escapedMessage[i] == '\n') {
                escapedMessage[i] = '\\';
            } else if (escapedMessage[i] == '\r') {
                escapedMessage[i] = '\\';
            }
        }
        
        json_ss << escapedMessage << R"("}]}]})";
        
        std::string jsonPayload = json_ss.str();
        
        // URL для Loki
        std::stringstream url_ss;
        url_ss << "http://" << lokiHost << ":" << lokiPort << "/loki/api/v1/push";
        std::string url = url_ss.str();
        
        // Заголовки
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonPayload.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
        
        CURLcode res = curl_easy_perform(curl);
        
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        
        return (res == CURLE_OK);
    }
    
public:
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }
    
    void init(const std::string& filepath = "logs/app.log", LogLevel level = LogLevel::DEBUG) {
        std::lock_guard<std::mutex> lock(logMutex);
        
        logFilePath = filepath;
        minLevel = level;
        fileEnabled = true;
        consoleEnabled = true;
        lokiFormatEnabled = false;
        
        // Инициализация curl
        curl_global_init(CURL_GLOBAL_ALL);
        
        // Создаем директорию для логов если её нет
        size_t lastSlash = filepath.find_last_of('/');
        if (lastSlash != std::string::npos) {
            std::string dir = filepath.substr(0, lastSlash);
            system(("mkdir -p " + dir).c_str());
        }
        
        logFile.open(filepath, std::ios::app);
        if (!logFile.is_open()) {
            fileEnabled = false;
        }
    }
    
    // Инициализация с настройками Loki
    void initWithLoki(const std::string& filepath, LogLevel level,
                      const std::string& host, int port) {
        init(filepath, level);
        lokiHost = host;
        lokiPort = port;
        lokiFormatEnabled = true;
    }
    
    void setLevel(LogLevel level) {
        std::lock_guard<std::mutex> lock(logMutex);
        minLevel = level;
    }
    
    void enableConsole(bool enabled) {
        std::lock_guard<std::mutex> lock(logMutex);
        consoleEnabled = enabled;
    }
    
    void enableFile(bool enabled) {
        std::lock_guard<std::mutex> lock(logMutex);
        if (enabled && !logFile.is_open()) {
            logFile.open(logFilePath, std::ios::app);
            if (!logFile.is_open()) {
                fileEnabled = false;
                return;
            }
        }
        fileEnabled = enabled;
    }
    
    void setServiceName(const std::string& name) {
        std::lock_guard<std::mutex> lock(logMutex);
        serviceName = name;
    }
    
    void debug(const std::string& message) { write(LogLevel::DEBUG, message); }
    void info(const std::string& message) { write(LogLevel::INFO, message); }
    void warning(const std::string& message) { write(LogLevel::WARNING, message); }
    void error(const std::string& message) { write(LogLevel::ERROR, message); }
    
    // Логирование с контекстом (файл:строка)
    void debug(const std::string& message, const std::string& context) {
        write(LogLevel::DEBUG, message + " [" + context + "]");
    }
    void info(const std::string& message, const std::string& context) {
        write(LogLevel::INFO, message + " [" + context + "]");
    }
    void warning(const std::string& message, const std::string& context) {
        write(LogLevel::WARNING, message + " [" + context + "]");
    }
    void error(const std::string& message, const std::string& context) {
        write(LogLevel::ERROR, message + " [" + context + "]");
    }
    
    // Логирование HTTP запросов
    void logRequest(const std::string& ip, const std::string& method, 
                    const std::string& path, int status, long duration_ms) {
        std::stringstream ss;
        ss << "HTTP Request: " << method << " " << path 
           << " | IP: " << ip << " | Status: " << status 
           << " | Duration: " << duration_ms << "ms";
        write(LogLevel::INFO, ss.str());
        
        if (lokiFormatEnabled) {
            LokiLabels labels;
            labels.add("component", "webserver")
                   .add("endpoint", path)
                   .add("method", method);
            sendToLoki("info", ss.str(), labels);
        }
    }
    
    // Логирование авторизации
    void logAuth(const std::string& username, bool success, const std::string& ip, 
                 const std::string& details = "") {
        std::stringstream ss;
        ss << "Auth attempt: user='" << username << "' "
           << "success=" << (success ? "true" : "false")
           << " ip=" << ip;
        if (!details.empty()) {
            ss << " details=" << details;
        }
        write(success ? LogLevel::INFO : LogLevel::WARNING, ss.str());
        
        if (lokiFormatEnabled) {
            LokiLabels labels;
            labels.add("component", "auth")
                   .add("user", username)
                   .add("success", success ? "true" : "false")
                   .add("ip", ip);
            sendToLoki(success ? "info" : "warning", ss.str(), labels);
        }
    }
    
    // Логирование ошибок БД
    void logDatabase(const std::string& operation, bool success, 
                     const std::string& details = "") {
        std::stringstream ss;
        ss << "DB operation: " << operation << " | success=" << (success ? "true" : "false");
        if (!details.empty()) {
            ss << " | details=" << details;
        }
        write(success ? LogLevel::INFO : LogLevel::ERROR, ss.str());
        
        if (lokiFormatEnabled) {
            LokiLabels labels;
            labels.add("component", "database")
                   .add("operation", operation)
                   .add("success", success ? "true" : "false");
            sendToLoki(success ? "info" : "error", ss.str(), labels);
        }
    }
    
    // Логирование устройств
    void logDevice(int deviceId, const std::string& operation, bool success) {
        std::stringstream ss;
        ss << "Device operation: " << operation << " | device_id=" << deviceId 
           << " | success=" << (success ? "true" : "false");
        write(success ? LogLevel::INFO : LogLevel::WARNING, ss.str());
        
        if (lokiFormatEnabled) {
            LokiLabels labels;
            labels.add("component", "device")
                   .add("operation", operation)
                   .add("device_id", std::to_string(deviceId));
            sendToLoki(success ? "info" : "warning", ss.str(), labels);
        }
    }
    
    // Логирование обслуживания
    void logService(int recordId, const std::string& operation, bool success) {
        std::stringstream ss;
        ss << "Service operation: " << operation << " | record_id=" << recordId 
           << " | success=" << (success ? "true" : "false");
        write(success ? LogLevel::INFO : LogLevel::WARNING, ss.str());
        
        if (lokiFormatEnabled) {
            LokiLabels labels;
            labels.add("component", "service")
                   .add("operation", operation)
                   .add("record_id", std::to_string(recordId));
            sendToLoki(success ? "info" : "warning", ss.str(), labels);
        }
    }
    
    // Логирование жизненного цикла приложения
    void logLifecycle(const std::string& event, const std::string& details = "") {
        std::stringstream ss;
        ss << "Lifecycle event: " << event;
        if (!details.empty()) {
            ss << " | details=" << details;
        }
        write(LogLevel::INFO, ss.str());
        
        if (lokiFormatEnabled) {
            LokiLabels labels;
            labels.add("component", "lifecycle")
                   .add("event", event);
            sendToLoki("info", ss.str(), labels);
        }
    }
    
    // Запрет копирования
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
};

