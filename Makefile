ifdef PS5_PAYLOAD_SDK
include $(PS5_PAYLOAD_SDK)/toolchain/prospero.mk
else
$(error PS5_PAYLOAD_SDK is undefined)
endif

CFLAGS := -O3 `$(PS5_SYSROOT)/bin/sdl2-config --cflags`
LIBS := `$(PS5_SYSROOT)/bin/sdl2-config --libs`
LDLIBS := -lkernel_sys -lSDL2main -lSDL2_ttf -lfreetype -lz -lbz2 -lpng -lSDL2_image -lwebp -lwebpmux -lwebpdemux

SRCS := main.c
OBJS := $(SRCS:.c=.o)

sprites.elf: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS) $(LDLIBS)

clean:
	rm -f sprites.elf $(OBJS)