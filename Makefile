CC = clang-3.6

CFLAGS = -I. -g `sdl-config --cflags --libs` -I/usr/include/SDL -std=gnu89 $(DEFINES)
DEFINES = -DLINUX -DINLINED_ACCESSORS -DSDL
LDFLAGS = `sdl-config --cflags --libs` -lSDLmain -lSDL -lz
OUTPUT = hugo

SOURCES = ./src/subs_eagle.c \
./src/ogglength.c \
./src/cd.c \
./src/osd_linux_sdl_music.c \
./src/h6280.c \
./src/bp.c \
./src/utils.c \
./src/hugo.c  \
./src/osd_linux_snd.c \
./src/osd_linux_cd_null.c \
./src/view_zp.c \
./src/osd_linux_sdl_machine.c \
./src/format.c \
./src/edit_ram.c \
./src/miniunz.c \
./src/debug.c \
./src/lsmp3.c \
./src/followop.c \
./src/dis.c \
./src/trans_fx.c \
./src/optable.c \
./src/view_inf.c \
./src/sound.c \
./src/osd_sdl_gfx.c \
./src/cheat.c \
./src/hard_pce.c \
./src/lang.c \
./src/mix.c \
./src/pcecd.c \
./src/hcd.c \
./src/iniconfig.c \
./src/sprite.c \
./src/bios.c \
./src/gfx.c \
./src/unzip.c \
./src/osd_keyboard.c \
./src/pce.c \
./src/list_rom.c 

OBJS = ${SOURCES:.c=.o}

${OUTPUT}:${OBJS}
	${CC} -o ${OUTPUT} ${SOURCES} ${CFLAGS} ${LDFLAGS} ${DEFINES} 
	
clean:
	rm src/*.o
	rm ${OUTPUT}
