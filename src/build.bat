@echo off


REM CLear the console so it's easier to see the errors
cls

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build


set commonCompilerFlags=-nologo -FC -Zi -Gm- -GR- -EHa- -Zo -Oi -Zi -Od
set commonCompilerFlags=-wd4577 %commonCompilerFlags%

set commonLinkLibs=Winmm.lib opengl32.lib shlwapi.lib Shell32.lib Kernel32.lib Ole32.lib

cl %commonCompilerFlags% /DNOMINMAX /Fe../bin/glypher.exe /I../libs /IE:/include ../src/main.cpp /link %commonLinkLibs% /SUBSYSTEM:WINDOWS

popd 