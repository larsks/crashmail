CC      = gcc -D__LINUX__ -I "../" --ansi -Wall

OBJS = osfile.o osdir.o osmisc.o osmem.o \
       ospattern.o os.o

all : oslib.a 

oslib.a : $(OBJS) 
	ar -ru oslib.a $(OBJS)
	cp oslib.a ../

# os

osfile.o : osfile.c
	$(CC) -ansi -c osfile.c -o osfile.o
 
osmisc.o : osmisc.c
	$(CC) -c osmisc.c -o osmisc.o

osdir.o : osdir.c
	$(CC) -c osdir.c -o osdir.o

osmem.o : osmem.c
	$(CC) -ansi -c osmem.c -o osmem.o

ospattern.o : ospattern.c
	$(CC) -ansi -c ospattern.c -o ospattern.o

os.o : os.c
	$(CC) -ansi -c os.c -o os.o


