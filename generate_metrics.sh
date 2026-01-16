#!/bin/bash

# Script to generate various HTTP requests with different methods and status codes
# This simulates traffic for testing Prometheus metrics and Grafana dashboards

# Default to container port 18080, can be overridden
SERVER_URL="${SERVER_URL:-http://localhost:18080}"

echo "=== Starting metrics generation script ==="
echo "Server: $SERVER_URL"
echo "Press Ctrl+C to stop"
echo ""

# Function to make HTTP requests and display results
make_request() {
    local method=$1
    local path=$2
    local data=$3
    local description=$4
    
    # Make request and capture HTTP code
    if [ -n "$data" ]; then
        http_code=$(curl -s -o /dev/null -w "%{http_code}" -X "$method" "$SERVER_URL$path" \
            -H "Content-Type: application/json" \
            -d "$data" 2>/dev/null)
    else
        http_code=$(curl -s -o /dev/null -w "%{http_code}" -X "$method" "$SERVER_URL$path" 2>/dev/null)
    fi
    
    echo "[$method] $path -> $http_code ($description)"
}

# Login credentials
valid_creds='{"username":"admin","password":"admin123"}'
invalid_creds='{"username":"unknown","password":"wrong"}'

# Main loop
iteration=1
while true; do
    echo ""
    echo "========================================"
    echo "Iteration $iteration"
    echo "========================================"
    
    # GET requests
    make_request "GET" "/" "null" "Root page"
    make_request "GET" "/api/test-db" "null" "Test DB connection"
    make_request "GET" "/api/devices" "null" "Get all devices"
    make_request "GET" "/api/service-types" "null" "Get service types"
    make_request "GET" "/api/service-history" "null" "Get service history"
    make_request "GET" "/api/service-records" "null" "Get service records"
    
    # POST - Login (random success/fail)
    if [ $((RANDOM % 2)) -eq 0 ]; then
        make_request "POST" "/api/login" "$valid_creds" "Login success"
    else
        make_request "POST" "/api/login" "$invalid_creds" "Login failed"
    fi
    
    # POST - Logout
    make_request "POST" "/api/logout" "null" "Logout"
    
    # POST - Add device
    device_data='{"name":"Test Device","model":"Model-X","purchase_date":"2024-01-01","status":"active"}'
    make_request "POST" "/api/devices" "$device_data" "Add device"
    
    # POST - Add service record
    service_data='{"device_id":1,"service_id":1,"service_date":"2024-01-01","cost":100.0,"notes":"Test","next_due_date":"2025-01-01"}'
    make_request "POST" "/api/service-history" "$service_data" "Add service record"
    
    # GET - Not found (404)
    make_request "GET" "/api/nonexistent" "null" "Not found"
    
    echo ""
    echo "--- Metrics sample ---"
    curl -s "$SERVER_URL/metrics" 2>/dev/null | grep -E "^http_request_duration_seconds|^http_requests_total" | head -10 || echo "No metrics available yet"
    
    iteration=$((iteration + 1))
    echo ""
    echo "Waiting 5 seconds before next iteration..."
    sleep 5
done

