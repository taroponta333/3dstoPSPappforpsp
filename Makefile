TARGET = 3ds_receiver

OBJS = main.o

CFLAGS = -O2 -G0 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBS = \
-lpspdebug \
-lpspdisplay \
-lpspctrl

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = 3DS File Receiver

PSPSDK := $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
