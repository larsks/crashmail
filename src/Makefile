# type either "make linux" or "make win32" to compile

help:
	@echo You can use this Makefile in the following ways:
	@echo make linux ............ Make Linux binaries
	@echo make win32 ............ Make Win32 binaries
	@echo make os2 .............. Make OS/2 binaries
	@echo make os2nmake ......... Make OS/2 binaries (IBM nmake)
	@echo make cleanlinux ....... Remove object files under Linux
	@echo make cleanwin32 ....... Remove object files under Win32
	@echo make cleanos2 ......... Remove object files under OS/2
	@echo make cleanos2nmake .... Remove object files under OS/2 (IBM nmake)
		
linux :
	make -C cmnllib -f Makefile.linux
	make -C jamlib -f Makefile.linux
	make -C oslib_linux 
	make -f Makefile.linux
	
win32 :
	make -C cmnllib -f Makefile.win32
	make -C jamlib -f Makefile.win32
	make -C oslib_win32 
	make -f Makefile.win32

os2 :
	make -C cmnllib -f Makefile.os2
	make -C jamlib -f Makefile.os2
	make -C oslib_os2 
	make -f Makefile.os2

os2nmake :
	cd cmnllib
	nmake /f Makefile.os2
	cd ../jamlib
	nmake /f Makefile.os2
	cd ../oslib_os2
	nmake
	cd ..
	nmake /f Makefile.os2
	
cleanlinux : 
	make -C cmnllib -f Makefile.linux clean
	make -C jamlib -f Makefile.linux clean
	make -C oslib_linux clean
	make -f Makefile.linux clean
	
cleanwin32 : 
	make -C cmnllib -f Makefile.win32 clean
	make -C jamlib -f Makefile.win32 clean
	make -C oslib_win32 clean
	make -f Makefile.win32 clean

cleanos2 : 
	make -C cmnllib -f Makefile.os2 clean
	make -C jamlib -f Makefile.os2 clean
	make -C oslib_os2 clean
	make -f Makefile.os2 clean

cleanos2nmake : 
	cd cmnllib
	nmake /f Makefile.os2 clean
	cd ../jamlib
	nmake /f Makefile.os2 clean
	cd ../oslib_os2
	nmake clean
	cd ..
	nmake /f Makefile.os2 clean
