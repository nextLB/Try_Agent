$ErrorActionPreference = "Stop"

$ProjectRoot = $PSScriptRoot
$SourceDir = Join-Path $ProjectRoot "Harness_APP"
$BuildDir = Join-Path $ProjectRoot "build"
$ExePath = Join-Path $BuildDir "main.exe"

$env:Path = "D:\Qt6\Tools\mingw1510_64\bin;D:\Qt6\6.12.1\mingw_64\bin;" + $env:Path

if (!(Test-Path -LiteralPath (Join-Path $SourceDir "CMakeLists.txt"))) {
    throw "CMakeLists.txt not found: $SourceDir"
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "        Harness App Build & Run         " -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

if (!(Test-Path -LiteralPath $BuildDir)) {
    New-Item -ItemType Directory -Path $BuildDir | Out-Null
}

try {
    Write-Host "`n[1/3] Configuring CMake..." -ForegroundColor Yellow

    cmake -S $SourceDir -B $BuildDir `
        -G "MinGW Makefiles" `
        -DCMAKE_BUILD_TYPE=Release

    if ($LASTEXITCODE -ne 0) {
        throw "CMake configuration failed."
    }

    Write-Host "`n[2/3] Building application..." -ForegroundColor Yellow

    cmake --build $BuildDir --parallel

    if ($LASTEXITCODE -ne 0) {
        throw "Build failed."
    }

    if (!(Test-Path -LiteralPath $ExePath)) {
        throw "Executable not found: $ExePath"
    }

    Write-Host "`n[3/3] Starting application..." -ForegroundColor Green

    & $ExePath

    if ($LASTEXITCODE -ne 0) {
        Write-Warning "Application exited with code $LASTEXITCODE."
    }
}
finally {
    if (Test-Path -LiteralPath $BuildDir) {
        Remove-Item -LiteralPath $BuildDir -Recurse -Force
        Write-Host "`n[Cleanup] Build directory removed." -ForegroundColor Green
    }
}
