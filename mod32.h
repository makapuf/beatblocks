#pragma once 

/*
 Original code by  "Pascal Piazzalunga" - http://www.serveurperso.com
 Adapted for bitbox by makapuf - makapuf2@gmail.com
 adapted for Beat Blocks by makapuf 2014
 */


#include <stdbool.h>
#include <stdint.h>

// #define SYSCLK 80000000L
//#define SYSCLK 96000000L

#define SAMPLERATE BITBOX_SAMPLERATE     
#define BITDEPTH BITBOX_SAMPLE_BITDEPTH 
#define FATBUFFERSIZE 2048                        // File system buffers CHANNELS * 2048 = 8192 bytes of memory
#define DIVIDER 10                                // Fixed-point mantissa used for integer arithmetic
#define STEREOSEPARATION 32                       // 0 (max) to 64 (mono)

// Hz = 7093789 / (amigaPeriod * 2) for PAL
// Hz = 7159091 / (amigaPeriod * 2) for NTSC
#define AMIGA (7093789 / 2 / SAMPLERATE << DIVIDER)
// Mixer.channelFrequency[channel] = AMIGA / amigaPeriod

#define ROWS 64
#define SAMPLES 31
#define CHANNELS 8 // changed from 18 to 4 - to 6
#define NONOTE 0xFFFF


typedef struct {
 uint8_t name[22];
 uint16_t length;
 int8_t fineTune;
 uint8_t volume;
 uint16_t loopBegin;
 uint16_t loopLength;
} Sample;

struct mod{
 uint8_t name[20];
 Sample samples[SAMPLES];
 uint8_t songLength;
 uint8_t numberOfPatterns;
 uint8_t order[128];
 uint8_t numberOfChannels;
};

typedef struct {
 uint8_t sampleNumber[ROWS][CHANNELS];
 uint16_t note[ROWS][CHANNELS];
 uint8_t effectNumber[ROWS][CHANNELS];
 uint8_t effectParameter[ROWS][CHANNELS];
} Pattern;

struct player{
 Pattern currentPattern;

 uint8_t follow_song; //  follows song or keep playing same pattern?

 uint32_t amiga;
 uint16_t samplesPerTick;
 uint8_t speed;
 uint8_t tick;
 uint8_t row;
 uint8_t lastRow;

 uint8_t orderIndex; // index in the list of songs to play
 uint8_t oldOrderIndex;
 uint8_t patternDelay;
 uint8_t patternLoopCount[CHANNELS];
 uint8_t patternLoopRow[CHANNELS];

 uint8_t lastSampleNumber[CHANNELS];
 int8_t volume[CHANNELS];
 uint16_t lastNote[CHANNELS];
 uint16_t amigaPeriod[CHANNELS];
 int16_t lastAmigaPeriod[CHANNELS];

 uint16_t portamentoNote[CHANNELS];
 uint8_t portamentoSpeed[CHANNELS];

 uint8_t waveControl[CHANNELS];

 uint8_t vibratoSpeed[CHANNELS];
 uint8_t vibratoDepth[CHANNELS];
 int8_t vibratoPos[CHANNELS];

 uint8_t tremoloSpeed[CHANNELS];
 uint8_t tremoloDepth[CHANNELS];
 int8_t tremoloPos[CHANNELS];
};

struct mixer{
 uint32_t sampleBegin[SAMPLES];
 uint32_t sampleEnd[SAMPLES];
 uint32_t sampleloopBegin[SAMPLES];
 uint16_t sampleLoopLength[SAMPLES];
 uint32_t sampleLoopEnd[SAMPLES];

 uint8_t channelSampleNumber[CHANNELS];
 uint32_t channelSampleOffset[CHANNELS];
 uint16_t channelFrequency[CHANNELS];
 uint8_t channelVolume[CHANNELS];
 uint8_t channelPanning[CHANNELS];
};

struct fatBuffer{
 uint8_t channels[CHANNELS][FATBUFFERSIZE];
 uint32_t samplePointer[CHANNELS];
 uint8_t channelSampleNumber[CHANNELS];
};

/*
struct soundBuffer{
 uint16_t left[SOUNDBUFFERSIZE];
 uint16_t right[SOUNDBUFFERSIZE];
 uint16_t writePos;
 volatile uint16_t readPos;
};
extern struct soundBuffer SoundBuffer;

This is replaced ont he bitbox by the sound driver buffer.

*/

extern struct player Player;
extern struct mod Mod;
extern struct mixer Mixer;
extern struct fatBuffer FatBuffer;


//prototypes
void loadMod(void);
void player(void);
uint16_t mixer(uint16_t *buffer);
void loadPattern(uint8_t pattern);


// Effects
#define ARPEGGIO              0x0
#define PORTAMENTOUP          0x1
#define PORTAMENTODOWN        0x2
#define TONEPORTAMENTO        0x3
#define VIBRATO               0x4
#define PORTAMENTOVOLUMESLIDE 0x5
#define VIBRATOVOLUMESLIDE    0x6
#define TREMOLO               0x7
#define SETCHANNELPANNING     0x8
#define SETSAMPLEOFFSET       0x9
#define VOLUMESLIDE           0xA
#define JUMPTOORDER           0xB
#define SETVOLUME             0xC
#define BREAKPATTERNTOROW     0xD
#define SETSPEED              0xF

// 0xE subset
#define SETFILTER             0x0
#define FINEPORTAMENTOUP      0x1
#define FINEPORTAMENTODOWN    0x2
#define GLISSANDOCONTROL      0x3
#define SETVIBRATOWAVEFORM    0x4
#define SETFINETUNE           0x5
#define PATTERNLOOP           0x6
#define SETTREMOLOWAVEFORM    0x7
#define RETRIGGERNOTE         0x9
#define FINEVOLUMESLIDEUP     0xA
#define FINEVOLUMESLIDEDOWN   0xB
#define NOTECUT               0xC
#define NOTEDELAY             0xD
#define PATTERNDELAY          0xE
#define INVERTLOOP            0xF
