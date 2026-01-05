@echo off
setlocal
echo ===================================================
echo  Antigravity MCP Config Generator
echo ===================================================

set "SCRIPT_DIR=%~dp0..\Script"
set "BRIDGE_SCRIPT=%SCRIPT_DIR%\unreal_mcp_bridge.py"
set "TARGET_DIR=%LOCALAPPDATA%\Programs\Antigravity"
set "CONFIG_FILE=%TARGET_DIR%\mcp_config.json"

:: Check if Bridge Script exists
if not exist "%BRIDGE_SCRIPT%" (
    echo [ERROR] Could not find bridge script at: %BRIDGE_SCRIPT%
    echo Please make sure you are running this from the Tools folder.
    pause
    exit /b 1
)

:: Check if Antigravity is installed (Folder check)
if not exist "%TARGET_DIR%" (
    echo [WARNING] Antigravity installation folder not found at: %TARGET_DIR%
    echo You may need to create the file manually or check installation.
    echo.
    echo Config Content to Copy:
    goto :PrintConfig
)

echo [INFO] Found Antigravity folder.
echo [INFO] Writing config to: %CONFIG_FILE%

(
echo {
echo   "mcpServers": {
echo     "unreal-mcp": {
echo       "command": "python",
echo       "args": [
echo         "%BRIDGE_SCRIPT:\=\\%"
echo       ],
echo       "env": {
echo         "PYTHONUTF8": "1"
echo       }
    }
  }
}
) > "%CONFIG_FILE%"

echo.
echo [SUCCESS] Config file created!
echo Please Restart Antigravity (Reload Window) to apply changes.
goto :End

:PrintConfig
echo {
echo   "mcpServers": {
echo     "unreal-mcp": {
echo       "command": "python",
echo       "args": [
echo         "%BRIDGE_SCRIPT:\=\\%"
echo       ],
echo       "env": {
echo         "PYTHONUTF8": "1"
echo       }
    }
  }
}

:End
echo.
pause
