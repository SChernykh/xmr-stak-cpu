@echo off
call "%VS140COMNTOOLS%\..\..\VC\vcvarsall.bat" amd64
mkdir pgo_build
cd pgo_build
cl /FA /GS- /GL /W1 /Zc:wchar_t /Zi /Gm- /O2 /Ob2 /Zc:inline /fp:precise /D "PGO_BUILD" /D "CONF_NO_HWLOC" /D "CONF_NO_HTTPD" /D "CONF_NO_TLS" /WX- /Zc:forScope /Gd /Oi /MD /FC /EHsc /nologo /Ot ../*.cpp ../crypto/*.cpp ../crypto/*.c /link /LTCG /GENPROFILE /DEBUG:FULL /incremental:no /machine:X64 /nologo /subsystem:console advapi32.lib /OUT:"xmr-stak-cpu-pgo-optimized.exe"
move xmr-stak-cpu-pgo-optimized.exe xmr-stak-cpu-pgo-instrument.exe
cd ..
pgo_build\xmr-stak-cpu-pgo-instrument.exe /instrument
cd pgo_build
del xmr-stak-cpu-pgo-instrument.exe
link /LTCG /USEPROFILE:AGGRESSIVE /DEBUG:FULL /incremental:no /machine:X64 /nologo /subsystem:console *.obj advapi32.lib /OUT:"xmr-stak-cpu-pgo-optimized.exe"
