# type either "make linux" or "make win32" to compile

linux :
	make -C cmnllib -f Makefile.linux
	make -C jamlib -f Makefile.linux
	make -C oslib_linux 
	make -f Makefile.linux
	strip ../bin/*
	
win32 :
	make -C cmnllib -f Makefile.win32
	make -C jamlib -f Makefile.win32
	make -C oslib_win32 
	make -f Makefile.win32
	strip ..\\bin\\*
	
cleanlinux : 
	rm -f cmnllib/*.a
	rm -f cmnllib/*.o
	rm -f jamlib/*.a
	rm -f jamlib/*.o
	rm -f oslib_linux/*.o
	rm -f oslib_linux/*.a 
	rm -f oslib_win32/*.o
	rm -f oslib_win32/*.a 
	rm -f obj/*.o
	rm -f *.a
	
cleanwin32 : 
	del cmnllib\*.a
	del cmnllib\*.o
	del jamlib\*.a
	del jamlib\*.o
	del oslib_linux\*.o
	del oslib_linux\*.a 
	del oslib_win32\*.o
	del oslib_win32\*.a 
	del obj\*.o
	del *.a
