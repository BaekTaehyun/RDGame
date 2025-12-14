@echo off
echo Launching Client and connecting to Localhost...

:: 실행 파일 경로 설정 (요청하신 경로)
:: 파일명이 RdGame.exe가 맞는지 확인하세요.
set EXECUTABLE="F:\Projects\RD\Windows\RdGame.exe"

:: 127.0.0.1 : 로컬 서버 접속
:: -windowed : 창 모드
:: -ResX/Y : 해상도
START "RdGameClient" %EXECUTABLE% 127.0.0.1 -windowed -ResX=1280 -ResY=720

echo Client Launched!
pause
