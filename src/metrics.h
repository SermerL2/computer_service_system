#pragma once
#include <string>
#include <unordered_map>
#include <sstream>
#include <iomanip>
#include <vector>
#include <mutex>
#include <memory>
#include <iostream>
#include <map>

// Simple Prometheus-compatible metrics implementation
// Fixed to properly output Prometheus format with separate name and labels

struct Labels {
    std::map<std::string, std::string> labels;
    
    Labels() {}
    
    Labels(const std::map<std::string, std::string>& l) : labels(l) {}
    
    std::string toString() const {
        if (labels.empty()) return "";
        std::stringstream ss;
        ss << "{";
        bool first = true;
        for (const auto& pair : labels) {
            if (!first) ss << ",";
            ss << pair.first << "=\"" << pair.second << "\"";
            first = false;
        }
        ss << "}";
        return ss.str();
    }
    
    bool operator<(const Labels& other) const {
        return labels < other.labels;
    }
};

class Counter {
private:
    double value_ = 0.0;
    std::string name_;
    std::string help_;
    Labels labels_;
    mutable std::mutex mutex_;
    
public:
    Counter() : name_(""), help_("") {}
    
    Counter(const std::string& name, const std::string& help = "", const Labels& labels = Labels())
        : name_(name), help_(help), labels_(labels) {}
    
    // Need proper copy/move for containers
    Counter(const Counter& other) {
        std::lock_guard<std::mutex> lock(other.mutex_);
        name_ = other.name_;
        help_ = other.help_;
        value_ = other.value_;
        labels_ = other.labels_;
    }
    
    Counter& operator=(const Counter& other) {
        if (this != &other) {
            std::lock_guard<std::mutex> lock1(mutex_);
            std::lock_guard<std::mutex> lock2(other.mutex_);
            name_ = other.name_;
            help_ = other.help_;
            value_ = other.value_;
            labels_ = other.labels_;
        }
        return *this;
    }
    
    Counter(Counter&& other) noexcept {
        std::lock_guard<std::mutex> lock(other.mutex_);
        name_ = std::move(other.name_);
        help_ = std::move(other.help_);
        value_ = other.value_;
        labels_ = std::move(other.labels_);
        other.value_ = 0;
    }
    
    Counter& operator=(Counter&& other) noexcept {
        if (this != &other) {
            std::lock_guard<std::mutex> lock1(mutex_);
            std::lock_guard<std::mutex> lock2(other.mutex_);
            name_ = std::move(other.name_);
            help_ = std::move(other.help_);
            value_ = other.value_;
            labels_ = std::move(other.labels_);
            other.value_ = 0;
        }
        return *this;
    }
    
    void increment(double val = 1.0) {
        std::lock_guard<std::mutex> lock(mutex_);
        value_ += val;
    }
    
    double value() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return value_;
    }
    
    const std::string& getName() const { return name_; }
    const std::string& getHelp() const { return help_; }
    const Labels& getLabels() const { return labels_; }
    
    void setName(const std::string& name) { name_ = name; }
    void setHelp(const std::string& help) { help_ = help; }
    
    // Format for output - outputs metric name + labels + value
    std::string format() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::stringstream ss;
        ss << name_ << labels_.toString() << " " << std::fixed << std::setprecision(0) << value_ << "\n";
        return ss.str();
    }
    
    // Format TYPE and HELP comments (called once per base name)
    static std::string formatTypeHelp(const std::string& name, const std::string& help, const std::string& type = "counter") {
        std::stringstream ss;
        ss << "# TYPE " << name << " " << type << "\n";
        if (!help.empty()) {
            ss << "# HELP " << name << " " << help << "\n";
        }
        return ss.str();
    }
};

class Histogram {
private:
    double sum_ = 0.0;
    uint64_t count_ = 0;
    std::vector<double> values_;
    std::string name_;
    std::string help_;
    Labels labels_;
    std::vector<double> buckets_;
    mutable std::mutex mutex_;
    
public:
    Histogram() : name_(""), help_("") {}
    
    Histogram(const std::string& name, const std::string& help = "",
              const std::vector<double>& buckets = {0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1.0, 2.5, 5.0, 10.0},
              const Labels& labels = Labels())
        : name_(name), help_(help), labels_(labels), buckets_(buckets) {}
    
