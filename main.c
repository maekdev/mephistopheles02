// main.c
//
// 210604 (beta-release)
// markus ekler
//
// OPEN BUG:
//  - still somewhat unresolved bug about the sync_timer causing freeze with wrong setting


// INCLUDES
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <SDL.h>
#include <SDL_image.h>

#include "65c02.h"

// DEFINES
#define SILENT
#define DAC
//#define DUMPRAM
//#define TESTFUNCTION

#define SCREEN_WIDTH 1250/2 // png size 1000x1250
#define SCREEN_HEIGHT 1250/2
#define COLOR_CURSOR (SDL_MapRGB(gScreenBuffer->format,255,0,0))
#define COLOR_LEDON (SDL_MapRGB(gScreenBuffer->format,255,0,0))
#define COLOR_LEDOFF (SDL_MapRGB(gScreenBuffer->format,0,0,0))

#define NMI_ClearLine()	(nmi=1)
#define NMI_SetLine() (nmi=0)


// GLOBALS
BOOL irq = 1;
BOOL nmi = 1;
BOOL stp = 0;
BOOL wai = 0;
WORD iopage = 0x8000;

// ticks for audio handling
uint32_t ticks = 0;

uint8_t m_input_mux = 0;
uint8_t m_mux = 0;
uint8_t m_dac = 0;

SDL_Window *gWindow = NULL;
SDL_Surface *gScreenSurface = NULL;
SDL_Surface *gScreenBuffer = NULL;
SDL_Surface *gImgBoard = NULL;
SDL_Surface *gImgFigure[14];

SDL_AudioDeviceID dev;

int cursor = 16;
int cursor_sel = -1;

#define DACFIFOLEN 22000
uint8_t dacfifo[DACFIFOLEN+2]; // 1s dac info
uint32_t last_ticks=0;
uint32_t dacw=0;
uint32_t dacr=0;

// PROTOTYPES
int SaveGame(char *fn); // 65c02mem.c
int LoadGame(char *fn); // 65c02mem.c
#ifdef DUMPRAM
#warning DEBUGGING OPTION DUMPRAM with Fx activated
int DumpRAM(char *fn);
#endif // DUMPRAM

// FUNCTIONS
void DAC_init(void) {
	int i;
	for (i=0;i!=DACFIFOLEN;i++) 
		dacfifo[i] = 0;
	dacw=0;
	dacr=0;
}

uint32_t DAC_len(void) {
	return dacw-dacr;
}

void DAC_push(uint8_t v) {
	dacfifo[dacw] = v;
	if (dacw == DACFIFOLEN) 
		dacw=0;
	else 
		dacw++;
}

uint8_t DAC_pop(void) {
	uint8_t ret;
	ret = dacfifo[dacr];
	if (dacr == DACFIFOLEN) {
		dacr = 0;
	} else {
		dacr++;
	}
	return ret;
}

void DAC_trigger(uint8_t v) {
#ifdef DAC
	uint32_t i;
	uint32_t delta = ticks-last_ticks;
	last_ticks = ticks;
	if (delta <= 200000) { // 0.1s as threshold
		for (i=0;i!=delta/91;i++) {
#ifdef SILENT
			if (v) DAC_push(16);
			//if (v) DAC_push(0);
#else
			if (v) DAC_push(255);
#endif
			else DAC_push(0);
			//DAC_push(v);
		}
	}
#endif // DAC
}

uint8_t read6502(uint16_t address) {
	int offset = address-0x3000;
	int i=0;
	if (address >= 0x3000 && address <= 0x3007) {
		// make it short in case nothing pressed
		if (cursor_sel < 0) 
			return 0x80;
		if (m_input_mux & 0x01) {
			if (cursor_sel-128-8 == offset) {
				return 0x00;
			}
			return 0x80;
		} else if (m_input_mux & 0x02) {
			// offset = 3 => "8"
			// offset = 1 => "6"
			if (cursor_sel-128-8-4 == offset) {
				return 0x00;
			}
			return 0x80;
		} else if (m_input_mux & 0x04) {
			// offset = 0 => "PLAY"
			if (cursor_sel-128 == offset) {
				return 0x00;
			}
			return 0x80;
		} else if (m_input_mux & 0x08) {
			// offset = 1 => "LEV"
			if (cursor_sel-128-4 == offset) {
				return 0x00;
			}
			return 0x80;
		}
		// Board input (offset,m_mux)
		// [LEDL - orientation: 0xFE=1,0xFD=2]
		// rook(wh) @ 3007,0xFE => LEDM(7),LEDL(0)
		// rook(wh) @ 3000,0xFE => LEDM(0),LEDL(0)
		// pawn(wh) @ 3000,0xFD => LEDM(0),LEDL(0)
		i=0;
		switch (m_mux) {
			//case 0xFE: i = 0; break;
			case 0xFD: i = 8; break;
			case 0xFB: i = 16; break;
			case 0xF7: i = 24; break;
			case 0xEF: i = 32; break;
			case 0xDF: i = 40; break;
			case 0xBF: i = 48; break;
			case 0x7F: i = 56; break;
		}
		if (i+offset == cursor_sel) {
			return 0x00;
		}
		return 0x80;
	}
	// illegal access - should be never reached
	return 0x00;
}

