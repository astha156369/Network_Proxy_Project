
$scriptFolder = Split-Path -Parent $MyInvocation.MyCommand.Definition
$projRoot = Split-Path -Parent $scriptFolder 

Push-Location $projRoot

$proxyExe = Join-Path $projRoot "proxy.exe"
$logDir = Join-Path $projRoot "logs"


if (!(Test-Path $logDir)) { 
    New-Item -ItemType Directory -Path $logDir | Out-Null 
}


if (!(Test-Path $proxyExe)) {
    Write-Error "ERROR: proxy.exe not found. Please build the project first."
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
    
    Write-Host "Starting proxy for concurrency test..."
    $proc = Start-Process -FilePath $proxyExe -NoNewWindow -PassThru
    Start-Sleep -Seconds 2 

    
    Write-Host "Spawning 50 parallel curl processes..."
    $processList = @()

    for ($i = 1; $i -le 50; $i++) {
        $outfile = Join-Path $logDir ("concurrency_" + $i + ".txt")
        
        $p = Start-Process -FilePath "curl.exe" -ArgumentList "-x localhost:8888", "-sS", "-I", "http://httpbin.org/get" `
             -RedirectStandardOutput $outfile -NoNewWindow -PassThru
        $processList += $p
    }

    Write-Host "All processes spawned. Waiting for them to complete..."

    
    $timeout = 20 
    $elapsed = 0
    while (($processList | Where-Object { !$_.HasExited }) -and ($elapsed -lt $timeout)) {
        Start-Sleep -Seconds 1
        $elapsed++
        $remaining = ($processList | Where-Object { !$_.HasExited }).Count
        Write-Host "Waiting... $remaining jobs still running." -NoNewline
        Write-Host " `r" -NoNewline 
    }

    Write-Host "`nAll curl jobs completed or timed out."

} catch {
    Write-Error "An error occurred: $_"
} finally {
    
    Stop-Proxy $proc
    Write-Host "Concurrency test cycle complete."
    Pop-Location
}