

$ErrorActionPreference = "Stop"


$ROOT = Resolve-Path "$PSScriptRoot\.."


$PROXY = Join-Path $ROOT "proxy.exe"

$logDir = Join-Path $ROOT "logs"
if (!(Test-Path $logDir)) {
    New-Item -ItemType Directory -Path $logDir | Out-Null
}

$serverOutLog = Join-Path $logDir "server_basic.out"

Write-Host "Starting proxy..."


if (!(Test-Path $PROXY)) {
    Write-Error "proxy.exe not found at $PROXY. Build first."
    Exit 1
}


$proc = Start-Process `
    -FilePath $PROXY `
    -WorkingDirectory $ROOT `
    -RedirectStandardOutput $serverOutLog `
    -NoNewWindow `
    -PassThru

Start-Sleep -Seconds 2

Write-Host "Running curl tests..."


cmd /c "curl.exe -x localhost:8888 -sS -I http://example.com" `
    > "$logDir\test1.txt" 2>&1
Write-Host "Test1 saved to logs\test1.txt"

cmd /c "curl.exe -x localhost:8888 -sS http://httpbin.org/get" `
    > "$logDir\test2.txt" 2>&1
Write-Host "Test2 saved to logs\test2.txt"


cmd /c "curl.exe -x localhost:8888 -sS -I http://httpbin.org/get" `
    > "$logDir\test3.txt" 2>&1
Write-Host "Test3 saved to logs\test3.txt"


if ($proc -and !$proc.HasExited) {
    $proc.Kill()
    $proc.WaitForExit()
    Write-Host "Proxy stopped."
}

Write-Host "Proxy stdout saved to logs\server_basic.out"