void write6502(uint16_t address, uint8_t value) {
	if (address == 0x2000) {
		/*for (i=0;i!=8;i++) {
		//if ((leds_data & (1<<i)) == 0x00) {
		if ((m_mux & (1<<i)) == 0x00) {
			if (value & 0x10) {
				// ledh
				// PLAY(0),ERR(4),WH(7)
				LED_set(i);
				//m_led[i] = 1;
			}
			if (value & 0x20) {
				// ledm
				// BoardX - orientation tbd
				LED_set(i+8);
				//m_led[i+8] = 1;
			}
			if (value & 0x40) {
				// ledl
				// BoardY - 0xFE=1,0xFD=2
				LED_set(i+16);
				//m_led[i+16] = 1;
			}
		}
		}
		*/
		m_input_mux = value ^ 0xFF;
		if (m_dac != (value & 0x80)) {
			DAC_trigger(value&0x80);
		}
		m_dac = value & 0x80;
		#warning check NMI_ClearLine() impact
		//NMI_ClearLine();
		return;

	} else if (address == 0x2800) {
		m_mux = value;
		return;
	}
	printf("ERR: Illegal write memory access @%04x = %02x\n",address,value);
}

void MyAudioCallback(void* userdata,uint8_t* stream,int len) {
	int i;
	for (i=0;i!=len;i++) {
		//*(stream+i) = (uint8_t)i;
		//*(stream+i) = (uint8_t)0x00;
		if (DAC_len()) 
			*(stream+i) = DAC_pop();
		else
			*(stream+i) = 0;
	}
}

int SND_Init(void) {
	SDL_AudioSpec want, have;

        SDL_memset(&want, 0, sizeof(want)); /* or SDL_zero(want) */
        want.freq = 22000;
        //want.format = AUDIO_F32;
        //want.format = AUDIO_S16; 
        want.format = AUDIO_U8; 
        want.channels = 1;
        want.samples = 4096;
        want.callback = MyAudioCallback; /* you wrote this function elsewhere -- see SDL_AudioSpec for details */

        SDL_Init(SDL_INIT_AUDIO);

        dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
        if (dev == 0) {
		SDL_Log("ERR: Failed to open audio: %s", SDL_GetError());
		return 0;
        } else {
		if (have.format != want.format) { 
			SDL_Log("We didn't get Float32 audio format.");
			return 0;
		}
		// This command starts audio playback
		SDL_PauseAudioDevice(dev, 0);
	}
	return 1;
}

void SND_Destroy(void) {
	SDL_CloseAudioDevice(dev);
}

SDL_Surface* GFX_LoadImage(SDL_Surface *src,char *fn) {
	SDL_Surface *ret = NULL;
	SDL_Surface *tmp = NULL;

	tmp = IMG_Load(fn);
	if (tmp == NULL) {
		printf("ERR: GFX_LoadImage() - IMG_Load()\n");
	} else {
		ret = SDL_ConvertSurface(tmp,src->format,0);
		if (ret == NULL) {
			printf("ERR: GFX_LoadImage() - SDL_ConvertSurface()\n");
		}
		SDL_FreeSurface(tmp);
	}
	return ret;
}

