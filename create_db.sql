DROP TABLE Devices CASCADE;
DROP TABLE Service_Types CASCADE;
DROP TABLE Service_History CASCADE;

-- Таблица 1: Устройства (5 атрибутов - оригинал + 1)
CREATE TABLE Devices (
    device_id SERIAL PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    model VARCHAR(100),
    purchase_date DATE,
    status VARCHAR(20) DEFAULT 'active'  -- дополнительный атрибут
);

-- Таблица 2: Типы работ (4 атрибута - оригинал)
CREATE TABLE Service_Types (
    service_id SERIAL PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    recommended_interval_months INT,
    standard_cost DECIMAL(10,2)  -- дополнительный атрибут
);

-- Таблица 3: История обслуживания (6 атрибутов - оригинал)
CREATE TABLE Service_History (
    record_id SERIAL PRIMARY KEY,
    device_id INT REFERENCES Devices(device_id),
    service_id INT REFERENCES Service_Types(service_id),
    service_date DATE NOT NULL,
    cost DECIMAL(10,2),
    notes TEXT,
    next_due_date DATE
);

-- Простой индекс для поиска просроченного обслуживания
CREATE INDEX idx_due_dates ON Service_History(next_due_date) 
WHERE next_due_date IS NOT NULL;