@echo off
setlocal

for /f "delims=" %%F in ('dir "wav\*.wav" /b /o:n') do (
    for /f "tokens=* delims=" %%a in ("%%F") do echo %%a >> song_list.txt
)

endlocal