int GFX_Init(void) {
	char *fn[] = { \
		"chess/wp.png", \
		"chess/wn.png", \
		"chess/wb.png", \
		"chess/wr.png", \
		"chess/wp.png", \
		"chess/wq.png", \
		"chess/wk.png", \
		"chess/bp.png", \
		"chess/bn.png", \
		"chess/bb.png", \
		"chess/br.png", \
		"chess/bp.png", \
		"chess/bq.png", \
		"chess/bk.png", \
		NULL };

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		printf("ERR: SDL_Init() - %s\n",SDL_GetError());
		return 0;
	} else {

		gWindow = SDL_CreateWindow("MEphisto Episode II - Lucifer s Life",SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,SCREEN_WIDTH,SCREEN_HEIGHT,SDL_WINDOW_SHOWN);

		if (gWindow == NULL) {
			printf("ERR: SDL_CreateWindow() - %s\n",SDL_GetError());
			return 0;
		} else {
			gScreenSurface = SDL_GetWindowSurface(gWindow);
			// seems to work all lets get sdl_image as well
			if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
				printf("ERR: IMG_Init(): %s\n",IMG_GetError());
			}

			// we start with loading board and figures as we need board image size
			gImgBoard = GFX_LoadImage(gScreenSurface,"mondial2/mondial2.png");
			
			gScreenBuffer = SDL_CreateRGBSurface(0,gImgBoard->w,gImgBoard->h,32,0xff,0xff00,0xff0000,0xff000000);
			for (int i=0;i!=14;i++) {
				gImgFigure[i] = GFX_LoadImage(gScreenBuffer,fn[i]);
			}

		}
	}

	return 1;
}

int GFX_Destroy(void) {
	SDL_FreeSurface(gScreenBuffer);
	SDL_FreeSurface(gImgBoard);
	for (int i=0;i!=12;i++) {
		SDL_FreeSurface(gImgFigure[i]);
	}
	SDL_DestroyWindow(gWindow);
	gWindow = 0;

	SDL_Quit();

	return 1;
}


int GFX_Update(void) {
	// only bllitscale buffer to screen and update
	#warning implement clever adaption to screen size here
	
	// for simplicity use full window now...
	//SDL_BlitSurface(gScreenBuffer,NULL,gScreenSurface,NULL);
	SDL_BlitScaled(gScreenBuffer,NULL,gScreenSurface,NULL);
	
	SDL_UpdateWindowSurface(gWindow);
	return 1;
}

int LED_get(int i,int state) {
	uint8_t offset = state % 2;
	if (i >= 0 && i <= 7) {
		if (mem[4+(offset)] & (1<<i)) 
			return 1;
	} else if (i >= 8 && i <= 15) {
		if (mem[0+(offset)] & (1<<(i-8)))
			return 1;
	} else if (i >= 15 && i <= 23) {
		if (mem[2+(offset)] & (1<<(i-16)) )
			return 1;
	}
	return 0;
}

int GFX_DrawLEDs(int state) {
	// x-LED @ mem 0x0000
	// y-LED @ mem 0x0002
	// bottom-LED @ mem 0x004
	// swapping on 500ms timer to address +1 -- based on state variable: if odd state then mem(1,3,5) else mem(0,2,4) for led display

	SDL_Rect r;
	const int ledx[] = { 83,190,294,399,504,608,713,818, \
				118,229,333,440,546,652,756,864, \
				48,48,48,48,48,48,48,48 };
	const int ledy[] = { 1043,1043,1043,1043,1043,1043,1043,1043, \
				942,942,942,942,942,942,942,942,\
				867,764,655,552,447,339,233,126 };
	for (int i=0;i!=24;i++) {
		r.x = ledx[i];
		r.y = ledy[i];
		if (i < 16) {
			r.w = 16;
			r.h = 8;
		} else {
			r.h = 16;
			r.w = 8;
		}
		if (LED_get(i,state))
			SDL_FillRect(gScreenBuffer,&r,COLOR_LEDON);
		else 
			SDL_FillRect(gScreenBuffer,&r,COLOR_LEDOFF);
			
	}
	return 1;
}

