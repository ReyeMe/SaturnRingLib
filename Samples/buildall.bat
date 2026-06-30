:; for d in */ ; do (cd "$d"/ ; echo "$d"; ./compile.bat release); done; exit;
@ECHO Off
set back=%cd%
for /d %%i in (.\*) do (
    cd "%%i"
    echo "%%i";
    compile.bat release
    if errorlevel 1 (
        echo Build failed in "%%i" with errorlevel %ERRORLEVEL%.
        cd ..
        exit /b %ERRORLEVEL%
    )
    cd ..
)
