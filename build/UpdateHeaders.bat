@ECHO OFF
PowerShell.exe -file "%~dp0build.ps1" -target=UpdateHeaders
PAUSE