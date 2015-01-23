// set volume 0/1/2/3 (cycle si dessus)
// undo
// a/b toggle, LR : pattern, select : drumkit, x then up : copy, x+down paste, y undo (1 undo = 64bytes, garder 16 prec., start  : pause/play)
// A/B : set,  X : other
// paste empty par defaut
// select : drumkit, 

// change print help (si X)
// dessins instrc + faire 707, real, 808
// anim au debut des boutons 

// What if no mod is loaded ! blink 00 

#include <strings.h>
#include <string.h>

#include <bitbox.h>
#include <blitter.h>

#include <fatfs/ff.h>
#include <fatfs/diskio.h>

#include "dessin.h"

extern const unsigned char cursor_spr[];

#include "mod32.h"

#define MOD_PATH "DRUMKITS"

#define C4NOTE 208 //104/2 // base note used
#define DELTA_BPM 5 // BPM increment

struct Sampler {

    // player, mod, mixer : see mod32.h  // incl current pattern data, current pattern, current step
    uint8_t sample_map [8]; // instrument on line X : ie val[line] = sample X and X+1

    uint8_t dirty_pattern; // must be saved   
    uint8_t in_help; // help is displayed
   
    object *cursor; // sprite glow, anime
    object *bg; // init with vram 64x64

    int cursor_x, cursor_y; // position on the grid, 0-Nb channels + 1 for options
    int drumkit;
};

FATFS fso;        // The FATFS structure (file system object) holds dynamic work area of individual logical drives
DIR dir;          // The DIR structure is used for the work area to read a directory by f_oepndir, f_readdir function
FILINFO fileInfo; // The FILINFO structure holds a file information returned by f_stat and f_readdir function
FIL file;         // The FIL structure (file object) holds state of an open file

Pattern clipboard;
// XXX undo !

/* -- globals --*/
int sample_in_tick;
struct Sampler sampler;
uint8_t vram[64*64];

void loadNextFile() {
	int res;
	char *dotPointer;

	do {
		res=f_readdir(&dir, &fileInfo);
		if (res) {
			die(4,res);
		}
		if(fileInfo.fname[0] == 0) break;
		dotPointer = strrchr(fileInfo.fname, '.');
	} while(dotPointer == NULL || strcasecmp(dotPointer, ".mod"));

	f_chdir (MOD_PATH);
	if(fileInfo.fname[0] != 0) {
		f_open(&file, fileInfo.fname, FA_READ);
		message("Opened File: %s ",fileInfo.fname);
		loadMod();
		message("Song name: [%s]\n",Mod.name);
	}
}


const int channel_y[8] = {6,8,10,12,15,17,19,21}; // Y position of each channel
// mini tilemaps
const uint8_t pad_tmap[][4] = { {76,77,83,84},{78,79,85,86} };
 
//                             0   1   2   3   4   5   6   7   8   9
const uint8_t numbers_hi[] = {123,124,125,125,126,127,127,125,123,123};
const uint8_t numbers_lo[] = {135,136,137,138,139,140,141,136,141,140};
const uint8_t cursor_tile[2] = {73,74};

const int drumkit_pos[2]={2,25}; // screen position of drumkit number
const int pattern_pos[2]={5,25}; // screen position of pattern number
const int tempo_pos[2]  ={9,25}; // screen position of tempo number
const int cursor_line = 5;


void zap_pattern(Pattern *p)
{
	memset(Player.currentPattern.sampleNumber,0,ROWS*CHANNELS);
	memset(Player.currentPattern.note,0xff,2*ROWS*CHANNELS); // NONOTE = 0xff 0xff 
}

void print_num(const int pos[2], int num, const int digits)
{
	for (int i=digits-1; i>=0;i--)
	{
		vram[ pos[1]   *64+pos[0]+i] = numbers_hi[num%10]+1;
		vram[(pos[1]+1)*64+pos[0]+i] = numbers_lo[num%10]+1;
		num /=10;
	}
}

// translate a beat to position on screen (in tiles)
inline int step2x(int step)
{
	if (step==16) step=15; //  can happen momentarily 
	return 3 + step*2 + step/4;
}

inline int chan2y(int chan)
{
	return channel_y[chan];
}

