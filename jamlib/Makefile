include ../Makefile.inc

OBJS = crc32.o mbase.o message.o lastread.o subpack.o structrw.o

jamlib.a : $(OBJS) 
	$(AR) jamlib.a $(OBJS)

clean:
	$(RM) $(OBJS) jamlib.a
