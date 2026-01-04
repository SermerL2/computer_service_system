#include "webserver.h"
#include <fstream>
#include <iostream>
#include <chrono>

WebServer::WebServer(const std::string& config_file) : port(8080) {
    // Инициализация логгера
    Logger::getInstance().init("logs/webserver.log", LogLevel::DEBUG);
    Logger::getInstance().info("WebServer constructor started", "webserver.cpp");
    
    // Чтение конфигурации
    std::ifstream config_stream(config_file);
    if (!config_stream) {
        Logger::getInstance().error("Cannot open config file: " + config_file, "webserver.cpp");
        return;
    }
    
    Logger::getInstance().info("Config file opened: " + config_file, "webserver.cpp");
    
    try {
        json config;
        config_stream >> config;
        
        // Конфигурация базы данных
        std::string conn_str = 
            "host=" + config["database"]["host"].get<std::string>() + " " +
            "port=" + std::to_string(config["database"]["port"].get<int>()) + " " +
            "dbname=" + config["database"]["dbname"].get<std::string>() + " " +
            "user=" + config["database"]["user"].get<std::string>() + " " +
            "password=" + config["database"]["password"].get<std::string>();
        
        Logger::getInstance().info("Connecting to database...", "webserver.cpp");
        
        db = std::make_unique<Database>(conn_str);
        
        if (!db->connect()) {
            Logger::getInstance().error("Failed to connect to database", "webserver.cpp");
            return;
        }
        
        Logger::getInstance().info("Connected to database successfully", "webserver.cpp");
        
        port = config["server"]["port"].get<int>();
        Logger::getInstance().info("Server configured for port: " + std::to_string(port), "webserver.cpp");
        
        setupRoutes();
        Logger::getInstance().info("Routes configured successfully", "webserver.cpp");
        
    } catch (const std::exception& e) {
        Logger::getInstance().error(std::string("Config error: ") + e.what(), "webserver.cpp");
    }
}

