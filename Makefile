TARGET = 3ds_receiver

OBJS = \
main.o \
dialog.o \
network.o

CFLAGS = -O2 -G0 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBS = \
-lpspdebug \
-lpspdisplay \
-lpspctrl \
-lpspnet \
-lpspnet_inet \
-lpspnet_apctl \
-lpspnet_resolver \
-lpsputility \
-lpspgu \
-lpspgum \
-lpspge \
-lpspvfpu \
-lpspkernel

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = 3DS File Receiver

PSPSDK := $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
