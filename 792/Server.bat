@echo off 
title OTserv Auto-restarter 
echo OTserv Auto-Restarter
:begin 
TheForgottenServer-debug.exe 
echo Server restarting
goto begin 
:goto begin