    // Proper copy/move
    Histogram(const Histogram& other) {
        std::lock_guard<std::mutex> lock(other.mutex_);
        name_ = other.name_;
        help_ = other.help_;
        sum_ = other.sum_;
        count_ = other.count_;
        values_ = other.values_;
        buckets_ = other.buckets_;
        labels_ = other.labels_;
    }
    
    Histogram& operator=(const Histogram& other) {
        if (this != &other) {
            std::lock_guard<std::mutex> lock1(mutex_);
            std::lock_guard<std::mutex> lock2(other.mutex_);
            name_ = other.name_;
            help_ = other.help_;
            sum_ = other.sum_;
            count_ = other.count_;
            values_ = other.values_;
            buckets_ = other.buckets_;
            labels_ = other.labels_;
        }
        return *this;
    }
    
    Histogram(Histogram&& other) noexcept {
        std::lock_guard<std::mutex> lock(other.mutex_);
        name_ = std::move(other.name_);
        help_ = std::move(other.help_);
        sum_ = other.sum_;
        count_ = other.count_;
        values_ = std::move(other.values_);
        buckets_ = std::move(other.buckets_);
        labels_ = std::move(other.labels_);
        other.sum_ = 0;
        other.count_ = 0;
    }
    
    Histogram& operator=(Histogram&& other) noexcept {
        if (this != &other) {
            std::lock_guard<std::mutex> lock1(mutex_);
            std::lock_guard<std::mutex> lock2(other.mutex_);
            name_ = std::move(other.name_);
            help_ = std::move(other.help_);
            sum_ = other.sum_;
            count_ = other.count_;
            values_ = std::move(other.values_);
            buckets_ = std::move(other.buckets_);
            labels_ = std::move(other.labels_);
            other.sum_ = 0;
            other.count_ = 0;
        }
        return *this;
    }
    
    void observe(double value) {
        std::lock_guard<std::mutex> lock(mutex_);
        values_.push_back(value);
        sum_ += value;
        count_++;
    }
    
    double sum() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return sum_;
    }
    
    uint64_t count() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return count_;
    }
    
    const std::string& getName() const { return name_; }
    const std::string& getHelp() const { return help_; }
    const Labels& getLabels() const { return labels_; }
    
    void setName(const std::string& name) { name_ = name; }
    void setHelp(const std::string& help) { help_ = help; }
    
    // Format for output - outputs histogram buckets, sum, and count
    // Fixed: Proper Prometheus exposition format
    std::string format() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::stringstream ss;
        
        // Calculate bucket counts
        std::vector<uint64_t> bucketCounts(buckets_.size(), 0);
        for (double v : values_) {
            for (size_t i = 0; i < buckets_.size(); ++i) {
                if (v <= buckets_[i]) {
                    bucketCounts[i]++;
                }
            }
        }
        
        // Get the label part without the leading '{'
        std::string labelPart = labels_.toString();
        if (!labelPart.empty() && labelPart[0] == '{') {
            labelPart = labelPart.substr(1);
        }
        
        // Output buckets with proper Prometheus format
        for (size_t i = 0; i < buckets_.size(); ++i) {
            ss << name_ << "_bucket{le=\"" << std::fixed << std::setprecision(4) << buckets_[i] << "\"";
            if (!labelPart.empty()) ss << "," << labelPart;
            ss << "} " << bucketCounts[i] << "\n";
        }
        
        // +Inf bucket
        ss << name_ << "_bucket{le=\"+Inf\"";
        if (!labelPart.empty()) ss << "," << labelPart;
        ss << "} " << count_ << "\n";
        
        // Sum and count
        ss << name_ << "_sum" << labels_.toString() << " " << std::fixed << std::setprecision(6) << sum_ << "\n";
        ss << name_ << "_count" << labels_.toString() << " " << count_ << "\n";
        
        return ss.str();
    }
    
    // Format TYPE and HELP comments
    static std::string formatTypeHelp(const std::string& name, const std::string& help) {
        std::stringstream ss;
        ss << "# TYPE " << name << " histogram\n";
        if (!help.empty()) {
            ss << "# HELP " << name << " " << help << "\n";
        }
        return ss.str();
    }
};