int GFX_DrawBoard(void) {
	int i,o,tmp;
	const int boardx[] = { 78,185,289,394,499,603,708,813 };
	//const int boardy[] = { 76,182,288,394,499,605,706,812,1073,1152 };
	// reversed
	const int boardy[] = { 1152,1073,812,706,605,499,394,288,182,76 };
	//const uint16_t baddr[] = { 0x215,0x21F,0x229,0x233,0x23D,0x247,0x251,0x25B };
	SDL_Rect r;

	// update background image
	SDL_BlitSurface(gImgBoard,NULL,gScreenBuffer,NULL);

	// draw cursor
	if (cursor >= 16) { // chess board
		r.x = boardx[cursor%8];
		r.y = boardy[cursor/8];
		r.w = 105;
		r.h = r.w;
	} else {
		r.x = boardx[cursor%8]+10;
		r.y = boardy[cursor/8];
		r.w = 85;
		r.h = 55;
	}

	SDL_FillRect(gScreenBuffer,&r,COLOR_CURSOR);

	// Draw the figures
	// 0x215,21F,229,233,23D,247,251,25B following each 8bytes
	// B=0x10,T=0x13,S=0x12,L=0x11,K/D?=0x15/16 (wh |= 0x10, bl |= 0x20)

	/* fixed beta version */
	const uint8_t baddr[] = { 0x15,0x1F,0x29,0x33,0x3D,0x47,0x51,0x5B };
	uint8_t board[256];

	// initial setup of board, based on following considerations:
	//  - 0x1a0+x contains piece value
	//  - 0x330+x contains piece position
	memset(board,0x00,128);
	// more clever initialization based on
	//  - piece value on 0x1a0[32]
	//  - piece position on 0x2e0[32]
	for (i=0;i!=32;i++) {
		board[(uint8_t)mem[0x2e0+i]] = (uint8_t)mem[0x1a0+i];
	}
	// calculation of current board based on following considerations:
	//  - 0x6c contains number of moves
	//  - 0x4b0+x contains start field address (history of moves)
	//  - 0x570+x contains destination field address (history of moves)
	//  - rochade is indicated by a single move of king over 2 pieces (rook to be moved)
	//  - en passent is indicated by a movement of striking piece only (killed pawn to be removed)

	uint8_t src;
	uint8_t dest;
	uint8_t val;

	uint8_t info_reduction=0;
	info_reduction = (mem[0x1e]&0x40)?(1):(0);

	for (i=0;i!=(uint8_t)mem[0x6c]-info_reduction;i++) {
		// legacy not sufficient anymore ;-)
		//board[(uint8_t)mem[0x570+i]] = board[(uint8_t)mem[0x4b0+i]];
		//board[(uint8_t)mem[0x4b0+i]] = 0x00;
		src = (uint8_t)mem[0x4b0+i];
		dest = (uint8_t)mem[0x570+i];
		val = board[src];
		// additional action for double move
		if (val == 0x16 && src == 0x19 && dest == 0x17) {
			// white rochade queen side (rook movement)
			board[0x18] = 0x13; 
			board[0x15] = 0x00;
		} else if (val == 0x16 && src == 0x19 && dest == 0x1b) {
			// white rochade king side
			board[0x1a] = 0x13;
			board[0x1c] = 0x00;
		} else if (val == 0x26 && src == 0x5f && dest == 0x5d) {
			// black rochade queen side
			board[0x5e] = 0x23;
			board[0x5b] = 0x00;
		} else if (val == 0x26 && src == 0x5f && dest == 0x61) {
			// black rochade king side
			board[0x60] = 0x23;
			board[0x62] = 0x00;
		}
		if (val == 0x10 && board[dest] == 0x00 && dest-src != 10) {
			// white en passent (pawn movement on diagonal on empty field)
			board[(uint8_t)(dest-10)] = 0x00;

		}
		if (val == 0x20 && board[dest] == 0x00 && src-dest != 10) {
			// black en passent (pawn movement on diagonal on empty field)
			board[(uint8_t)(dest+10)] = 0x00;
		}
		// by default convert pawn to queen when in final rank
		if (val == 0x10 && dest >= 0x5B) {
			val = 0x15;
		}
		if (val == 0x20 && dest <= 0x1c) {
			val = 0x25;
		}

		// execute standard move
		board[dest] = val;
		board[src] = 0x00;
		
	}

	for (i=0;i!=8;i++) {
		for (o=0;o!=8;o++) {
			tmp = board[(uint8_t)baddr[i]+o];
			if (tmp) {
				if (tmp & 0x10) {
					tmp &= 0x0F;
				} else { 
					tmp &= 0x0F;
					tmp += 7;
				}
				r.x = boardx[o];
				r.y = boardy[i+2];
				r.w = 104;
				r.h = 104;
				SDL_BlitScaled(gImgFigure[tmp],NULL,gScreenBuffer,&r);

			}
		}
	}

	return 1;
}

