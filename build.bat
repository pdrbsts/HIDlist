@echo off
echo Building HIDlist.exe with MSVC...

REM Set up the environment for MSVC (you may need to adjust the path)
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat"

REM Compile the program
cl.exe /EHsc main.cpp /FeHIDlist.exe setupapi.lib cfgmgr32.lib

echo Build completed successfully!
pause
