# Мониторинг и Метрики — Руководство

## Содержание

1. [Обзор системы мониторинга](#обзор-системы-мониторинга)
2. [Prometheus](#prometheus)
3. [Метрики приложения](#метрики-приложения)
4. [Grafana](#grafana)
5. [Loki](#loki)
6. [Примеры запросов](#примеры-запросов)
7. [Алерты](#алерты)

---

## Обзор системы мониторинга

```
+-------------------------------------------------------------------------+
|                         Система мониторинга                              |
+-------------------------------------------------------------------------+
|                                                                              |
|   +----------------+     +----------------+     +----------------+        |
|   |  Prometheus    |     |     Loki       |     |    Grafana     |        |
|   |  (сбор метрик) |     |  (логи)        |     |  (визуализация)|        |
|   +-------+--------+     +-------+--------+     +-------+--------+        |
|           |                    |                       |                  |
|           |                    |                       |                  |
|   +-------v--------+     +-----v------+       +-------v--------+          |
|   |  service-system|     |  app_logs  |       |   Dashboards  |          |
|   |  /metrics      |     |            |       |   + Alerts    |          |
|   +----------------+     +------------+       +----------------+          |
|                                                                              |
+-------------------------------------------------------------------------+
```

### Компоненты

| Компонент | Порт | URL | Назначение |
|-----------|------|-----|------------|
| Prometheus | 19090 | http://localhost:19090 | Сбор и хранение метрик |
| Grafana | 13000 | http://localhost:13000 | Дашборды и алерты |
| Loki | 13100 | http://localhost:13100 | Централизованное логирование |
| Promtail | - | - | Агент сбора логов |

---

## Prometheus

### Конфигурация

Файл: `docker/prometheus.yml`

```yaml
global:
  scrape_interval: 15s
  evaluation_interval: 15s
  external_labels:
    monitor: 'service-management-system'

scrape_configs:
  # Prometheus self-monitoring
  - job_name: 'prometheus'
    static_configs:
      - targets: ['prometheus:9090']

  # Service Management System application
  - job_name: 'service-system'
    static_configs:
      - targets: ['service-system:8080']
    metrics_path: /metrics
    scrape_interval: 5s

  # Promtail metrics
  - job_name: 'promtail'
    static_configs:
      - targets: ['promtail:9080']

  # Loki metrics
  - job_name: 'loki'
    static_configs:
      - targets: ['loki:3100']
```

### Как Prometheus собирает метрики

1. **Service System** экспонирует метрики на `/metrics`
2. **Prometheus** каждые 5 секунд опрашивает endpoint
3. Метрики хранятся в TSDB сроком 15 дней
4. **Grafana** получает данные через Prometheus API

### Доступ к Prometheus

```bash
# Web UI
open http://localhost:19090

# Проверка целей
curl http://localhost:19090/api/v1/targets

# Проверка флагов
curl http://localhost:19090/api/v1/status/flags

# Проверка конфигурации
curl http://localhost:19090/api/v1/status/config
```

---

## Метрики приложения

### HTTP Requests

| Метрика | Тип | Описание | Labels |
|---------|-----|----------|--------|
| `http_requests_total` | Counter | Общее количество HTTP запросов | method, path, status |
| `http_request_duration_seconds` | Histogram | Время обработки запроса | (без labels) |

**Пример вывода:**

```
# TYPE http_requests_total counter
# HELP http_requests_total Total number of HTTP requests
http_requests_total{method="GET",path="/api/devices",status="200"} 15
http_requests_total{method="POST",path="/api/login",status="200"} 3
http_requests_total{method="POST",path="/api/login",status="401"} 1

# TYPE http_request_duration_seconds histogram
# HELP http_request_duration_seconds HTTP request duration in seconds
http_request_duration_seconds_bucket{le="0.01"} 10
http_request_duration_seconds_bucket{le="0.025"} 18
http_request_duration_seconds_bucket{le="0.05"} 19
http_request_duration_seconds_bucket{le="0.1"} 19
http_request_duration_seconds_bucket{le="+Inf"} 19
http_request_duration_seconds_sum 0.185
http_request_duration_seconds_count 19
```

### Database Operations

| Метрика | Тип | Описание | Labels |
|---------|-----|----------|--------|
| `db_operations_total` | Counter | Операции с БД | operation, success |

**Доступные операции:**
- `connect`, `test_connection`
- `get_devices`, `add_device`, `update_device`, `delete_device`
- `get_service_types`, `add_service_type`
- `get_service_history`, `add_service_record`

### Authentication

| Метрика | Тип | Описание | Labels |
|---------|-----|----------|--------|
| `auth_attempts_total` | Counter | Попытки авторизации | username, success |
| `auth_failures_total` | Counter | Неудачные попытки | - |

### Device Operations

| Метрика | Тип | Описание | Labels |
|---------|-----|----------|--------|
| `device_operations_total` | Counter | Операции с устройствами | operation, device_id, success |

### Service Operations

| Метрика | Тип | Описание | Labels |
|---------|-----|----------|--------|
| `service_operations_total` | Counter | Операции обслуживания | operation, record_id, success |

### Примеры PromQL запросов

```promql
# Запросы в секунду по методу
rate(http_requests_total[5m])

# Среднее время ответа
rate(http_request_duration_seconds_sum[5m]) / rate(http_request_duration_seconds_count[5m])

# 95-й перцентиль времени ответа
histogram_quantile(0.95, rate(http_request_duration_seconds[5m]))

# Ошибки авторизации по пользователю
sum by (username) (rate(auth_attempts_total{success="false"}[5m]))

# Успешность операций БД
sum by (operation) (rate(db_operations_total{success="true"}[5m])) 
  / sum by (operation) (rate(db_operations_total[5m]))

# Количество запросов по endpoint
sum by (path) (rate(http_requests_total[5m]))

# HTTP ошибки (5xx)
sum by (status) (rate(http_requests_total{status=~"5.."}[5m]))
```

---

## Grafana

### Доступ

```bash
# Web UI
open http://localhost:13000

# Логин: admin
# Пароль: admin
```

### Источники данных (Data Sources)

После первого запуска Grafana автоматически настраивает:

| Источник | URL | Тип | Назначение |
|----------|-----|-----|------------|
| Prometheus | http://prometheus:9090 | Prometheus | Метрики |
| Loki | http://loki:3100 | Loki | Логи |

### Предлагаемые дашборды

#### 1. Service System Overview

```
Панели:
- HTTP Requests Rate (запросы/сек по методу)
- HTTP Response Time (гистограмма)
- Error Rate (ошибки в процентах)
- DB Operations (успешные/неуспешные)
- Auth Attempts (успешные/неуспешные)
```

**Конфигурация панели HTTP Requests:**

```json
{
  "title": "HTTP Requests Rate",
  "type": "timeseries",
  "targets": [
    {
      "expr": "sum by (method) (rate(http_requests_total[5m]))",
      "legendFormat": "{{method}}"
    }
  ],
  "fieldConfig": {
    "defaults": {
      "unit": "reqps",
      "min": 0
    }
  }
}
```

#### 2. HTTP Response Times

```json
{
  "title": "HTTP Response Time Percentiles",
  "type": "timeseries",
  "targets": [
    {
      "expr": "histogram_quantile(0.50, rate(http_request_duration_seconds[5m]))",
      "legendFormat": "p50"
    },
    {
      "expr": "histogram_quantile(0.95, rate(http_request_duration_seconds[5m]))",
      "legendFormat": "p95"
    },
    {
      "expr": "histogram_quantile(0.99, rate(http_request_duration_seconds[5m]))",
      "legendFormat": "p99"
    }
  ],
  "fieldConfig": {
    "defaults": {
      "unit": "s",
      "min": 0
    }
  }
}
```

#### 3. Database Operations

```json
{
  "title": "DB Operations Success Rate",
  "type": "gauge",
  "targets": [
    {
      "expr": "sum(db_operations_total{success=\"true\"}) / sum(db_operations_total) * 100",
      "legendFormat": "Success Rate"
    }
  ],
  "fieldConfig": {
    "defaults": {
      "unit": "percent",
      "min": 0,
      "max": 100
    }
  }
}
```

#### 4. Authentication Monitor

```json
{
  "title": "Auth Attempts",
  "type": "timeseries",
  "targets": [
    {
      "expr": "sum by (success) (rate(auth_attempts_total[5m]))",
      "legendFormat": "{{success}}"
    }
  ]
}
```

#### 5. Top Endpoints by Request Count

```json
{
  "title": "Top Endpoints",
  "type": "barchart",
  "targets": [
    {
      "expr": "topk(10, sum by (path) (rate(http_requests_total[1h])))",
      "legendFormat": "{{path}}"
    }
  ]
}
```

### Создание нового дашборда

1. Нажмите **+** -> **Dashboard**
2. Нажмите **Add panel** -> **Add query**
3. Введите PromQL запрос
4. Настройте визуализацию
5. Сохраните дашборд (Ctrl+S)

### Переменные в дашбордах

Для динамических фильтров используйте переменные:

```promql
# Переменная $method
{__name__=~"http_requests_total", method=~"$method"}

# Переменная $path
{__name__=~"http_requests_total", path=~"$path"}
```

Создание переменной:
1. Настройки дашборда -> **Variables** -> **New**
2. Тип: **Query**
3. Query: `label_values(http_requests_total, path)`
4. Применить

---

## Loki

### Доступ

```bash
# API
curl http://localhost:13100/loki/api/v1/labels

# Проверка статуса
curl http://localhost:13100/ready
```

### LogQL запросы

#### Базовый поиск

```logql
# Все логи сервиса
{job="service-system"}

# С фильтром по уровню
{job="service-system"} | level="error"

# С текстовым поиском
{job="service-system"} |= "database"
```

#### Парсинг структурированных логов

```
{job="service-system"} | json | line_format "{{.timestamp}} {{.level}} {{.message}}"
```

#### Метрики на основе логов

```logql
# Количество ошибок в минуту
count_over_time({job="service-system"} |= "error"[1m])

# Запросы по endpoint
sum by (endpoint) (rate({job="service-system"} |= "/api"[5m]))

# Уникальные IP адреса
count(count by (ip) (rate({job="service-system"} | json [5m])))
```

### Примеры Grafana с Loki

**Панель с логами:**

```json
{
  "type": "logs",
  "targets": {
    "expr": "{job=\"service-system\"} |= \"error\"",
    "legendFormat": "Errors"
  },
  "options": {
    "showLabels": false,
    "showCommonLabels": false,
    "wrapLogMessage": true,
    "enableLogDetails": true
  }
}
```

---

## Примеры запросов

### Получение метрик

```bash
# Все метрики
curl http://localhost:18080/metrics

# Только HTTP
curl http://localhost:18080/metrics | grep http_

# Только с определённым лейблом
curl http://localhost:18080/metrics | grep 'method="GET"'
```

### PromQL для анализа

```promql
# RPS (запросов в секунду)
sum(rate(http_requests_total[5m]))

# Среднее время ответа
avg(rate(http_request_duration_seconds_sum[5m]) 
    / rate(http_request_duration_seconds_count[5m]))

# p95 время ответа
histogram_quantile(0.95, 
    rate(http_request_duration_seconds_bucket[5m]))

# Ошибки 5xx
sum(rate(http_requests_total{status=~"5.."}[5m])) 
  / sum(rate(http_requests_total[5m])) * 100

# Уникальные пользователи
count(count by (username) (auth_attempts_total))

# Операции БД в секунду
sum(rate(db_operations_total[5m]))

# Процент успешных операций БД
sum(rate(db_operations_total{success="true"}[5m])) 
  / sum(rate(db_operations_total[5m])) * 100

# Горячие endpoint'ы (топ 5)
topk(5, sum by (path) (rate(http_requests_total[1h])))

# Медленные запросы (> 100ms)
http_request_duration_seconds_bucket{le="0.1"}

# Активность по часам
sum by (hour) (rate(http_requests_total[1h])) 
  * 3600
```

### LogQL для анализа логов

```logql
# Ошибки за последний час
{job="service-system"} |= "error" | logfmt 
  | __error__ = "" 
  | line_format "{{.timestamp}} {{.message}}"

# Запросы к API
{job="service-system"} |= "/api/" | json 
  | line_format "{{.timestamp}} {{.method}} {{.path}} {{.status}}"

# Медленные запросы (debug логи)
{job="service-system"} | level="debug" 
  | duration > 100ms
```

---

## Алерты

### Примеры правил алертинга

Файл: `prometheus/rules.yml`

```yaml
groups:
  - name: service-system-alerts
    rules:
      # Высокий процент ошибок
      - alert: HighErrorRate
        expr: |
          sum(rate(http_requests_total{status=~"5.."}[5m])) 
          / sum(rate(http_requests_total[5m])) > 0.05
        for: 5m
        labels:
          severity: critical
        annotations:
          summary: "Высокий процент ошибок"
          description: "Более 5% запросов возвращают ошибки 5xx"

      # Медленные ответы
      - alert: SlowResponses
        expr: |
          histogram_quantile(0.95, 
            rate(http_request_duration_seconds[5m])) > 1
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "Медленные ответы"
          description: "95-й перцентиль времени ответа > 1 секунды"

      # Много неудачных авторизаций
      - alert: ManyAuthFailures
        expr: |
          sum(rate(auth_failures_total[5m])) > 0.1
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "Много неудачных попыток входа"
          description: "Более 0.1 неудачных авторизаций в секунду"

      # Ошибки БД
      - alert: DatabaseErrors
        expr: |
          sum(rate(db_operations_total{success="false"}[5m])) > 0.01
        for: 2m
        labels:
          severity: critical
        annotations:
          summary: "Ошибки базы данных"
          description: "Обнаружены ошибки при работе с БД"

      # Сервис недоступен
      - alert: ServiceDown
        expr: up{job="service-system"} == 0
        for: 1m
        labels:
          severity: critical
        annotations:
          summary: "Сервис недоступен"
          description: "Service System не отвечает на запросы"

      # Нехватка памяти (если настроено)
      - alert: HighMemoryUsage
        expr: |
          container_memory_usage_bytes{container="service-system"} 
          / container_spec_memory_limit_bytes > 0.9
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "Высокое использование памяти"
          description: "Использование памяти более 90%"

      # Высокая нагрузка CPU
      - alert: HighCPUUsage
        expr: |
          rate(container_cpu_usage_seconds_total{container="service-system"}[5m]) 
          > 0.9
        for: 10m
        labels:
          severity: warning
        annotations:
          summary: "Высокая нагрузка CPU"
          description: "CPU загружен более чем на 90%"
```

### Настройка алертов в Grafana

1. **Alerting** -> **Notification channels** -> **New channel**
2. Тип: **Email**, **Slack**, **Telegram**, **Webhook**
3. Заполните параметры
4. Сохраните

5. На панели дашборда:
   - Нажмите на заголовок панели -> **Edit**
   - Вкладка **Alert**
   - **Create alert**
   - Настройте условия
   - Укажите notification channel

---

## Практические примеры

### Мониторинг производительности API

```promql
# Общий RPS
sum(rate(http_requests_total[1m]))

# RPS по endpoint
sum by (path) (rate(http_requests_total[1m]))

# Среднее время ответа
sum(rate(http_request_duration_seconds_sum[1m])) 
  / sum(rate(http_request_duration_seconds_count[1m]))

# p99 время ответа
histogram_quantile(0.99, 
    rate(http_request_duration_seconds[1m]))

# Количество ошибок
sum(rate(http_requests_total{status=~"5.."}[1m]))
```

### Мониторинг БД

```promql
# Операции БД в секунду
sum(rate(db_operations_total[1m]))

# Успешность операций
sum by (operation) (
  rate(db_operations_total{success="true"}[5m])
) / sum by (operation) (
  rate(db_operations_total[5m])
)

# Медленные операции
db_operations_total{some_metric > 1}
```

### Мониторинг безопасности

```promql
# Попытки авторизации
sum by (username, success) (
  rate(auth_attempts_total[5m])
)

# География (если есть IP)
topk(10, count by (ip) (
  rate(http_requests_total[1h])
))
```

---

## Troubleshooting

### Метрики не собираются

```bash
# Проверка endpoint'а
curl http://localhost:18080/metrics

# Проверка scrape в Prometheus
curl http://localhost:19090/api/v1/targets

# Проверка логов
docker-compose logs prometheus
```

### Grafana не видит данные

1. Проверьте Data Sources
2. Configuration -> Data Sources -> Prometheus/Loki
3. Нажмите **Save & Test**

### Логи не поступают в Loki

```bash
# Проверка Promtail
docker-compose logs promtail

# Проверка Loki
curl http://localhost:13100/loki/api/v1/label

# Тестовый запрос
{job="service-system"} | limit 10
```

### Alert не срабатывает

1. Проверьте выражение в Prometheus
2. Проверьте `for` (время ожидания)
3. Проверьте notification channel
4. Проверьте логи Alertmanager

