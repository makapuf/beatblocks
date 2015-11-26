NAME = beat_blk

GAME_C_FILES = main.c mod32.c
GAME_BINARY_FILES = dessin.tset dessin.tmap cursor.spr
USE_ENGINE = 1
USE_SDCARD = 1

include $(BITBOX)/lib/bitbox.mk

build/$(NAME).o : dessin.h 
main.c: dessin.h
	
dessin.tset dessin.tmap dessin.h: dessin.tmx 
	python $(BITBOX)/scripts/tmx.py $< > dessin.h

cursor.spr: cursor.png
	python $(BITBOX)/scripts/sprite_encode1.py $< $@

clean::
	rm -f dessin.tset dessin.tmap dessin.h cursor.h cursor.spr
