TARGET = 3ds_receiver
OBJS = main.o

CFLAGS = -O2 -G0 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

# ネットワーク機能を使うために必要なライブラリを全部リンクする設定
LIBS = -lpspnet_apctl -lpspnet_resolver -lpspnet_inet -lpspnet -lpsputility

# 🔥これが超重要！最終成果物として EBOOT.PBP を作れという命令
EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = 3DS File Receiver

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.make
