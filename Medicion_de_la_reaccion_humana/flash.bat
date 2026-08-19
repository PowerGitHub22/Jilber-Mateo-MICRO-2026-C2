@echo off
set IDF_TOOLS_PATH=C:\Espressif
set IDF_PYTHON_ENV_PATH=C:\Espressif\tools\python\v5.4.4\venv
set IDF_PATH=C:\esp\v5.4.4\esp-idf
if "%~1"=="" (set PORT=COM5) else (set PORT=%~1)
powershell -NoProfile -ExecutionPolicy Bypass -Command "& 'C:\esp\v5.4.4\esp-idf\export.ps1'; & idf.py -p %PORT% flash"
pause