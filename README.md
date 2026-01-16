# Service Management System — Руководство пользователя

## Содержание

1. [Описание проекта](#описание-проекта)
2. [Архитектура](#архитектура)
3. [Технологический стек](#технологический-стек)
4. [Быстрый старт](#быстрый-старт)
5. [Структура проекта](#структура-проекта)
6. [API эндпоинты](#api-эндпоинты)
7. [Мониторинг](#мониторинг)
8. [Конфигурация](#конфигурация)
9. [Разработка](#разработка)
10. [CI/CD](#cicd)
11. [Устранение неполадок](#устранение-неполадок)

---

## Описание проекта

**Service Management System** — это система управления обслуживанием оборудования, разработанная для отслеживания:
- Устройств (принтеры, серверы, маршрутизаторы, рабочие станции, ИБП)
- Типов работ по обслуживанию (замена тонера, чистка, замена дисков, обновление прошивки)
- Истории обслуживания с расчётом следующей даты обслуживания

**Ключевые возможности:**
- REST API для управления устройствами и историей обслуживания
- Prometheus метрики для мониторинга
- Интеграция с Grafana для визуализации
- Централизованное логирование через Loki
- Веб-интерфейс для управления системой

---

## Архитектура

```
+-------------------------------------------------------------------------+
|                           Service Management System                      |
+-------------------------------------------------------------------------+
|                                                                              |
|   +-------------+    +-------------+    +-------------+    +-------------+ |
|   |   Grafana   |    | Prometheus  |    |    Loki     |    |   Web UI    | |
|   |  (визуал.)  |    |  (метрики)  |    |   (логи)    |    |   (SPA)     | |
|   +------+------+    +------+------+    +------+------+    +------+------+ |
|          |                  |                  |                  |          |
|          +------------------+------------------+------------------+          |
|                                      |                                      |
|                                 +----+----+                                |
|                                 |  Crow   |                                |
|                                 |  C++    |                                |
|                                 +----+----+                                |
|                                      |                                      |
|                                      v                                      |
|                              +---------------+                              |
|                              |   libpqxx     |                              |
|                              |   (PostgreSQL)|                              |
|                              +-------+-------+                              |
|                                      |                                      |
|                              +-------v-------+                              |
|                              |  PostgreSQL   |                              |
|                              |  (car_service)|                              |
|                              +---------------+                              |
|                                                                              |
+-------------------------------------------------------------------------+
```

---

## Технологический стек

| Компонент | Технология | Версия | Назначение |
|-----------|------------|--------|------------|
| Backend | C++ | C++23 | Основное приложение |
| Web Framework | Crow | latest | HTTP сервер |
| Database | PostgreSQL | 15-alpine | Хранение данных |
| DB Client | libpqxx | 7.7.5 | PostgreSQL C++ клиент |
| JSON | nlohmann/json | header-only | Парсинг JSON |
| Monitoring | Prometheus | v2.48.0 | Сбор метрик |
| Logging | Loki | 2.9.2 | Централизованные логи |
| Log Agent | Promtail | 2.9.2 | Сбор логов |
| Visualization | Grafana | 10.2.2 | Дашборды |
| Frontend | HTML5 + Vanilla JS | - | Веб-интерфейс |
| Containerization | Docker | latest | Изоляция сервисов |
| Build System | CMake | 3.10+ | Сборка C++ проекта |

---

## Быстрый старт

### Предварительные требования

- Docker Engine 20.10+
- Docker Compose v2.0+
- 4 ГБ RAM (минимум)
- 10 ГБ дискового пространства

### Шаг 1: Клонирование репозитория

```bash
git clone <repository-url>
cd service-system
```

### Шаг 2: Запуск всех сервисов

```bash
# Запуск в режиме демона
docker-compose up -d

# Или с просмотром логов
docker-compose up
```

### Шаг 3: Проверка запуска

```bash
# Проверка статуса контейнеров
docker-compose ps

# Проверка доступности API
curl http://localhost:18080/api/test-db

# Проверка метрик Prometheus
curl http://localhost:19090/api/v1/status/flags
```

### Шаг 4: Доступ к компонентам

| Сервис | URL | Логин | Пароль |
|--------|-----|-------|--------|
| Веб-интерфейс | http://localhost:18080 | - | - |
| Prometheus | http://localhost:19090 | - | - |
| Grafana | http://localhost:13000 | admin | admin |
| Loki | http://localhost:13100 | - | - |

---

## Структура проекта

```
service-system/
├── CMakeLists.txt              # Конфигурация сборки CMake
├── config.json                 # Конфигурация приложения
├── docker-compose.yml          # Основной compose файл
├── docker-compose.test.yml     # Compose для тестирования
├── Dockerfile                  # Docker образ приложения
├── generate_metrics.sh         # Скрипт генерации метрик
│
├── src/                        # Исходный код C++
│   ├── main.cpp               # Точка входа
│   ├── webserver.h/cpp        # HTTP сервер (Crow)
│   ├── database.h/cpp         # Работа с PostgreSQL
│   ├── logger.h               # Логирование
│   └── metrics.h              # Prometheus метрики
│
├── include/                    # Заголовочные файлы (external)
│   └── nlohmann/              # JSON библиотека
│
├── Crow/                       # Crow web framework (git submodule)
│   ├── include/
│   ├── examples/
│   └── tests/
│
├── libpqxx-7.7.5/              # PostgreSQL C++ клиент
│   ├── include/
│   ├── src/
│   └── cmake/
│
├── docker/                     # Docker конфигурации
│   ├── init.sql               # Инициализация БД
│   ├── prometheus.yml         # Конфигурация Prometheus
│   ├── promtail-local-config.yaml
│   ├── wait-for-db.sh         # Скрипт ожидания БД
│   └── grafana/               # Provisioning для Grafana
│
├── www/                        # Веб-интерфейс
│   └── index.html             # SPA приложение
│
├── tests/                      # Тесты
│   ├── test_simple.cpp
│   └── CMakeLists.txt
│
├── build/                      # Собранные артефакты
├── logs/                       # Логи приложения
└── reports/                    # Отчёты
```

---

## API эндпоинты

### Метрики

| Метод | Endpoint | Описание |
|-------|----------|----------|
| GET | `/metrics` | Prometheus метрики |

### Тестирование

| Метод | Endpoint | Описание |
|-------|----------|----------|
| GET | `/api/test-db` | Проверка подключения к БД |

### Авторизация

| Метод | Endpoint | Параметры | Описание |
|-------|----------|-----------|----------|
| POST | `/api/login` | `username`, `password` | Авторизация |
| POST | `/api/logout` | - | Выход |

**Тестовые учётные данные:**
- `admin` / `admin123` — администратор
- `user` / `user123` — обычный пользователь

### Устройства

| Метод | Endpoint | Описание |
|-------|----------|----------|
| GET | `/api/devices` | Получить все устройства |
| POST | `/api/devices` | Добавить устройство (**заблокировано**) |
| PUT | `/api/devices/<id>` | Обновить устройство |
| DELETE | `/api/devices/<id>` | Удалить устройство |

### Типы обслуживания

| Метод | Endpoint | Описание |
|-------|----------|----------|
| GET | `/api/service-types` | Получить все типы услуг |
| POST | `/api/service-types` | Добавить тип услуги |
| PUT | `/api/service-types/<id>` | Обновить тип услуги |
| DELETE | `/api/service-types/<id>` | Удалить тип услуги |

### История обслуживания

| Метод | Endpoint | Описание |
|-------|----------|----------|
| GET | `/api/service-history` | Получить историю (детализированная) |
| POST | `/api/service-history` | Добавить запись |
| GET | `/api/service-records` | Получить все записи |
| PUT | `/api/service-records/<id>` | Обновить запись |
| DELETE | `/api/service-records/<id>` | Удалить запись |

### Статические файлы

| Метод | Endpoint | Описание |
|-------|----------|----------|
| GET | `/` | Главная страница (index.html) |
| GET | `/<file>` | Статические файлы из www/ |

---

## Мониторинг

### Prometheus

**URL:** http://localhost:19090

**Доступные метрики:**
- `http_requests_total` — количество HTTP запросов
- `http_request_duration_seconds` — время обработки запросов
- `db_operations_total` — операции с БД
- `auth_attempts_total` — попытки авторизации
- `device_operations_total` — операции с устройствами
- `service_operations_total` — операции обслуживания

### Grafana

**URL:** http://localhost:13000
**Логин:** `admin`
**Пароль:** `admin`

**Преднастроенные дашборды:**
1. Service System Overview — обзор системы
2. HTTP Requests — HTTP метрики
3. Database Operations — операции БД
4. Auth Monitoring — мониторинг авторизации

### Loki

**URL:** http://localhost:13100

**Логи доступны через:**
- Grafana Explore -> Loki
- LogQL запросы: `{job="service-system"}`

---

## Конфигурация

### Файл `config.json`

```json
{
    "database": {
        "host": "db",
        "port": 5432,
        "dbname": "car_service_db",
        "user": "postgres",
        "password": "postgres"
    },
    "server": {
        "port": 8080,
        "threads": 4,
        "static_files": "./www"
    }
}
```

### Переменные окружения (Docker)

| Переменная | Значение по умолчанию | Описание |
|------------|----------------------|----------|
| `DB_HOST` | `db` | Хост базы данных |
| `DB_PORT` | `5432` | Порт базы данных |
| `APP_PORT` | `8080` | Порт приложения |
| `LOKI_HOST` | `loki` | Хост Loki |
| `GF_SECURITY_ADMIN_USER` | `admin` | Пользователь Grafana |
| `GF_SECURITY_ADMIN_PASSWORD` | `admin` | Пароль Grafana |

### Порты сервисов

| Сервис | Порт | Описание |
|--------|------|----------|
| service-system | 18080 | HTTP API и веб-интерфейс |
| db (PostgreSQL) | 15432 | PostgreSQL |
| prometheus | 19090 | Prometheus |
| grafana | 13000 | Grafana |
| loki | 13100 | Loki |

---

## Разработка

### Локальная сборка (без Docker)

#### 1. Установка зависимостей (Ubuntu/Debian)

```bash
# C++ компилятор и инструменты
sudo apt-get update
sudo apt-get install -y build-essential cmake

# PostgreSQL и библиотеки
sudo apt-get install -y libpq-dev libpqxx-dev

# Boost
sudo apt-get install -y libboost-all-dev

# libcurl
sudo apt-get install -y libcurl4-openssl-dev

# Crow framework
git clone https://github.com/CrowCpp/Crow.git
cd Crow && sudo cp -r include/* /usr/local/include/
```

#### 2. Сборка проекта

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)
```

#### 3. Запуск

```bash
./service_system ../config.json
```

### Сборка Docker образа

```bash
# Сборка образа
docker build -t service-system:latest .

# Или с использованием docker-compose
docker-compose build
```

### Тестирование

```bash
# Запуск тестов
docker-compose -f docker-compose.test.yml up --build

# Или локально
cd build && ctest
```

---

## CI/CD

### GitHub Actions Workflow

Файл: `.github/workflows/docker-publish.yml`

**Автоматизация:**
1. Сборка Docker образа на пуш в `main`
2. Тестирование контейнера
3. Публикация в Docker Hub (тег `latest`)
4. Публикация по тегам версий

### Workflow для тестирования

```bash
# Запуск тестовой среды
docker-compose -f docker-compose.test.yml up -d

# Проверка логов
docker-compose -f docker-compose.test.yml logs -f

# Остановка тестов
docker-compose -f docker-compose.test.yml down
```

---

## База данных

### Схема данных

```
+-----------------------------------------+
|              Devices                     |
+-----------------------------------------+
| device_id       SERIAL PRIMARY KEY      |
| name            VARCHAR(100) NOT NULL   |
| model           VARCHAR(100)            |
| purchase_date   DATE                    |
| status          VARCHAR(20)             |
+-----------------------------------------+
         |
         | 1:N
         v
+-----------------------------------------+
|           Service_History               |
+-----------------------------------------+
| record_id       SERIAL PRIMARY KEY      |
| device_id       INTEGER REFERENCES      |
| service_id      INTEGER REFERENCES      |
| service_date    DATE NOT NULL           |
| cost            DECIMAL(10,2)           |
| notes           TEXT                    |
| next_due_date   DATE                    |
+-----------------------------------------+
         ^
         | N:1
         |
+-----------------------------------------+
|            Service_Types                 |
+-----------------------------------------+
| service_id      SERIAL PRIMARY KEY      |
| name            VARCHAR(100) NOT NULL   |
| recommended_interval_months INT         |
| standard_cost   DECIMAL(10,2)           |
+-----------------------------------------+
```

### Тестовые данные

| Устройство | Модель | Статус |
|------------|--------|--------|
| Printer HP LaserJet | HP LaserJet Pro | active |
| Server Dell | PowerEdge R740 | active |
| Router Cisco | Cisco 2901 | maintenance |
| Workstation Dell | OptiPlex 7090 | active |
| UPS APC | Smart-UPS 1500 | active |

---

## Устранение неполадок

### Контейнер не запускается

```bash
# Проверка логов
docker-compose logs service-system

# Проверка статуса
docker-compose ps

# Перезапуск
docker-compose restart service-system
```

### Ошибка подключения к БД

```bash
# Проверка статуса PostgreSQL
docker-compose exec db pg_isready -U postgres

# Проверка подключения
docker-compose exec service-system /app/wait-for-db.sh
```

### Логи в Loki не поступают

```bash
# Проверка Promtail
docker-compose logs promtail

# Проверка статуса Loki
curl http://localhost:13100/ready
```

### Метрики не собираются

```bash
# Проверка endpoint метрик
curl http://localhost:18080/metrics

# Проверка Prometheus
curl http://localhost:19090/api/v1/query?query=up
```

### Очистка и перезапуск

```bash
# Остановка всех сервисов
docker-compose down

# Удаление томов (ВНИМАНИЕ: удалит все данные!)
docker-compose down -v

# Полный перезапуск
docker-compose down -v && docker-compose up -d
```

---

## Лицензия

Проект разработан для образовательных и демонстрационных целей.

---

## Поддержка

При возникновении проблем:
1. Проверьте логи: `docker-compose logs`
2. Проверьте статус сервисов: `docker-compose ps`
3. Убедитесь, что порты не заняты другими процессами

**Полезные команды:**

```bash
# Мониторинг ресурсов
docker stats

# Просмотр логов всех сервисов
docker-compose logs -f

# Проверка сетевых соединений
docker network ls
docker network inspect service-system_monitoring
```

