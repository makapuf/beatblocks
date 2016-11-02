NAME = beat_blk

GAME_C_FILES = main.c mod32.c lib/blitter/blitter.c lib/blitter/blitter_tmap.c lib/blitter/blitter_sprite.c 

GAME_BINARY_FILES = dessin.tset dessin.tmap cursor.spr
USE_SDCARD = 1

include $(BITBOX)/kernel/bitbox.mk

build/$(NAME).o : dessin.h 
main.c: dessin.h
	
dessin.tset dessin.tmap dessin.h: dessin.tmx 
	python $(BITBOX)/lib/blitter/scripts/tmx.py $< > dessin.h

cursor.spr: cursor.png
	python $(BITBOX)/lib/blitter/scripts/sprite_encode1.py $< $@

clean::
	rm -f dessin.tset dessin.tmap dessin.h cursor.h cursor.spr