void handle_display()
{

	const uint32_t pad_header = TMAP_HEADER(2,2,TSET_16,TMAP_U8);

	tmap_blit(sampler.bg,0,0,dessin_header, dessin_tmap);
    // display line headers with instruments IDs. 

    // display blocks according to the current pattern samples (dont care about notes or samples)
    for (int chn=0;chn<CHANNELS;chn++) 
    	for (int step=0;step<16;step++)
    	{
    		uint8_t s = Player.currentPattern.sampleNumber[step*4][chn];
    		// 0 or 1 ! inverse channel mapping (if possible, else just use 0).
    		if (s) {    			
    			tmap_blit(sampler.bg,step2x(step), chan2y(chn), pad_header, pad_tmap[s==chn+1 ? 0 : 1]);
    		}   		
    	}
    
    print_num(drumkit_pos,sampler.drumkit,2);
    print_num(pattern_pos,Player.orderIndex,2);

    int bpm =  5* SAMPLERATE  / Player.samplesPerTick /2;
    print_num(tempo_pos,bpm,3);
    	
    // display player beat position 
    vram[cursor_line*64 + step2x(Player.row/4)] = cursor_tile[0]+1;
    vram[cursor_line*64 + step2x(Player.row/4) + 1 ] = cursor_tile[1]+1;


	// position & display blinking cursor
	sampler.cursor->x= step2x(sampler.cursor_x)*16;
	sampler.cursor->y=(vga_frame & 32) ? chan2y(sampler.cursor_y)*16: 500;
}


void changePattern()
{
	
	// XXX SAVE, undo (?), wait end of patten
	loadPattern(Player.orderIndex);
	zap_pattern(&clipboard); // NO, just loop
}

void handle_input(void)
{
	#ifndef NO_USB

	static uint16_t prev_buttons;

	int b = gamepad_buttons[0] & ~prev_buttons; // new buttons

	if (gamepad_buttons[0] & gamepad_X) { // control - key ? (press X before)
		if ( b & gamepad_up )   memcpy(&clipboard,&Player.currentPattern, sizeof(Pattern)); // copy
		if ( b & gamepad_down ) memcpy(&Player.currentPattern,&clipboard,sizeof(Pattern)); // paste
		if ( b & gamepad_A ) zap_pattern(&Player.currentPattern); // zap

		if ( b & gamepad_R && Player.orderIndex < Mod.songLength) {
			// next pattern
			Player.orderIndex+=1;
			changePattern();
		}

		if ( b & gamepad_L && Player.orderIndex>0 ) {
			// prev pattern
			Player.orderIndex-=1;
			changePattern();
		}
	} else {
		if ( b & gamepad_up ) sampler.cursor_y -=1;
		if ( b & gamepad_down ) sampler.cursor_y +=1;
		sampler.cursor_y &= 0x7; // 8 channels

		if ( b & gamepad_left) sampler.cursor_x -=1;
		if ( b & gamepad_right) sampler.cursor_x +=1;
		sampler.cursor_x &= 0xf;

		int bpm = 5* SAMPLERATE  / Player.samplesPerTick /2;
		if ( b & gamepad_L) bpm -= DELTA_BPM;
		if ( b & gamepad_R) bpm += DELTA_BPM;
		Player.samplesPerTick = SAMPLERATE / (2 * bpm / 5);

		// shorter to type
		uint8_t *s = &Player.currentPattern.sampleNumber[sampler.cursor_x*4][sampler.cursor_y];
		uint16_t *n = &Player.currentPattern.note[sampler.cursor_x*4][sampler.cursor_y];

		// toggle pad to A if in B or 0 ; set to zero if already set
		if (b & gamepad_A ) {
			if (*s == sampler.cursor_y+1) {
				*s = 0;
				*n = NONOTE;
			} else {
				*s = sampler.cursor_y+1;
				*n = C4NOTE; 
			}
		}

		// toggle pad to B if in A or 0 ; set to zero if already set
		if (b & gamepad_B) {
			if (*s == sampler.cursor_y+1+8) {
				*s = 0;
				*n = NONOTE;
			} else {
				*s = sampler.cursor_y+1+8;
				*n = C4NOTE; 
			}
		}
	}
	prev_buttons = gamepad_buttons[0];
	#endif
}




void game_init()
{
	// Graphics : one bg + one cursor (simple)
	blitter_init();
	sampler.bg = tilemap_new (dessin_tset, 640, 480, TMAP_HEADER(64,64,TSET_16, TMAP_U8), vram);
	sampler.cursor = sprite_new((uint32_t *)cursor_spr,5,5,-1);
	sampler.cursor_x = 5;
	sampler.cursor_y = 5;

	// Filesystem : load all drumkits to table.

	// XXX  better check those errors !
	int res;
	
	f_mount(&fso,"",1); //mount now
	
	res = f_opendir(&dir, MOD_PATH);
	if (res != FR_OK) {
		die(3,res);
	}
	

	// MOD
	loadNextFile();
	loadPattern(0); 
	zap_pattern(&clipboard);

	// disable auto next pattern (or taken care by following)
	Player.follow_song = 0;

	// inject a loop to start of current pattern ? 
}


void game_frame()
{
	handle_input();
	handle_display();
}


void game_snd_buffer(uint16_t *stream, int size)
{
	for (int i=0;i<size; i++) {
		if (sample_in_tick++>=Player.samplesPerTick) {
			// check button for next song ...
			if (0){
				loadNextFile();
			} else {
				player();
			}
			sample_in_tick=0;
		}
		stream[i] = mixer(stream); 
	}
} 