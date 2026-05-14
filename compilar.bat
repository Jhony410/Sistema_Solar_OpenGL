@echo off
chcp 65001 > nul
echo.
echo ============================================================
echo   Sistema Solar 3D  -  Compilando...
echo ============================================================
echo.

g++ src/main.cpp src/glad.c ^
    -Iinclude -Llib ^
    -lglfw3 -lopengl32 -lgdi32 ^
    -std=c++17 -O2 ^
    -o programa.exe

if %ERRORLEVEL% == 0 (
    echo.
    echo   [OK] Compilacion exitosa!
    echo.
    echo   Controles:
    echo     WASD       = mover camara
    echo     Mouse      = rotar camara
    echo     Scroll     = velocidad de camara
    echo     E / Q      = subir / bajar
    echo     O          = toggle orbitas
    echo     P          = pausar simulacion
    echo     + / -      = acelerar / reducir velocidad
    echo     R          = resetear camara
    echo     ESC        = salir
    echo.
    echo   Iniciando programa...
    echo.
    programa.exe
) else (
    echo.
    echo   [ERROR] La compilacion fallo. Revisa los mensajes arriba.
    echo.
    pause
)
