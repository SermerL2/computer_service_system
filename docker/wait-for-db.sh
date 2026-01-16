#!/bin/sh
# Скрипт ожидания готовности PostgreSQL

set -e

TIMEOUT=30
INTERVAL=2
ELAPSED=0

echo "Waiting for PostgreSQL to be ready..."

while ! pg_isready -h "${DB_HOST:-db}" -p "${DB_PORT:-5432}" -U "${DB_USER:-postgres}" > /dev/null 2>&1; do
    ELAPSED=$((ELAPSED + INTERVAL))
    
    if [ $ELAPSED -ge $TIMEOUT ]; then
        echo "Timeout waiting for PostgreSQL"
        exit 1
    fi
    
    echo "PostgreSQL is not ready yet. Waiting ${INTERVAL}s... (${ELAPSED}/${TIMEOUT}s)"
    sleep $INTERVAL
done

echo "PostgreSQL is ready!"
echo "Starting application: $@"

exec "$@"

