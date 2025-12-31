
$ErrorActionPreference = "Stop"
$projRoot = Get-Location

Write-Host "--- Starting Full Build and Test Cycle ---" -ForegroundColor Cyan

Write-Host "[1/4] Compiling Proxy..." -ForegroundColor Yellow
g++ -Iinclude src\main.cpp src\proxy_server.cpp src\filter_manager.cpp src\logger.cpp src\metrics.cpp -lws2_32 -o proxy.exe

Write-Host "[2/4] Running CONNECT and Filter tests..." -ForegroundColor Yellow
powershell -ExecutionPolicy Bypass -File .\tests\connect_tests.ps1

Write-Host "[3/4] Running Stress/Concurrency tests..." -ForegroundColor Yellow
powershell -ExecutionPolicy Bypass -File .\tests\concurrency_test.ps1


Write-Host "[4/4] Collecting logs for submission..." -ForegroundColor Yellow
powershell -ExecutionPolicy Bypass -File .\tests\collect_sample_log.ps1

Write-Host "`nDONE! Check 'logs\sample_run.log' for your final results." -ForegroundColor Green
