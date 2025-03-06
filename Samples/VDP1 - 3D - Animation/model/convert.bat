@echo off
set CONVERTER_PATH=D:\VisualStudio\ModelConverter\BuildDrop
set SAMPLE_DATA_PATH=D:\Saturn\homebrew\SaturnRingLib\Samples\VDP1 - 3D - Animation\cd\data

%CONVERTER_PATH%\ModelConverter.exe -i "bud_00.obj" "bud_01.obj" "bud_02.obj" "bud_03.obj" "bud_04.obj" "bud_05.obj" "bud_06.obj" "bud_07.obj" "bud_08.obj" "bud_09.obj" "bud_10.obj" "bud_11.obj" -o "%SAMPLE_DATA_PATH%\BUD.NYA" -t smooth
pause