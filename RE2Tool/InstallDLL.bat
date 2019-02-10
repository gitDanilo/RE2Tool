@echo off
set RE2DIR=E:\Steam\steamapps\common\RESIDENT EVIL 2  BIOHAZARD RE2
set DLLDIR=C:\Users\danil.000\source\repos\RE2Tool\x64\Release
del /Q "%RE2DIR%\dinput8.dll"
copy /Y "%DLLDIR%\RE2Tool.dll" "%RE2DIR%\dinput8.dll"