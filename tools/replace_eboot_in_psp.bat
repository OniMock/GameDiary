@echo off
setlocal enabledelayedexpansion

REM Get project root (one folder up from /tools)
set ROOT_DIR=%~dp0..
set SOURCE=%ROOT_DIR%\EBOOT.PBP
set PRX_SOURCE=%ROOT_DIR%\GameDiary.prx

set DEST_PATH=\PSP\GAME\CAT_App\Gamediary\EBOOT.PBP
set PRX_DEST=\SEPLUGINS\GameDiary.prx

if not exist "%SOURCE%" (
echo [ERROR] EBOOT.PBP not found in project root.
pause
exit /b
)

if not exist "%PRX_SOURCE%" (
echo [ERROR] GameDiary.prx not found in project root.
pause
exit /b
)

echo Searching for connected PSP...

set FOUND_DRIVE=

for %%D in (D E F G H I J K L M N O P Q R S T U V W X Y Z) do (
if exist "%%D:%DEST_PATH%" (
set FOUND_DRIVE=%%D:
goto :found
)
)

:found
if "%FOUND_DRIVE%"=="" (
echo [ERROR] PSP not found. Please connect it via USB and try again.
pause
exit /b
)

echo PSP found at %FOUND_DRIVE%

REM Ensure SEPLUGINS folder exists
if not exist "%FOUND_DRIVE%\SEPLUGINS" (
echo Creating SEPLUGINS folder...
mkdir "%FOUND_DRIVE%\SEPLUGINS"
)

echo Copying EBOOT.PBP...
copy /Y "%SOURCE%" "%FOUND_DRIVE%%DEST_PATH%" >nul

if %errorlevel% neq 0 (
echo [ERROR] Failed to copy EBOOT.
) else (
echo [OK] EBOOT deployed successfully!
)

echo Copying GameDiary.prx...
copy /Y "%PRX_SOURCE%" "%FOUND_DRIVE%%PRX_DEST%" >nul

if %errorlevel% neq 0 (
echo [ERROR] Failed to copy PRX.
) else (
echo [OK] PRX deployed successfully!
)

pause