class MetricsRegistry {
private:
    // Store metrics organized by base name, then by labels
    std::unordered_map<std::string, std::map<Labels, Counter>> counters_;
    std::unordered_map<std::string, std::map<Labels, Histogram>> histograms_;
    
    // Track TYPE/HELP info for each metric
    std::unordered_map<std::string, std::string> counterHelps_;
    std::unordered_map<std::string, std::string> histogramHelps_;
    
    mutable std::mutex mutex_;
    bool initialized_ = false;
    
    MetricsRegistry() = default;
    
public:
    static MetricsRegistry& getInstance() {
        static MetricsRegistry instance;
        return instance;
    }
    
    // Initialize standard metrics so Prometheus can scrape them from startup
    void initialize() {
        if (initialized_) return;
        
        std::lock_guard<std::mutex> lock(mutex_);
        if (initialized_) return;
        
        std::cerr << "[METRICS] Initializing metrics registry..." << std::endl;
        
        // Initialize http_request_duration_seconds histogram
        counterHelps_["http_requests_total"] = "Total number of HTTP requests";
        histogramHelps_["http_request_duration_seconds"] = "HTTP request duration in seconds";
        
        // Pre-register http_requests_total with all expected label combinations
        // This ensures Prometheus can scrape the metric even before any requests are made
        std::vector<std::string> methods = {"GET", "POST", "PUT", "DELETE", "PATCH"};
        std::vector<std::string> paths = {"/", "/metrics", "/api/test-db", "/api/login", 
                                           "/api/logout", "/api/devices", "/api/service-types",
                                           "/api/service-history", "/api/service-records"};
        std::vector<int> statuses = {200, 201, 400, 401, 403, 404, 500};
        
        for (const auto& method : methods) {
            for (const auto& path : paths) {
                for (int status : statuses) {
                    std::map<std::string, std::string> labels;
                    labels["method"] = method;
                    labels["path"] = path;
                    labels["status"] = std::to_string(status);
                    Labels l(labels);
                    Counter c("http_requests_total", "Total number of HTTP requests", l);
                    counters_["http_requests_total"].emplace(l, std::move(c));
                }
            }
        }
        
        // Initialize auth_failures_total counter
        counterHelps_["auth_failures_total"] = "Total number of authentication failures";
        Counter authFailures("auth_failures_total", "Total number of authentication failures");
        counters_["auth_failures_total"].emplace(Labels(), std::move(authFailures));
        
        // Initialize histogram
        Histogram h("http_request_duration_seconds", 
                    "HTTP request duration in seconds",
                    {0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1.0, 2.5, 5.0, 10.0});
        histograms_["http_request_duration_seconds"].emplace(Labels(), std::move(h));
        
        std::cerr << "[METRICS] Initialization complete. Counters: " << getCounterCount() 
                  << ", Histograms: " << getHistogramCount() << std::endl;
        
        initialized_ = true;
    }
    
    size_t getCounterCount() const {
        size_t count = 0;
        for (const auto& pair : counters_) {
            count += pair.second.size();
        }
        return count;
    }
    
    size_t getHistogramCount() const {
        size_t count = 0;
        for (const auto& pair : histograms_) {
            count += pair.second.size();
        }
        return count;
    }
    
    Counter& getCounter(const std::string& name, const std::string& help = "", 
                        const Labels& labels = Labels()) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Store help if provided
        if (!help.empty()) {
            counterHelps_[name] = help;
        }
        
        auto it = counters_.find(name);
        if (it == counters_.end()) {
            // Create new counter entry
            Counter c(name, help, labels);
            auto& metricMap = counters_[name];
            auto result = metricMap.emplace(labels, std::move(c));
            return result.first->second;
        }
        
        // Check if counter with these labels exists
        auto& metricMap = it->second;
        auto labelIt = metricMap.find(labels);
        if (labelIt == metricMap.end()) {
            Counter c(name, help, labels);
            auto result = metricMap.emplace(labels, std::move(c));
            return result.first->second;
        }
        
