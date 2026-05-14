@echo off
echo Compilando proyecto...
g++ src/main.cpp src/glad.c -o programa.exe -I include -L lib -lglfw3 -lgdi32 -lopengl32
if %errorlevel% neq 0 (
    echo.
    echo [ERROR] Hubo un problema al compilar. Revisa el texto de arriba.
) else (
    echo [EXITO] Compilacion terminada. Abriendo programa...
    echo ==================================================
    programa.exe
)