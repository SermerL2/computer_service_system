-- Инициализация базы данных для Service Management System

-- Удаляем существующие таблицы (если есть)
DROP TABLE IF EXISTS Service_History CASCADE;
DROP TABLE IF EXISTS Service_Types CASCADE;
DROP TABLE IF EXISTS Devices CASCADE;

-- Таблица 1: Устройства
CREATE TABLE Devices (
    device_id SERIAL PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    model VARCHAR(100),
    purchase_date DATE,
    status VARCHAR(20) DEFAULT 'active'
);

-- Таблица 2: Типы работ
CREATE TABLE Service_Types (
    service_id SERIAL PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    recommended_interval_months INT,
    standard_cost DECIMAL(10,2)
);

-- Таблица 3: История обслуживания
CREATE TABLE Service_History (
    record_id SERIAL PRIMARY KEY,
    device_id INT REFERENCES Devices(device_id),
    service_id INT REFERENCES Service_Types(service_id),
    service_date DATE NOT NULL,
    cost DECIMAL(10,2),
    notes TEXT,
    next_due_date DATE
);

-- Индекс для поиска просроченного обслуживания
CREATE INDEX idx_due_dates ON Service_History(next_due_date) 
WHERE next_due_date IS NOT NULL;

-- Вставляем тестовые данные
INSERT INTO Devices (name, model, purchase_date, status) VALUES
    ('Printer HP LaserJet', 'HP LaserJet Pro', '2023-01-15', 'active'),
    ('Server Dell', 'PowerEdge R740', '2022-06-10', 'active'),
    ('Router Cisco', 'Cisco 2901', '2021-03-20', 'maintenance'),
    ('Workstation Dell', 'OptiPlex 7090', '2023-02-28', 'active'),
    ('UPS APC', 'Smart-UPS 1500', '2022-11-05', 'active');

INSERT INTO Service_Types (name, recommended_interval_months, standard_cost) VALUES
    ('Замена тонера', 6, 50.00),
    ('Чистка и смазка', 12, 100.00),
    ('Замена HDD', 36, 150.00),
    ('Обновление прошивки', 12, 75.00),
    ('Проверка состояния', 3, 30.00);

INSERT INTO Service_History (device_id, service_id, service_date, cost, notes, next_due_date) VALUES
    (1, 1, '2023-07-15', 50.00, 'Замена тонера TN-1030', '2024-01-15'),
    (1, 2, '2023-01-15', 100.00, 'Полная чистка', '2024-01-15'),
    (2, 4, '2023-06-10', 75.00, 'Обновление BIOS', '2024-06-10'),
    (3, 5, '2023-09-20', 30.00, 'Проверка конфигурации', '2023-12-20'),
    (4, 5, '2023-08-28', 30.00, 'Проверка системы', '2023-11-28'),
    (5, 2, '2023-05-05', 100.00, 'Замена батарей', '2024-05-05');

-- Проверка
SELECT 'Devices count: ' || COUNT(*) FROM Devices;
SELECT 'Service_Types count: ' || COUNT(*) FROM Service_Types;
SELECT 'Service_History count: ' || COUNT(*) FROM Service_History;

