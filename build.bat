@ECHO OFF
SETLOCAL

SET "config_option="
SET "target_option="

IF NOT [%~1]==[] SET config=%1
IF NOT [%~2]==[] SET target=%2
IF NOT [%config%]==[] SET config_option=--config %config%
IF NOT [%target%]==[] SET target_option=--target %target%

cmake --build build %config_option% %target_option%
ENDLOCAL
