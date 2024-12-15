:;
    for d in */ ; do
        (cd "$d"/ ; ./compile.bat release)
    done
    exit;
@ECHO Off
set back=%cd%
for /d %%i in (.\*) do (
    cd "%%i"
    ./compile.bat release
    cd ..
)
