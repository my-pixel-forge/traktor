@echo off

call %~dp0config.bat
call %~dp0config-vsenv.bat build\win64\version.txt

set AGGREGATE_OUTPUT_PATH=%TRAKTOR_HOME%\bin\latest\win64

start build\win64\"Traktor Win64.sln"