        return labelIt->second;
    }
    
    Histogram& getHistogram(const std::string& name, const std::string& help = "",
                            const std::vector<double>& buckets = {},
                            const Labels& labels = Labels()) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Store help if provided
        if (!help.empty()) {
            histogramHelps_[name] = help;
        }
        
        auto it = histograms_.find(name);
        if (it == histograms_.end()) {
            Histogram h(name, help, buckets, labels);
            auto& metricMap = histograms_[name];
            auto result = metricMap.emplace(labels, std::move(h));
            return result.first->second;
        }
        
        auto& metricMap = it->second;
        auto labelIt = metricMap.find(labels);
        if (labelIt == metricMap.end()) {
            Histogram h(name, help, buckets, labels);
            auto result = metricMap.emplace(labels, std::move(h));
            return result.first->second;
        }
        
        return labelIt->second;
    }
    
    std::string format() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::stringstream ss;
        
        // Output counters with TYPE/HELP first, then values
        for (const auto& namePair : counters_) {
            const std::string& name = namePair.first;
            const auto& labelMap = namePair.second;
            
            // Output TYPE and HELP once per metric
            auto helpIt = counterHelps_.find(name);
            std::string help = (helpIt != counterHelps_.end()) ? helpIt->second : "";
            ss << Counter::formatTypeHelp(name, help, "counter");
            
            // Output all label combinations
            for (const auto& labelPair : labelMap) {
                ss << labelPair.second.format();
            }
        }
        
        // Output histograms with TYPE/HELP first, then values
        for (const auto& namePair : histograms_) {
            const std::string& name = namePair.first;
            const auto& labelMap = namePair.second;
            
            // Output TYPE and HELP once per metric
            auto helpIt = histogramHelps_.find(name);
            std::string help = (helpIt != histogramHelps_.end()) ? helpIt->second : "";
            ss << Histogram::formatTypeHelp(name, help);
            
            // Output all label combinations
            for (const auto& labelPair : labelMap) {
                ss << labelPair.second.format();
            }
        }
        
        return ss.str();
    }
    
    // Convenience methods for common metrics
    void recordHttpRequest(const std::string& method, const std::string& path, int status, double duration_seconds) {
        std::map<std::string, std::string> labels;
        labels["method"] = method;
        labels["path"] = path;
        labels["status"] = std::to_string(status);
        
        // Get or create counter with these labels
        auto& counter = getCounter("http_requests_total", "Total number of HTTP requests", Labels(labels));
        counter.increment();
        
        // Histogram for duration (without labels)
        auto& histogram = getHistogram("http_request_duration_seconds", 
                                       "HTTP request duration in seconds",
                                       {0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1.0, 2.5, 5.0, 10.0});
        histogram.observe(duration_seconds);
    }
    
    void recordDbOperation(const std::string& operation, bool success) {
        std::map<std::string, std::string> labels;
        labels["operation"] = operation;
        labels["success"] = success ? "true" : "false";
        
        auto& counter = getCounter("db_operations_total", "Total number of database operations", Labels(labels));
        counter.increment();
    }
    
    void recordAuthAttempt(const std::string& username, bool success) {
        std::map<std::string, std::string> labels;
        labels["username"] = username;
        labels["success"] = success ? "true" : "false";
        
        auto& counter = getCounter("auth_attempts_total", "Total number of authentication attempts", Labels(labels));
        counter.increment();
        
        if (!success) {
            auto& failCounter = getCounter("auth_failures_total", "Total number of authentication failures");
            failCounter.increment();
        }
    }
    
    void recordDeviceOperation(const std::string& operation, int device_id, bool success) {
        std::map<std::string, std::string> labels;
        labels["operation"] = operation;
        labels["device_id"] = std::to_string(device_id);
        labels["success"] = success ? "true" : "false";
        
        auto& counter = getCounter("device_operations_total", "Total number of device operations", Labels(labels));
        counter.increment();
    }
    
    void recordServiceOperation(const std::string& operation, int record_id, bool success) {
        std::map<std::string, std::string> labels;
        labels["operation"] = operation;
        labels["record_id"] = std::to_string(record_id);
        labels["success"] = success ? "true" : "false";
        
        auto& counter = getCounter("service_operations_total", "Total number of service operations", Labels(labels));
        counter.increment();
    }
};

