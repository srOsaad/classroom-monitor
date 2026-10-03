@echo off

setlocal
set "RUN_KEY=HKCU\Software\Microsoft\Windows\CurrentVersion\Run"
set "RUN_VALUE=ClassroomStudent"
set "APP_KEY=HKCU\Software\Classroom\Student"

echo Classroom Student - uninstall
echo.

reg query "%RUN_KEY%" /v "%RUN_VALUE%" >nul 2>&1
if %errorlevel%==0 (
    reg delete "%RUN_KEY%" /v "%RUN_VALUE%" /f >nul 2>&1
    if errorlevel 1 (
        echo [!] Failed to remove the auto-start entry.
    ) else (
        echo [+] Removed auto-start entry "%RUN_VALUE%".
    )
) else (
    echo [ ] No auto-start entry found ^(already removed^).
)

reg query "%APP_KEY%" >nul 2>&1
if %errorlevel%==0 (
    reg delete "%APP_KEY%" /f >nul 2>&1
    if errorlevel 1 (
        echo [!] Failed to remove stored settings at %APP_KEY%.
    ) else (
        echo [+] Removed stored settings at %APP_KEY%.
    )
) else (
    echo [ ] No stored settings found ^(already removed^).
)

echo.
echo Done. The client will no longer start automatically on login.
echo You may now delete the Student executable if you wish.
echo.
pause
endlocal
