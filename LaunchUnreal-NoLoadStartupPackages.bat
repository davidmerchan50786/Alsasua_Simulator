@echo off
setlocal
set UE_EXE="C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
set PROJECT="F:\Epic Games\UE_5.7\altsasu_gtavii\UnrealProject\AlsasuaSimulator.uproject"
start "Unreal Editor" %UE_EXE% %PROJECT% -NoLoadStartupPackages -game
endlocal
