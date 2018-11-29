
ProcessMonitorps.dll: dlldata.obj ProcessMonitor_p.obj ProcessMonitor_i.obj
	link /dll /out:ProcessMonitorps.dll /def:ProcessMonitorps.def /entry:DllMain dlldata.obj ProcessMonitor_p.obj ProcessMonitor_i.obj \
		kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib \

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL \
		$<

clean:
	@del ProcessMonitorps.dll
	@del ProcessMonitorps.lib
	@del ProcessMonitorps.exp
	@del dlldata.obj
	@del ProcessMonitor_p.obj
	@del ProcessMonitor_i.obj
