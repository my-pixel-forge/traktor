@call %~dp0config.bat
@%TRAKTOR_HOME%\bin\win64\solutionbuilder -f=msvc -p=$(TRAKTOR_HOME)\bin\msvc-2015-android.xml %TRAKTOR_HOME%\TraktorAndroid.xms