#ifdef TESTFUNCTION
#warning DEBUGGING OPTION TEST-Function with t activated
void test(void) {
	/* test 1
	printf("0x0000: %02x%02x\n",mem[0],mem[1]);
	printf("0x0002: %02x%02x\n",mem[2],mem[3]);
	printf("0x0004: %02x%02x\n",mem[4],mem[5]);
	printf("0x006c: %02x\n",mem[0x6c]);
	*/
	/* test2 
	int i;
	int baseaddr = 0x0000;
	int res=0;
	for (i=0;i!=16;i++) {
		res += (uint8_t)mem[0xb0+i];
	}
	printf("==================================================================\n");
	printf("OPENING INDICATOR: %i",res);
	printf("==================================================================\n");
	for (i=0;i!=0x1bf;i++) {
		if (i % 16 == 0) {
			printf("\n0x%04x:",baseaddr+i);
		}
		//printf("0x%04x: %02x\n",0x330+i,mem[0x330+i]);
		printf(" %02x",mem[baseaddr+i]);
	}
	*/
	int i;
	int baseaddr = 0x0000;
	int res=0;
	res = (mem[0x1e] & 0x40)?(1):(0);
	printf("==================================================================\n");
	printf("OPENING INDICATOR: %i",res);
	printf("==================================================================\n");
	for (i=0;i!=0x1bf;i++) {
		if (i % 16 == 0) {
			printf("\n0x%04x:",baseaddr+i);
		}
		//printf("0x%04x: %02x\n",0x330+i,mem[0x330+i]);
		printf(" %02x",mem[baseaddr+i]);
	}
}
#endif // TESTFUNCTION

int led2int(uint8_t c) {
	int i;
	for (i=0;i!=8;i++) {
		if (c & (1<<i)) 
			return i;
	}
	return 0;
}

int INPUT_Handler(void) {
	// return 0 in case of quit, else 1
	// handle global vars for emulator inputs
	SDL_Event e;
	while (SDL_PollEvent(&e) != 0) {
		if (e.type == SDL_QUIT) {
			return 0;
		} else if (e.type == SDL_KEYUP) {
			switch (e.key.keysym.sym) {
				case SDLK_q:
					return 0;
#ifdef DUMPRAM
				// DUMP RAM HOTKEYS
				case SDLK_F1:
					DumpRAM("/tmp/dump1.bin");
					break;
				case SDLK_F2:
					DumpRAM("/tmp/dump2.bin");
					break;
				case SDLK_F3:
					DumpRAM("/tmp/dump3.bin");
					break;
				case SDLK_F4:
					DumpRAM("/tmp/dump4.bin");
					break;
				case SDLK_F5:
					DumpRAM("/tmp/dump5.bin");
					break;
				case SDLK_F6:
					DumpRAM("/tmp/dump6.bin");
					break;
				case SDLK_F7:
					DumpRAM("/tmp/dump7.bin");
					break;
				case SDLK_F8:
					DumpRAM("/tmp/dump8.bin");
					break;
#endif // DUMPRAM
				// 210604 new feature: load/save game
				case SDLK_l:
					if (LoadGame("/tmp/mephisto02.sav")) {
						printf("Game restored.");
					} else {
						printf("Load game failed.");
					}
					break;
				case SDLK_s: 
					if (SaveGame("/tmp/mephisto02.sav")) {
						printf("Game saved.");
					} else {
						printf("Save game failed.");
					}
					break;
#ifdef TESTFUNCTION
				case SDLK_t:
					test();
					break;
#endif // TESTFUNCTION
				case SDLK_1:
					cursor = cursor % 8;
					cursor += 16;
					break;
				case SDLK_2:
					cursor = cursor % 8;
					cursor += 24;
					break;
				case SDLK_3:
					cursor = cursor % 8;
					cursor += 32;
					break;
				case SDLK_4:
					cursor = cursor % 8;
					cursor += 40;
					break;
				case SDLK_5:
					cursor = cursor % 8;
					cursor += 48;
					break;
				case SDLK_6:
					cursor = cursor % 8;
					cursor += 56;
					break;
				case SDLK_7:
					cursor = cursor % 8;
					cursor += 64;
					break;
				case SDLK_8:
					cursor = cursor % 8;
					cursor += 72;
					break;
				case SDLK_a:
					cursor = cursor - (cursor % 8);
					break;
				case SDLK_b:
					cursor = cursor - (cursor % 8)+1;
					break;
				case SDLK_c:
					cursor = cursor - (cursor % 8)+2;
					break;
				case SDLK_d:
					cursor = cursor - (cursor % 8)+3;
					break;
				case SDLK_e:
					cursor = cursor - (cursor % 8)+4;
					break;
				case SDLK_f:
					cursor = cursor - (cursor % 8)+5;
					break;
				case SDLK_g:
					cursor = cursor - (cursor % 8)+6;
					break;
				case SDLK_h:
					cursor = cursor - (cursor % 8)+7;
					break;
				case SDLK_i:
					cursor = led2int(mem[2])*8;
					cursor += led2int(mem[0]);
					cursor += 16;
					break;
				case SDLK_DOWN:
					if (cursor/8)
						cursor -= 8;
					break;
				case SDLK_UP:
					if (cursor < 72) 
						cursor += 8;
					break;
				case SDLK_LEFT:
					if (cursor % 8) 
						cursor -= 1;
					break;
				case SDLK_RIGHT:
					if (cursor % 8 != 7)
						cursor += 1;
					break;
			}
			cursor_sel = -1;
		} else if (e.type == SDL_KEYDOWN) {
			if (e.key.keysym.sym == SDLK_RETURN) {
				if (cursor >= 16) {
					cursor_sel = cursor-16;
				} else {
					cursor_sel = cursor+128;
				}
			}
		}
	}
	return 1;
}

