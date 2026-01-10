#pragma once
#include "database.h"
#include "logger.h"
#include "metrics.h"
#include <crow.h>
#include <cfloat>
#include <string>
#include <memory>

class WebServer {
private:
    std::unique_ptr<Database> db;
    crow::SimpleApp app;
    int port;
    
    void setupRoutes();
    std::string readConfig();
    
public:
    WebServer(const std::string& config_file);
    void run();
};
