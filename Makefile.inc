AR = ar -ru
RM = rm -f
STRIP = strip
INSTALL = install

CPPFLAGS = -DPLATFORM_LINUX -D__LINUX__ -I$(PWD) $(OPTCPPFLAGS)
CFLAGS =

