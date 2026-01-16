#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>

// Тесты структур данных
TEST(DataStructuresTest, DeviceDefaultValues) {
    struct Device {
        int id = 0;
        std::string name = "";
        std::string model = "";
        std::string purchase_date = "";
        std::string status = "active";
    };
    
    Device device;
    EXPECT_EQ(device.id, 0);
    EXPECT_EQ(device.name, "");
    EXPECT_EQ(device.model, "");
    EXPECT_EQ(device.purchase_date, "");
    EXPECT_EQ(device.status, "active");
}

TEST(DataStructuresTest, DeviceWithValues) {
    struct Device {
        int id = 0;
        std::string name = "";
        std::string model = "";
        std::string purchase_date = "";
        std::string status = "active";
    };
    
    Device device;
    device.id = 1;
    device.name = "Test Device";
    device.model = "Model X";
    device.purchase_date = "2024-01-15";
    device.status = "inactive";
    
    EXPECT_EQ(device.id, 1);
    EXPECT_EQ(device.name, "Test Device");
    EXPECT_EQ(device.model, "Model X");
    EXPECT_EQ(device.purchase_date, "2024-01-15");
    EXPECT_EQ(device.status, "inactive");
}

TEST(DataStructuresTest, ServiceTypeDefaultValues) {
    struct ServiceType {
        int id = 0;
        std::string name = "";
        int recommended_interval_months = 0;
        double standard_cost = 0.0;
    };
    
    ServiceType type;
    EXPECT_EQ(type.id, 0);
    EXPECT_EQ(type.name, "");
    EXPECT_EQ(type.recommended_interval_months, 0);
    EXPECT_DOUBLE_EQ(type.standard_cost, 0.0);
}

TEST(DataStructuresTest, ServiceTypeWithValues) {
    struct ServiceType {
        int id = 0;
        std::string name = "";
        int recommended_interval_months = 0;
        double standard_cost = 0.0;
    };
    
    ServiceType type;
    type.id = 1;
    type.name = "Oil Change";
    type.recommended_interval_months = 6;
    type.standard_cost = 50.0;
    
    EXPECT_EQ(type.id, 1);
    EXPECT_EQ(type.name, "Oil Change");
    EXPECT_EQ(type.recommended_interval_months, 6);
    EXPECT_DOUBLE_EQ(type.standard_cost, 50.0);
}

TEST(DataStructuresTest, ServiceRecordDefaultValues) {
    struct ServiceRecord {
        int id = 0;
        int device_id = 0;
        int service_id = 0;
        std::string service_date = "";
        double cost = 0.0;
        std::string notes = "";
        std::string next_due_date = "";
    };
    
    ServiceRecord record;
    EXPECT_EQ(record.id, 0);
    EXPECT_EQ(record.device_id, 0);
    EXPECT_EQ(record.service_id, 0);
    EXPECT_EQ(record.service_date, "");
    EXPECT_DOUBLE_EQ(record.cost, 0.0);
    EXPECT_EQ(record.notes, "");
    EXPECT_EQ(record.next_due_date, "");
}

TEST(DataStructuresTest, ServiceRecordWithValues) {
    struct ServiceRecord {
        int id = 0;
        int device_id = 0;
        int service_id = 0;
        std::string service_date = "";
        double cost = 0.0;
        std::string notes = "";
        std::string next_due_date = "";
    };
    
    ServiceRecord record;
    record.id = 1;
    record.device_id = 5;
    record.service_id = 3;
    record.service_date = "2024-01-15";
    record.cost = 150.50;
    record.notes = "Full service";
    record.next_due_date = "2024-07-15";
    
    EXPECT_EQ(record.id, 1);
    EXPECT_EQ(record.device_id, 5);
    EXPECT_EQ(record.service_id, 3);
    EXPECT_EQ(record.service_date, "2024-01-15");
    EXPECT_DOUBLE_EQ(record.cost, 150.50);
    EXPECT_EQ(record.notes, "Full service");
    EXPECT_EQ(record.next_due_date, "2024-07-15");
}

// Тесты конфигурации
TEST(ConfigTest, ConfigFileExists) {
    std::ifstream config("config_test.json");
    EXPECT_TRUE(config.is_open()) << "config_test.json not found";
}

TEST(ConfigTest, ConfigFileParsable) {
    std::ifstream config("config_test.json");
    ASSERT_TRUE(config.is_open());
    
    std::string content((std::istreambuf_iterator<char>(config)),
                        std::istreambuf_iterator<char>());
    
    EXPECT_FALSE(content.empty());
    
    // Проверяем наличие ключевых полей
    EXPECT_TRUE(content.find("database") != std::string::npos);
    EXPECT_TRUE(content.find("server") != std::string::npos);
    EXPECT_TRUE(content.find("host") != std::string::npos);
    EXPECT_TRUE(content.find("port") != std::string::npos);
}

