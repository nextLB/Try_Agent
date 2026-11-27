$ErrorActionPreference = "Stop"

$ProjectRoot = $PSScriptRoot
$AgentDir = Join-Path $ProjectRoot "Agent"
$BackendScript = Join-Path $AgentDir "main.py"

if (!(Test-Path -LiteralPath $BackendScript)) {
    throw "Backend script not found: $BackendScript"
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "       Harness Python Backend           " -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Script: $BackendScript" -ForegroundColor Gray
Write-Host "Listening: 127.0.0.1:9000" -ForegroundColor Gray
Write-Host "Type 'stop' and press Enter to quit." -ForegroundColor Gray
Write-Host ""

Push-Location -LiteralPath $AgentDir

try {
    & python $BackendScript
}
finally {
    Pop-Location
    Write-Host ""
    Write-Host "Python backend stopped." -ForegroundColor Yellow
}
