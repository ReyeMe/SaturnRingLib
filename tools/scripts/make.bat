@echo off

:: Save the original PATH at script startup
SET "OLD_PATH=%PATH%"

:: 1. Save the target, default to debug if empty
SET "TARGET=%~1"
IF "%TARGET%"=="" SET "TARGET=debug"

:: 2. Check for custom compiler directory
SET "COMPILER_DIR=../../Compiler"
SET "HAS_CUSTOM_PATH=0"

IF NOT "%2" == "" (
    IF NOT "%~2" == "" (
        powershell write-host -fore Red Using custom compiler path
        SET "COMPILER_DIR=%~2"
        SET "HAS_CUSTOM_PATH=1"
    )
)

setlocal enabledelayedexpansion

:: 3. POLYGLOT SAFE PARSER:
:: This loops through the raw line string, breaking it apart by spaces safely.
:: It completely filters out targets and compiler paths without triggering drive errors.
SET "MAKE_ARGS="
for %%A in (%*) do (
    set "token=%%~A"
    
    :: 1. Strip out any backslashes or forward slashes to make path checking safe
    set "clean_token=!token:\=!"
    set "clean_token=!clean_token:/=!"
    
    :: 2. Only process if it's not the target word and not part of a compiler path string
    if /I not "!clean_token!"=="clean" if /I not "!clean_token!"=="debug" if /I not "!clean_token!"=="release" (
        if not "!clean_token:~0,2!"==".." if /I not "!clean_token!"=="Compiler" (
            
            :: 3. Build the clean argument string
            if "!MAKE_ARGS!"=="" (
                SET "MAKE_ARGS=%%~A"
            ) else (
                SET "MAKE_ARGS=!MAKE_ARGS! %%~A"
            )
        )
    )
)

:: Export variables safely out of local scope 
FOR /F "delims=" %%A in ("!MAKE_ARGS!") do (
    endlocal
    SET "MAKE_ARGS=%%A"
)

echo Make args are: "%MAKE_ARGS%"

:: 3. Environment Setup
:: Absolute path, so builds that run "make -C <subfolder>" (Tests campaigns) still find the compiler
FOR %%I IN ("%COMPILER_DIR%") DO SET "COMPILER_DIR=%%~fI"
SET "UTIL_DIR=%COMPILER_DIR%\Other Utilities"
SET "MSYS_DIR=%COMPILER_DIR%\msys2\usr\bin"
SET "BIN_DIR=%COMPILER_DIR%\sh2eb-elf\bin"

:: Temporarily modify PATH (Fixed path variable layout for make binary lookup)
SET "PATH=%UTIL_DIR%;%MSYS_DIR%;%BIN_DIR%;%OLD_PATH%"

:: 4. Execute Build Targets
IF "%TARGET%" == "debug" GOTO debug
IF "%TARGET%" == "release" GOTO release
IF "%TARGET%" == "clean" GOTO clean
echo Unknown target: %TARGET%
:: Restore original PATH on exit
SET "PATH=%OLD_PATH%"
exit /b 1

:debug
powershell write-host -back Yellow -fore Black Building debug...
make all DEBUG=1 %MAKE_ARGS%
GOTO end

:release
powershell write-host -back Green -fore White Building release...
make all %MAKE_ARGS%
GOTO end

:clean
powershell write-host -back Green -fore White Cleaning...
make clean %MAKE_ARGS%
GOTO end

:end
:: Restore original PATH on exit
SET "PATH=%OLD_PATH%"
