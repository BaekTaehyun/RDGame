@echo off
echo Running Dedicated Server...
echo Make sure you have cooked content in Saved/Cooked/WindowsServer

:: /D : 작업 디렉토리 설정 (프로젝트 루트로 설정해야 Saved/Cooked를 찾음)
START "RdGameServer" /D "%~dp0" "%~dp0Binaries\Win64\RdGameServer.exe" -log

echo Server Launched!
pause
