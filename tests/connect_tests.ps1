
$scriptFolder = Split-Path -Parent $MyInvocation.MyCommand.Definition
$projRoot = Split-Path -Parent $scriptFolder 

Push-Location $projRoot

$proxyExe = Join-Path $projRoot "proxy.exe"
$logDir = Join-Path $projRoot "logs"
$configFile = Join-Path $projRoot "config\blocked_domains.txt"


if (!(Test-Path $logDir)) { 
    New-Item -ItemType Directory -Path $logDir | Out-Null 
}


if (!(Test-Path $proxyExe)) {
    Write-Error "ERROR: proxy.exe not found at $proxyExe. Please build the project first!"
    Pop-Location
    Exit 1
}


function Stop-Proxy($p) {
    if ($p -and !$p.HasExited) {
        Write-Host "Stopping proxy..."
        $p.Kill()
        $p.WaitForExit()
    }
}

try {
   
    Write-Host "Starting proxy server..."
    $proc = Start-Process -FilePath $proxyExe -NoNewWindow -PassThru
    Start-Sleep -Seconds 2 

    
    Write-Host "Testing ALLOWED CONNECT to example.com..."
    
    cmd /c "curl.exe -x localhost:8888 -sS -I https://example.com" > "$logDir\connect_allowed.txt" 2>&1
    Write-Host "Result saved to $logDir\connect_allowed.txt"

   
    if (Test-Path $configFile) {
        Write-Host "Adding example.com to blocked list temporarily..."
        Copy-Item $configFile -Destination "$configFile.bak" -Force
        "example.com" | Out-File -FilePath $configFile -Encoding ascii -Append
        
      
        Stop-Proxy $proc
        Write-Host "Restarting proxy to load new rules..."
        $proc = Start-Process -FilePath $proxyExe -NoNewWindow -PassThru
        Start-Sleep -Seconds 2

        Write-Host "Testing BLOCKED CONNECT to example.com..."
        cmd /c "curl.exe -x localhost:8888 -sS -I https://example.com" > "$logDir\connect_blocked.txt" 2>&1
        Write-Host "Result saved to $logDir\connect_blocked.txt (Should see 403 Forbidden)"
    }
    else {
        Write-Warning "Config file not found at $configFile, skipping block test."
    }

}
catch {
    Write-Error "An unexpected error occurred: $_"
}
finally {
 
    Stop-Proxy $proc

    if (Test-Path "$configFile.bak") {
        Write-Host "Restoring original config..."
        Move-Item "$configFile.bak" -Destination $configFile -Force
    }
    
    Write-Host "Test Cycle Complete."
    Pop-Location
}