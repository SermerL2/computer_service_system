# Dockerfile для Service Management System на Alpine Linux
# Alpine более компактен и лучше подходит для production

FROM alpine:3.19 AS builder

# Установка зависимостей для сборки
RUN apk add --no-cache \
    build-base \
    cmake \
    postgresql-dev \
    boost-dev \
    curl-dev

WORKDIR /app

# Копируем libpqxx 7.7.5 исходники
COPY libpqxx-7.7.5/ /app/libpqxx-7.7.5/

# Собираем и устанавливаем libpqxx 7.7.5 из исходников
# Используем C++23 для совместимости с std::span, std::chrono::year_month_day, std::unreachable
# Строим как статическую библиотеку для Alpine
RUN mkdir -p /app/libpqxx-build && \
    cd /app/libpqxx-build && \
    cmake /app/libpqxx-7.7.5 \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_CXX_STANDARD=23 \
        -DSKIP_BUILD_TEST=ON \
        -DBUILD_DOC=OFF && \
    make -j$(nproc) && \
    make install && \
    rm -rf /app/libpqxx-build /app/libpqxx-7.7.5

# Копируем Crow framework
COPY Crow/ /app/Crow/

# Копируем nlohmann/json
COPY include/ /app/include/

# Копируем исходный код приложения
COPY src/ /app/src/
COPY CMakeLists.txt /app/
COPY config.json /app/config.json

# Сборка приложения
# Используем C++23 для совместимости с libpqxx 7.7.5
RUN mkdir -p /app/build && \
    cd /app/build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_STANDARD=23 && \
    make -j$(nproc) && \
    cp service_system /app/ && \
    rm -rf /app/build


# Финальный образ
FROM alpine:3.19

# Установка runtime зависимостей
# Важно: libpq (PostgreSQL C interface) НЕОБХОДИМ для libpqxx
RUN apk add --no-cache \
    boost \
    libcurl \
    postgresql-libs \
    postgresql-client

# Копируем libpqxx статическую библиотеку из builder этапа
COPY --from=builder /usr/local/lib/libpqxx*.a /usr/local/lib/

# Создание пользователя
RUN addgroup -g 1000 appgroup && \
    adduser -u 1000 -G appgroup -s /bin/sh -D appuser

WORKDIR /app

# Копирование собранного приложения
COPY --from=builder /app/service_system /app/
COPY --from=builder /app/config.json /app/config.json

# Копирование www папки
COPY www/ /app/www/

# Копирование скрипта ожидания БД
COPY docker/wait-for-db.sh /app/wait-for-db.sh
RUN chmod +x /app/wait-for-db.sh

# Создание директории для логов
RUN mkdir -p /app/logs && \
    chown -R appuser:appgroup /app

USER appuser

# Переменные окружения
ENV APP_CONFIG=/app/config.json
ENV APP_PORT=8080
ENV LOKI_HOST=loki
ENV DB_HOST=db

# Healthcheck - используем 127.0.0.1 вместо localhost для избежания проблем с IPv6 в BusyBox wget
HEALTHCHECK --interval=30s --timeout=10s --start-period=15s --retries=3 \
    CMD wget --no-verbose --tries=1 --spider http://127.0.0.1:${APP_PORT}/metrics || exit 1

# Запуск приложения
CMD ["/app/wait-for-db.sh", "/app/service_system", "/app/config.json"]

