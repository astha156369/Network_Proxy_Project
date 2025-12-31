
Write-Host "--- Starting Packaging Process ---" -ForegroundColor Cyan


if (Get-Command mingw32-make -ErrorAction SilentlyContinue) {
    Write-Host "Cleaning build artifacts..." -ForegroundColor Yellow
    try {
        mingw32-make clean
    }
    catch {
        Write-Host "Note: Could not delete some files. Ensure proxy.exe is closed." -ForegroundColor Magenta
    }
}


if (Test-Path "logs\proxy.log") { 
    try {
        Clear-Content "logs\proxy.log" -ErrorAction Stop
        Write-Host "Logs cleared for fresh evaluation." -ForegroundColor Yellow
    }
    catch {
        Write-Host "Warning: Could not clear proxy.log (file is in use)." -ForegroundColor Red
    }
}


$date = Get-Date -Format "yyyy-MM-dd"
$zipName = "ProxyProject_Submission_$date.zip"


Write-Host "Creating ZIP: $zipName" -ForegroundColor Green

Get-ChildItem -Path . -Exclude $zipName, "package.ps1" | Compress-Archive -DestinationPath $zipName -Force

Write-Host "--- Packaging Complete! ---" -ForegroundColor Cyan
Write-Host "Your file is ready at: D:\proxy-project\$zipName"