@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64
mkdir pgo_build
cd pgo_build
ml64 /c /nologo /Zi /Fo"cnv2_main_loop.obj" /Fl"" /W3 /errorReport:prompt /Ta../crypto/asm/cnv2_main_loop.asm
cl /FA /GS- /GL /W1 /Zc:wchar_t /Zi /Gm- /O2 /Ob2 /Zc:inline /fp:precise /D "PGO_BUILD" /D "CONF_NO_HWLOC" /D "CONF_NO_HTTPD" /D "CONF_NO_TLS" /WX- /Zc:forScope /Gd /Oi /MD /FC /EHsc /nologo /Ot ../*.cpp ../crypto/*.cpp ../crypto/*.c /link /LTCG /GENPROFILE /DEBUG:FULL /incremental:no /machine:X64 /nologo /subsystem:console cnv2_main_loop.obj advapi32.lib ws2_32.lib /OUT:"xmr-stak-cpu-pgo-optimized.exe"
move xmr-stak-cpu-pgo-optimized.exe xmr-stak-cpu-pgo-instrument.exe
cd ..
pgo_build\xmr-stak-cpu-pgo-instrument.exe /instrument
cd pgo_build
del xmr-stak-cpu-pgo-instrument.exe
link /LTCG /USEPROFILE:AGGRESSIVE /DEBUG:FULL /incremental:no /machine:X64 /nologo /subsystem:console *.obj advapi32.lib  ws2_32.lib /OUT:"xmr-stak-cpu-pgo-optimized.exe"
