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

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR
};

class Logger {
private:
    std::ofstream logFile;
    LogLevel minLevel;
    std::mutex logMutex;
    bool fileEnabled;
    bool consoleEnabled;
    std::string logFilePath;
    
    Logger() : minLevel(LogLevel::DEBUG), fileEnabled(false), 
               consoleEnabled(true), logFilePath("logs/app.log") {}
    
    ~Logger() {
        if (logFile.is_open()) {
            logFile.close();
        }
    }
    
    std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
        ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
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
    
    void write(LogLevel level, const std::string& message) {
        if (level < minLevel) return;
        
        std::lock_guard<std::mutex> lock(logMutex);
        
        std::stringstream logEntry;
        logEntry << "[" << getCurrentTimestamp() << "] ";
        logEntry << "[" << levelToString(level) << "] ";
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
        
        // Создаем директорию для логов если её нет
        size_t lastSlash = filepath.find_last_of('/');
        if (lastSlash != std::string::npos) {
            std::string dir = filepath.substr(0, lastSlash);
            // Создаем директорию через системную команду
            system(("mkdir -p " + dir).c_str());
        }
        
        logFile.open(filepath, std::ios::app);
        if (!logFile.is_open()) {
            fileEnabled = false;
        }
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
    }
    
    // Запрет копирования
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
};

