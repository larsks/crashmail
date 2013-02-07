AR = ar -ru
RM = rm -f
STRIP = strip

CPPFLAGS = -DPLATFORM_LINUX -D__LINUX__ -I$(PWD) $(OPTCPPFLAGS)
CFLAGS = -Wno-pointer-sign

