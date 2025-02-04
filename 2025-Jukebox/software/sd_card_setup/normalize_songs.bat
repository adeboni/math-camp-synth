@echo off
mkdir "normalized-mp3"
for %%f in (songs\*.mp3) do (ffmpeg-normalize "%%f" -c:a libmp3lame -b:a 320k -o "normalized-mp3/%%~nf.mp3")
mkdir "wav"
for %%f in (normalized-mp3\*.mp3) do (ffmpeg -i "%%f" -ar 44100 -ac 2 "wav\%%~nf.wav")