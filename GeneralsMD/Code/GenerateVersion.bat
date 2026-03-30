@echo off
REM GenerateVersion.bat - Maintains generatedVersion.h
REM Tries legacy versionUpdate/buildVersionUpdate flow first, then falls back.

setlocal enabledelayedexpansion

set "HEADER_FILE=%~dp0Main\generatedVersion.h"
set "VERSION_UPDATE_EXE=%~dp0Tools\versionUpdate\versionUpdate.exe"
set "BUILD_VERSION_UPDATE_EXE=%~dp0Tools\buildVersionUpdate\buildVersionUpdate.exe"
set "BUILD_VERSION_HEADER=%~dp0Main\buildVersion.h"

if exist "!VERSION_UPDATE_EXE!" (
    if exist "!BUILD_VERSION_UPDATE_EXE!" (
        if exist "!HEADER_FILE!" (
            echo Running legacy version tools...
            "!VERSION_UPDATE_EXE!" "!HEADER_FILE!"
            if errorlevel 1 (
                echo ERROR: versionUpdate failed
                exit /b 1
            )
            if exist "!BUILD_VERSION_HEADER!" (
                "!BUILD_VERSION_UPDATE_EXE!" "!BUILD_VERSION_HEADER!"
                if errorlevel 1 (
                    echo ERROR: buildVersionUpdate failed
                    exit /b 1
                )
            )
            endlocal
            exit /b 0
        )
    )
)

if not exist "!HEADER_FILE!" (
    echo Creating %HEADER_FILE%...
    (
        echo // Do not modify this file by hand. Auto-created fallback.
        echo #pragma once
        echo #define VERSION_STRING "1.0.0"
        echo #define VERSION_MAJOR 1
        echo #define VERSION_MINOR 0			///^< This effects the replay version number.
        echo #define VERSION_BUILDNUM 0
        echo #define VERSION_LOCALBUILDNUM 0
        echo #define VERSION_BUILDUSER "local"
        echo #define VERSION_BUILDLOC "unknown"
    ) > "!HEADER_FILE!"
    if !errorlevel! equ 0 (
        echo Successfully created generatedVersion.h
    ) else (
        echo ERROR: Failed to create generatedVersion.h
        exit /b 1
    )
) else (
    echo generatedVersion.h already exists
)

endlocal
exit /b 0
