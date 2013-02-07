include ../Makefile.inc

OBJS = osfile.o osdir.o osmisc.o osmem.o ospattern.o os.o

oslib.a : $(OBJS) 
	$(AR)	oslib.a $(OBJS)

clean :
	$(RM) $(OBJS) oslib.a