void WebServer::setupRoutes() {
    // Статические файлы - правильный путь
    CROW_ROUTE(app, "/")
    ([]() {
        std::ifstream file("www/index.html");
        if (!file) {
            // Альтернативный путь
            file.open("../www/index.html");
            if (!file) {
                return crow::response(404, "Index file not found");
            }
        }
        
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        
        crow::response res;
        res.set_header("Content-Type", "text/html; charset=utf-8");
        res.body = content;
        return res;
    });
    
    // Статические файлы CSS, JS и т.д.
    CROW_ROUTE(app, "/<string>")
    ([](const std::string& filename) {
        std::string path = "www/" + filename;
        std::ifstream file(path);
        
        if (!file) {
            // Попробуем альтернативный путь
            path = "../www/" + filename;
            file.open(path);
            
            if (!file) {
                return crow::response(404, "File not found: " + filename);
            }
        }
        
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        
        crow::response res;
        
        // Определение Content-Type по расширению
        if (filename.find(".css") != std::string::npos) {
            res.set_header("Content-Type", "text/css");
        } else if (filename.find(".js") != std::string::npos) {
            res.set_header("Content-Type", "application/javascript");
        } else if (filename.find(".png") != std::string::npos) {
            res.set_header("Content-Type", "image/png");
        } else if (filename.find(".jpg") != std::string::npos || filename.find(".jpeg") != std::string::npos) {
            res.set_header("Content-Type", "image/jpeg");
        } else {
            res.set_header("Content-Type", "text/html");
        }
        
        res.body = content;
        return res;
    });
    
    // API: Тест подключения к БД
    CROW_ROUTE(app, "/api/test-db")
    ([this]() {
        bool connected = db->testConnection();
        
        json response;
        response["database_connected"] = connected;
        response["timestamp"] = std::time(nullptr);
        
        crow::response res;
        res.set_header("Content-Type", "application/json; charset=utf-8");
        res.set_header("Access-Control-Allow-Origin", "*");
        res.body = response.dump();
        return res;
    });
    
    // API: Авторизация пользователя с логированием
    CROW_ROUTE(app, "/api/login")
    .methods("POST"_method)
    ([this](const crow::request& req) {
        auto start_time = std::chrono::high_resolution_clock::now();
        std::string client_ip = req.remote_ip_address.empty() ? "unknown" : req.remote_ip_address;
        
        try {
            auto body = json::parse(req.body);
            std::string username = body["username"].get<std::string>();
            std::string password = body["password"].get<std::string>();
            
            // Логируем попытку авторизации
            Logger::getInstance().logAuth(username, true, client_ip, "Login attempt started");
            
            // Простая проверка логина и пароля (в реальном проекте использовать хеширование)
            bool auth_success = false;
            std::string auth_message;
            
            if (username == "admin" && password == "admin123") {
                auth_success = true;
                auth_message = "Administrator login successful";
                Logger::getInstance().info("Admin user authenticated successfully", "webserver.cpp");
            } else if (username == "user" && password == "user123") {
                auth_success = true;
                auth_message = "User login successful";
                Logger::getInstance().info("Regular user authenticated successfully", "webserver.cpp");
            } else {
                auth_success = false;
                auth_message = "Invalid credentials";
                Logger::getInstance().logAuth(username, false, client_ip, "Invalid username or password");
            }
            
            json response;
            response["success"] = auth_success;
            response["message"] = auth_message;
            response["username"] = username;
            response["token"] = auth_success ? "mock-jwt-token-" + username : "";
            
            auto end_time = std::chrono::high_resolution_clock::now();
            long duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
            
            Logger::getInstance().logRequest(client_ip, "POST", "/api/login", 
                                              auth_success ? 200 : 401, duration_ms);
            
            crow::response res(auth_success ? 200 : 401);
            res.set_header("Content-Type", "application/json; charset=utf-8");
            res.set_header("Access-Control-Allow-Origin", "*");
            res.body = response.dump();
            return res;
            
        } catch (const std::exception& e) {
            auto end_time = std::chrono::high_resolution_clock::now();
            long duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
            
            Logger::getInstance().logRequest(client_ip, "POST", "/api/login", 400, duration_ms);
            Logger::getInstance().error(std::string("Login error: ") + e.what(), "webserver.cpp");
            
            json response;
            response["success"] = false;
            response["error"] = e.what();
            
            crow::response res(400);
            res.set_header("Content-Type", "application/json; charset=utf-8");
            res.set_header("Access-Control-Allow-Origin", "*");
            res.body = response.dump();
            return res;
        }
    });
    
    // API: Выход пользователя с логированием
    CROW_ROUTE(app, "/api/logout")
    .methods("POST"_method)
    ([this](const crow::request& req) {
        std::string client_ip = req.remote_ip_address.empty() ? "unknown" : req.remote_ip_address;
        
        // Логируем выход пользователя
        Logger::getInstance().info("User logout successful", "webserver.cpp");
        Logger::getInstance().logRequest(client_ip, "POST", "/api/logout", 200, 1);
        
        json response;
        response["success"] = true;
        response["message"] = "Logged out successfully";
        
        crow::response res;
        res.set_header("Content-Type", "application/json; charset=utf-8");
        res.set_header("Access-Control-Allow-Origin", "*");
        res.body = response.dump();
        return res;
    });
    
    // API: Получение всех устройств
    CROW_ROUTE(app, "/api/devices")
    .methods("GET"_method)
    ([this]() {
        auto devices = db->getAllDevices();
        json result = json::array();
        
        for (const auto& device : devices) {
            json j;
            j["id"] = device.id;
            j["name"] = device.name;
            j["model"] = device.model;
            j["purchase_date"] = device.purchase_date;
            j["status"] = device.status;
            result.push_back(j);
        }
        
        crow::response res;
        res.set_header("Content-Type", "application/json; charset=utf-8");
        res.set_header("Access-Control-Allow-Origin", "*");
        res.body = result.dump();
        return res;
    });
    
    // API: Добавление нового устройства
    CROW_ROUTE(app, "/api/devices")
    .methods("POST"_method)
    ([this](const crow::request& req) {
        try {
            auto body = json::parse(req.body);
            Device device;
            device.name = body["name"].get<std::string>();
            device.model = body["model"].get<std::string>();
            device.purchase_date = body["purchase_date"].get<std::string>();
            device.status = body["status"].get<std::string>();
            
            bool success = db->addDevice(device);
            
            json response;
            response["success"] = success;
            
            crow::response res;
            res.set_header("Content-Type", "application/json; charset=utf-8");
            res.set_header("Access-Control-Allow-Origin", "*");
            res.body = response.dump();
            return res;
        } catch (const std::exception& e) {
            json response;
            response["success"] = false;
            response["error"] = e.what();
            
            crow::response res(400);
            res.set_header("Content-Type", "application/json; charset=utf-8");
            res.set_header("Access-Control-Allow-Origin", "*");
            res.body = response.dump();
            return res;
        }
    });
    
    // API: Получение всех типов услуг
    CROW_ROUTE(app, "/api/service-types")
    .methods("GET"_method)
    ([this]() {
        auto types = db->getAllServiceTypes();
        json result = json::array();
        
        for (const auto& type : types) {
            json j;
            j["id"] = type.id;
            j["name"] = type.name;
            j["recommended_interval_months"] = type.recommended_interval_months;
            j["standard_cost"] = type.standard_cost;
            result.push_back(j);
        }
        
        crow::response res;
        res.set_header("Content-Type", "application/json; charset=utf-8");
        res.set_header("Access-Control-Allow-Origin", "*");
        res.body = result.dump();
        return res;
    });
    
    // API: Получение истории обслуживания (детализированная с JOIN)
    CROW_ROUTE(app, "/api/service-history")
    .methods("GET"_method)
    ([this]() {
        auto history = db->getDetailedServiceHistory();
        
        crow::response res;
        res.set_header("Content-Type", "application/json; charset=utf-8");
        res.set_header("Access-Control-Allow-Origin", "*");
        res.body = history.dump();
        return res;
    });
    
    // API: Добавление записи обслуживания
    CROW_ROUTE(app, "/api/service-history")
    .methods("POST"_method)
    ([this](const crow::request& req) {
        try {
            auto body = json::parse(req.body);
            ServiceRecord record;
            record.device_id = body["device_id"].get<int>();
            record.service_id = body["service_id"].get<int>();
            record.service_date = body["service_date"].get<std::string>();
            record.cost = body["cost"].get<double>();
            record.notes = body["notes"].get<std::string>();
            record.next_due_date = body["next_due_date"].get<std::string>();
            
            bool success = db->addServiceRecord(record);
            
            json response;
            response["success"] = success;
            
            crow::response res;
            res.set_header("Content-Type", "application/json; charset=utf-8");
            res.set_header("Access-Control-Allow-Origin", "*");
            res.body = response.dump();
            return res;
        } catch (const std::exception& e) {
            json response;
            response["success"] = false;
            response["error"] = e.what();
            
            crow::response res(400);
            res.set_header("Content-Type", "application/json; charset=utf-8");
            res.set_header("Access-Control-Allow-Origin", "*");
            res.body = response.dump();
            return res;
        }
    });
    
    // API: Получение всех записей обслуживания (простой вариант)
    CROW_ROUTE(app, "/api/service-records")
    .methods("GET"_method)
    ([this]() {
        auto records = db->getAllServiceRecords();
        json result = json::array();
        
        for (const auto& record : records) {
            json j;
            j["id"] = record.id;
            j["device_id"] = record.device_id;
            j["service_id"] = record.service_id;
            j["service_date"] = record.service_date;
            j["cost"] = record.cost;
            j["notes"] = record.notes;
            j["next_due_date"] = record.next_due_date;
            result.push_back(j);
        }
        
        crow::response res;
        res.set_header("Content-Type", "application/json; charset=utf-8");
        res.set_header("Access-Control-Allow-Origin", "*");
        res.body = result.dump();
        return res;
    });
}

void WebServer::run() {
    std::cout << "Starting server on port " << port << std::endl;
    app.port(port).multithreaded().run();
}