// MAIN
int main(int argc,char *argv[]) {
	int run = 1;
	int i;

	uint8_t refreshgfx = 0;
	int16_t nmi_timer = 4096;
	uint32_t sync_timer = 0;
	uint32_t lastTime = 0;
	uint8_t ledtimer = 0;

	int last_cursor = 0;
	uint8_t last_led[8] = "\0\0\0\0\0\0\0";
	uint8_t last_ledtimer = 0;
	
	if (!GFX_Init()) {
		return 1;
	}

	if (!SND_Init()) {
		return 1;
	}

	DAC_init();

	// emulator initialization
	MemInitialize();

	lastTime = SDL_GetTicks();
	while (run) {
		// emulator
		i = CpuExecute();
		nmi_timer -= i;
		ticks += i;
		sync_timer += i;

		if (nmi_timer < 0) {
			nmi_timer = 4096;
			NMI_SetLine();
		}
		//if (sync_timer >= 100000) {
		if (sync_timer >= 200000) {
			sync_timer = 0;
			uint32_t delta = SDL_GetTicks() - lastTime;
			//i = SDL_GetTicks() - lastTime;
			if (delta >= 0) {
				SDL_Delay(100-i);
				//SDL_Delay(50-delta);
			}
			lastTime = SDL_GetTicks();
		} else {
			continue;
		}
		/*		
		//if (ticks >= 200000) {
		if (ticks >= 100000) {
			ticks = 0;
			uint32_t delta = SDL_GetTicks() - lastTime;
			//i = SDL_GetTicks() - lastTime;
			if (delta >= 0) {
				//SDL_Delay(100-i);
				SDL_Delay(50-delta);
			}
			lastTime = SDL_GetTicks();
		} else {
			continue;
		}
		*/
		// SDL input handline 
		run = INPUT_Handler();

		// GFX update
		// conditions to update screen
		//  - led change in memory 0x0000-0x0005
		//  - led timer passed (maybe in combinatin with flashing led)
		//  - cursor change
		refreshgfx = 0;
		
		if (last_cursor != cursor) {
			last_cursor = cursor;
			refreshgfx = 1;
		}
		for (i=0;i!=6;i++) {
			if (mem[i] != last_led[i]) {
				last_led[i] = mem[i];
				refreshgfx = 1;
			}
		}
		ledtimer = (SDL_GetTicks()/500)%2;
		if (ledtimer != last_ledtimer) {
			last_ledtimer = ledtimer;
			refreshgfx = 1;
		}
		
		if (!refreshgfx) {
			continue;
		}

		// dies hier sind die CPU-Fresser!
		GFX_DrawBoard();
		//GFX_DrawLEDs(SDL_GetTicks()/500);
		GFX_DrawLEDs(ledtimer);
		GFX_Update();
		
	}
	SND_Destroy();
	GFX_Destroy();
	// for debuggin only
	//DumpRAM("ram.bin");
	return 0;
}