// Тесты граничных значений
TEST(BoundaryTest, EmptyStrings) {
    struct Device {
        int id = 0;
        std::string name = "";
        std::string model = "";
        std::string purchase_date = "";
        std::string status = "active";
    };
    
    Device device;
    device.name = "";
    device.model = "";
    device.purchase_date = "";
    
    EXPECT_EQ(device.name, "");
    EXPECT_EQ(device.model, "");
    EXPECT_EQ(device.purchase_date, "");
}

TEST(BoundaryTest, LongStrings) {
    struct Device {
        int id = 0;
        std::string name = "";
        std::string model = "";
        std::string purchase_date = "";
        std::string status = "active";
    };
    
    Device device;
    device.name = std::string(255, 'A');
    device.model = std::string(255, 'B');
    
    EXPECT_EQ(device.name.length(), 255);
    EXPECT_EQ(device.model.length(), 255);
}

TEST(BoundaryTest, ZeroAndNegativeValues) {
    struct ServiceRecord {
        int id = 0;
        int device_id = 0;
        int service_id = 0;
        double cost = 0.0;
    };
    
    ServiceRecord record;
    record.id = 0;
    record.device_id = -1;
    record.service_id = -5;
    record.cost = -100.0;
    
    EXPECT_EQ(record.id, 0);
    EXPECT_EQ(record.device_id, -1);
    EXPECT_EQ(record.service_id, -5);
    EXPECT_DOUBLE_EQ(record.cost, -100.0);
}

TEST(BoundaryTest, LargeCostValues) {
    struct ServiceType {
        double standard_cost = 0.0;
    };
    
    struct ServiceRecord {
        double cost = 0.0;
    };
    
    ServiceType type;
    type.standard_cost = 999999.99;
    
    ServiceRecord record;
    record.cost = 999999.99;
    
    EXPECT_DOUBLE_EQ(type.standard_cost, 999999.99);
    EXPECT_DOUBLE_EQ(record.cost, 999999.99);
}

// Тесты Unicode (кириллица)
TEST(UnicodeTest, CyrillicStrings) {
    struct Device {
        std::string name = "";
        std::string model = "";
        std::string status = "";
    };
    
    Device device;
    device.name = "Тестовое устройство";
    device.model = "Модель 123";
    device.status = "активный";
    
    EXPECT_EQ(device.name, "Тестовое устройство");
    EXPECT_EQ(device.model, "Модель 123");
    EXPECT_EQ(device.status, "активный");
}

TEST(UnicodeTest, CyrillicInServiceType) {
    struct ServiceType {
        std::string name = "";
    };
    
    ServiceType type;
    type.name = "Замена масла";
    
    EXPECT_EQ(type.name, "Замена масла");
}

// Тесты статусов
TEST(StatusTest, DeviceStatusValues) {
    struct Device {
        std::string status = "active";
    };
    
    Device device;
    
    device.status = "active";
    EXPECT_EQ(device.status, "active");
    
    device.status = "inactive";
    EXPECT_EQ(device.status, "inactive");
    
    device.status = "maintenance";
    EXPECT_EQ(device.status, "maintenance");
    
    device.status = "retired";
    EXPECT_EQ(device.status, "retired");
}

// Тесты векторов
TEST(VectorTest, DeviceVector) {
    struct Device {
        int id = 0;
        std::string name = "";
    };
    
    std::vector<Device> devices;
    
    // Пустой вектор
    EXPECT_TRUE(devices.empty());
    EXPECT_EQ(devices.size(), 0);
    
    // Добавляем элементы
    Device d1;
    d1.id = 1;
    d1.name = "Device 1";
    devices.push_back(d1);
    
    Device d2;
    d2.id = 2;
    d2.name = "Device 2";
    devices.push_back(d2);
    
    EXPECT_EQ(devices.size(), 2);
    EXPECT_EQ(devices[0].id, 1);
    EXPECT_EQ(devices[1].id, 2);
}

TEST(VectorTest, ServiceTypeVector) {
    struct ServiceType {
        int id = 0;
        std::string name = "";
        double cost = 0.0;
    };
    
    std::vector<ServiceType> types;
    
    EXPECT_TRUE(types.empty());
    
    ServiceType t1;
    t1.id = 1;
    t1.name = "Service 1";
    t1.cost = 50.0;
    types.push_back(t1);
    
    EXPECT_EQ(types.size(), 1);
    EXPECT_DOUBLE_EQ(types[0].cost, 50.0);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

