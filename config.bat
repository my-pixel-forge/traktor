@echo off

set TRAKTOR_HOME=%~dp0

set PATH=%PATH%;%TRAKTOR_HOME%/build/win32/releaseshared
set PATH=%PATH%;%TRAKTOR_HOME%/build/win32/debugshared

set PATH=%PATH%;"%TRAKTOR_HOME%/3rdp/POWERVR SDK/OGLES2_WINDOWS_PCEMULATION_2.07.27.0484/Builds/OGLES2/WindowsPC/Lib"
set PATH=%PATH%;%SCE_PS3_ROOT%/host-win32/Cg/bin

