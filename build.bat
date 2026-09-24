@echo off
echo Building CI/CD Pipeline Server...
if not exist bin mkdir bin
if not exist deploy mkdir deploy
if not exist plugins mkdir plugins

:: Сборка плагина DLL
g++ -shared -o plugins/lint.dll plugins/lint_plugin.cpp -std=c++17

:: Сборка главного сервера C++
g++ -std=c++17 src/main.cpp -o bin/cicd_server.exe -lws2_32

echo Build completed!
echo Launching server at http://localhost:8080
bin\cicd_server.exe