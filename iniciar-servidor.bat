@echo off
title Servidor Local - Credencial Inteligente ITTLA
cls
echo Iniciando servidor local para abrir en celular...
powershell -ExecutionPolicy Bypass -NoProfile -File "%~dp0servidor-local.ps1"
pause
