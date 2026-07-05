TARGET = 3ds_receiver
OBJS = main.o

CFLAGS = -O2 -G0 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBS = \
-lpspdebug \
-lpspvfpu \
-lpspdisplay \
-lpspgu \
-lpspgum \
-lpspge \
-lpspnet_apctl \
-lpspnet_resolver \
-lpspnet_inet \
-lpspnet \
-lpsputility

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = 3DS File Receiver

PSPSDK := $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
