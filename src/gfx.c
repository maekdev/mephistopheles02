// gfx.c
//
// 241020
// markus ekler
// 
// gfx handler and input

// INCLUDES
#include "config.h"
#include "gfx.h"
#include "emu.h"

#include "gfx/png_chessboard.h"
#include "gfx/png_newgfx.h"

// LOCAL STUFF
SDL_Window *gWindow = NULL;
SDL_Surface *gScreenSurface = NULL;
//SDL_Surface *gScreenBuffer = NULL;
SDL_Renderer *gRenderer = NULL;

SDL_AudioDeviceID dev;
#define SAMPLE_RATE 44100    // Audio sample rate in Hz
#define AMPLITUDE 4000      // Amplitude of the square wave
#define CHANNELS 1           // Mono audio

// Struct to hold tone information
typedef struct {
    int sample_index;    // Current sample index in the tone
    int playing;         // Flag to indicate if a tone is currently playing
} Tune;
Tune tune = { 0,0 };

int cursor = 16;
SDL_Texture *gImgBoardW = NULL;
SDL_Texture *gImgBoardB = NULL;
SDL_Texture *gImgBoardF = NULL;

int gNoMouse = 0;

// local buffer for field coords
//int boardx[8] = { 82,163,244,325,406,487,568,649 };
const int cboardx[8] = { PNG_BOARDX0, PNG_BOARDX1, PNG_BOARDX2, PNG_BOARDX3, PNG_BOARDX4, PNG_BOARDX5, PNG_BOARDX6, PNG_BOARDX7 };
int boardx[8] = { PNG_BOARDX0, PNG_BOARDX1, PNG_BOARDX2, PNG_BOARDX3, PNG_BOARDX4, PNG_BOARDX5, PNG_BOARDX6, PNG_BOARDX7 };
//int boardy[10] = { 891,836,636,556,477,397,318,239,159,80 };
const int cboardy[10] = { PNG_BOARDY0, PNG_BOARDY1, PNG_BOARDY2, PNG_BOARDY3, PNG_BOARDY4, PNG_BOARDY5, PNG_BOARDY6, PNG_BOARDY7, PNG_BOARDY8, PNG_BOARDY9 };
int boardy[10] = { PNG_BOARDY0, PNG_BOARDY1, PNG_BOARDY2, PNG_BOARDY3, PNG_BOARDY4, PNG_BOARDY5, PNG_BOARDY6, PNG_BOARDY7, PNG_BOARDY8, PNG_BOARDY9 };
int boardw = PNG_FIELD_W;
int boardh = PNG_FIELD_H;
int boardledx[24];
int boardledy[24];


// DEFINES (only locally needed)
#define BADDR2X(a,b) 	(b)?(7-((a-0x15)%10)):((a-0x15)%10)
#define BADDR2Y(a,b)	(b)?(9-((a-0x15)/10)):((a-0x15)/10)

//SDL_Rect srcRect;
SDL_Rect dstRect;

void GFX_ResizeCalc(void) {
	// assumes window to be loaded
	// this function calculates src and dest boxes to be used in DrawBoard
	int w=0,h=0;
	if (gWindow == NULL) 
		return;
	SDL_GetWindowSize(gWindow,&w,&h);
	//printf("ResizeCalc: %i/%i\n",w,h);
	// calculate the bestfit dimensions
	float scale_x = (float)w / PNG_BOARD_W;
	float scale_y = (float)h / PNG_BOARD_H;
	float scale = (scale_x < scale_y) ? scale_x : scale_y;

	dstRect.w = (int)(PNG_BOARD_W*scale);
	dstRect.h = (int)(PNG_BOARD_H*scale);
	dstRect.x = (w-dstRect.w)/2;
	dstRect.y = (h-dstRect.h)/2;

	//printf(" -> (%i/%i) w=%i,h=%i\n",dstRect.x,dstRect.y,dstRect.w,dstRect.h);
	// calculate field variable coordinates
	for (int i=0;i!=8;i++) {
		//boardx[i] = (int)(PNG_BOARDX[i]*scale);
		//boardy[i] = (int)(PNG_BOARDY[i]*scale);
		boardx[i] = (int)(cboardx[i]*scale);
		boardy[i] = (int)(cboardy[i]*scale);
	}
	boardy[8] = (int)(cboardy[8]*scale);
	boardy[9] = (int)(cboardy[9]*scale);
	boardw = (int)(PNG_FIELD_W*scale);
	boardh = (int)(PNG_FIELD_H*scale);
	// calculate led pos
	for (int i=0;i!=8;i++) {
		boardledx[i] = dstRect.x + boardx[i] + (boardw>>4); // function keys at 1/4
		boardledx[i+8] = dstRect.x + boardx[i] + (boardw>>1)-(boardw>>3);
		boardledx[i+16] = dstRect.x + boardx[0] - (boardw>>2);
		boardledy[i] = dstRect.y + boardy[1]-(boardh>>2);
		boardledy[i+8] = dstRect.y + boardy[2] + boardh + (boardh>>2);
		boardledy[i+16] = dstRect.y + boardy[2+i]+(boardh>>1)-(boardh>>3);
	}
}

// FUNCTIONS
void audio_callback(void *userdata, Uint8 *stream, int len) {
    int16_t *buffer = (int16_t *)stream;
    int samples = len / sizeof(int16_t);
	int sample_count = 0;
	int frequency = 0;
    
	switch (tune.playing & 0x03) {
		case 0x01:
			frequency = 1833/2;
			sample_count = 100*SAMPLE_RATE / frequency;
			break;
		case 0x02:
			frequency = 1833/4;
			sample_count = 100*SAMPLE_RATE / frequency;
			break;
		default: // 0x00
    		// If no tone is playing, output silence
			SDL_memset(stream,0,len);
			return;
	}

    int samples_per_half_period = SAMPLE_RATE / (frequency * 2);

    for (int i = 0; i < samples; i++) {
        // Generate square wave signal
        buffer[i] = (tune.sample_index / samples_per_half_period) % 2 == 0 ? AMPLITUDE : -AMPLITUDE;

        // Increment the sample index
        tune.sample_index++;
        
        if (tune.sample_index >= sample_count) {
            //tune.playing = 0;         // Stop playback
			tune.playing = tune.playing >> 2;
            tune.sample_index = 0;    // Reset sample index for the next tone
            break;
        }
    }
}

int SND_Init(uint8_t nosound) {
	// NOTES on audio output
	// - boot sound (1) - (2) - (1)
	// @22000Hz one period = 24pulses (1) starting @104 = 100 periods
	//          one period = 50pulses (2) starting @2505 = 100 periods
	//          one period = 24pulses (1) starting @7505 = 100 periods
	// - stop thinking sound (1) - (1) ???
	SDL_AudioSpec want, have;
    SDL_AudioDeviceID dev;

	if (nosound) 
		return 1;

    SDL_memset(&want, 0, sizeof(want));
    want.freq = SAMPLE_RATE;
    want.format = AUDIO_S16SYS;
    want.channels = CHANNELS;
    want.samples = 2048;
    want.callback = audio_callback;

	if (SDL_Init(SDL_INIT_AUDIO) < 0) 
		return 0;

    dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
    if (dev == 0) {
        return 0;
    }

    SDL_PauseAudioDevice(dev, 0);  // Start audio playback
	return 1;
}

void SND_Play(uint8_t t) {
	tune.sample_index = 0;
	tune.playing = t; // encode everything into these 8bit
}

int SND_Destroy(void) {
    SDL_CloseAudioDevice(dev);
	return 1;
}

int GFX_Init(void) {
	
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		printf("ERR: SDL_Init() - %s\n",SDL_GetError());
		return 0;
	} 

	gWindow = SDL_CreateWindow("MEphisto Episode II - Lucifer s Life",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,SCREEN_WIDTH,SCREEN_HEIGHT,SDL_WINDOW_SHOWN|SDL_WINDOW_RESIZABLE);

	if (gWindow == NULL) {
		printf("ERR: SDL_CreateWindow() - %s\n",SDL_GetError());
		return 0;
	} else {
		gRenderer = SDL_CreateRenderer(gWindow,-1,SDL_RENDERER_ACCELERATED);
		if (!gRenderer) {
			printf("ERR: CreateRenderer: %s\n",SDL_GetError());
			return 0;
		}
		// seems to work all lets get sdl_image as well
		if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
			printf("ERR: IMG_Init(): %s\n",IMG_GetError());
			return 0;
		}

		// we start with loading board and figures as we need board image size

		SDL_RWops *rw;
		SDL_Surface *tmp;
		// white board
		rw = SDL_RWFromMem(MephistoMondial2_white_png, MephistoMondial2_white_png_len);
		tmp = IMG_Load_RW(rw, 1); // Load image using SDL_Image
		gImgBoardW = SDL_CreateTextureFromSurface(gRenderer,tmp);
		SDL_FreeSurface(tmp);
		// black board
		rw = SDL_RWFromMem(mondial2_black_png, mondial2_black_png_len);
		tmp = IMG_Load_RW(rw, 1); // Load image using SDL_Image
		gImgBoardB = SDL_CreateTextureFromSurface(gRenderer,tmp);
		SDL_FreeSurface(tmp);
		// figures
		rw = SDL_RWFromMem(MephistoMondial2_figures_png,MephistoMondial2_figures_png_len);
		tmp = IMG_Load_RW(rw,1);
		gImgBoardF = SDL_CreateTextureFromSurface(gRenderer,tmp);
		SDL_FreeSurface(tmp);
	}

	return 1;
}

int GFX_Destroy(void) {
	SDL_DestroyRenderer(gRenderer);
	SDL_DestroyTexture(gImgBoardW);
	SDL_DestroyTexture(gImgBoardB);
	SDL_DestroyTexture(gImgBoardF);
	SDL_DestroyWindow(gWindow);
	gWindow = 0;

	IMG_Quit();
	SDL_Quit();

	return 1;
}

#ifdef _DEBUG
void MemoryDump(uint16_t start) {
	int i;
	for (i=start;i!=start+512;i++) {
		if (i%16 == 0) {
			printf("\n%04x: ",i);
		} else if (i%2 == 0) {
			printf(" ");
		}
		printf("%02x",EMU_GetRAM(i));
	}
}
#endif // _DEBUG

int GFX_InputHandler(int playblack) {
	int ret = 0; // return value is one for every game relevant input (not 
	static int savestate = 0;
	char filename[] = "savestate0.bin";
    SDL_Event e;
	while (SDL_PollEvent(&e) != 0) {
		if (e.type == SDL_QUIT) {
			//running = 0;
			ret = -1;
		} else if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
			GFX_ResizeCalc();
		}
		// Handle input events here (keyboard, mouse, etc.)
		if (e.type == SDL_KEYUP) {
			EMU_SetButton(-1);
			ret = 1;
		} else if (e.type == SDL_MOUSEBUTTONDOWN && !gNoMouse) {
			printf("Mouse button pressed.\n");
			SND_Play(0x01);
			if (cursor >= 16) {
				EMU_SetButton(cursor-16);
			} else {
				EMU_SetButton(cursor+128);
			}
			ret = 1;
		} else if (e.type == SDL_MOUSEMOTION && !gNoMouse) {
			//printf("Mouse moving in field (%i,%i)\n",e.motion.x,e.motion.y);
			uint32_t x = e.motion.x-dstRect.x;
			uint32_t y = e.motion.y-dstRect.y;
			if (x >= boardx[0] && x <= (boardx[7]+boardw)) {
				// detect if mouse in function key area (first row)
				if (y >= boardy[1] && y <= (boardy[1]+(boardh>>1))) {
					x -= boardx[0];
					x /= boardw;
					cursor = 8+(x%8);
				// detect if mouse in function key area (second row)
				} else if (y >= boardy[0] && y <= (boardy[0]+(boardh>>1))) {
					x -= boardx[0];
					x /= boardw;
					cursor = (x%8);
				// detect if mouse in chess board area
				} else if (y <= (boardy[2]+boardh) && y >= boardy[9]) {
					x -= (boardx[0]);
					x /= boardw;
					y -= (boardy[9]);
					y /= boardh;
					cursor = 16+((7-(y%8))*8+(x%8));
					ret = 1;
					//printf("[BOARD] Setting cursor to (%i,%i) = %i\n",x,y,cursor);
				}
			}
			
		} else if (e.type == SDL_KEYDOWN) {
			switch (e.key.keysym.sym) {
				case SDLK_q:
				case SDLK_ESCAPE:
					//running = 0;
					ret = -1;
					break;
				case SDLK_RETURN:
					SND_Play(0x01);
					if (cursor >= 16) {
						EMU_SetButton(cursor-16);	
					} else {
						EMU_SetButton(cursor+128);	
					}
					ret = 1;
					break;
				case SDLK_F2:
					savestate += 1;
					savestate %= 10;
					printf("F2 pressed. Save State %i selected.\n",savestate);
					break;
				case SDLK_F3:
					printf("F3 pressed. Save State %i.\n",savestate);
					filename[9] = '0'+savestate;
					EMU_SaveState(filename);
					break;
				case SDLK_F4:
					printf("F4 pressed. Restore State %i.\n",savestate);
					filename[9] = '0'+savestate;
					EMU_RestoreState(filename);
					ret = 1;
					break;
				case SDLK_t:
					// TEST Functions
					printf("LASTMOVE(-3): %04x\n",EMU_GetLastMove(-3));
					printf("LASTMOVE(-1): %04x\n",EMU_GetLastMove(-1));
					printf("LEDPOS  : %04x\n",EMU_GetLEDPos());
					// run for several ticks
					for (int i=0;i!=500000;i++) EMU_Tick();
					break;
				#ifdef _DEBUG
				case SDLK_m:
					printf("\n\nMemory Dump:\n");
					MemoryDump(0x0000);
					break;
				case SDLK_n:
					printf("\n\nPosition history Dump:\n");
					MemoryDump(0x04b0);
					break;
				case SDLK_u:
					printf("\n\nValue / Position:\n");
					MemoryDump(0x02e0);
					break;
				case SDLK_k:
					// experimental automove
					EMU_AutoMove();
					ret = 1;
					break;
				#endif // _DEBUG
				// Handle other keys...
				case SDLK_1:
				case SDLK_2:
				case SDLK_3:
				case SDLK_4:
				case SDLK_5:
				case SDLK_6:
				case SDLK_7:
				case SDLK_8:
					cursor = cursor % 8;
					if (playblack) { 
						cursor += 16+(8*(7-(e.key.keysym.sym-SDLK_1))); // little awkward formular to prevent typing 8 times the same stuff
					} else {
						cursor += 16+(8*(e.key.keysym.sym-SDLK_1)); 
					}
					ret = 1;
					break;
				case SDLK_a:
				case SDLK_b:
				case SDLK_c:
				case SDLK_d:
				case SDLK_e:
				case SDLK_f:
				case SDLK_g:
				case SDLK_h:
					cursor = cursor - (cursor % 8);
					if (playblack) {
						cursor += (7-(e.key.keysym.sym-SDLK_a));
					} else {
						cursor += (e.key.keysym.sym-SDLK_a);
					}
					ret = 1;
					break;
				case SDLK_DOWN:
					if (cursor/8)
						cursor -= 8;
					ret = 1;
					break;
				case SDLK_UP:
					if (cursor < 72) 
						cursor += 8;
					ret = 1;
					break;
				case SDLK_LEFT:
					if (cursor % 8) 
						cursor -= 1;
					ret = 1;
					break;
				case SDLK_RIGHT:
					if (cursor % 8 != 7)
						cursor += 1;
					ret = 1;
					break;
			}
		}
	}
	return ret;
}

int GFX_Update(void) {
	// only blitscale buffer to screen and update
	SDL_RenderPresent(gRenderer);
	return 1;
}

int GFX_DrawLEDs(int state) {
	// x-LED @ mem 0x0000 (LED16..23)
	// y-LED @ mem 0x0002 (LED08..15)
	// bottom-LED @ mem 0x0004 (LED00..07)
	// swapping on 500ms timer to address +1 -- based on state variable: if odd state then mem(1,3,5) else mem(0,2,4) for led display
	// this creates the winning animation & loading blink

	// revised implementation (calculation based on boardx,boardy & boardw)
	SDL_Rect r;

	for (int i=0;i!=24;i++) {
		r.x = boardledx[i];
		r.y = boardledy[i];
		if (i<16) {
			r.w = boardw>>2;
			r.h = boardh>>3;
		} else {
			r.w = boardw>>3;
			r.h = boardh>>2;
		}
		if (EMU_GetLED(i,state)) 
			SDL_SetRenderDrawColor(gRenderer,COLOR_LEDON);
		else 
			SDL_SetRenderDrawColor(gRenderer,COLOR_LEDOFF);
		
		SDL_RenderFillRect(gRenderer,&r);
			
	}

	return 1;
}

int GFX_DrawBoard(int playblack) {
	int i,o,tmp;
	//const int boardx[] = { 78,185,289,394,499,603,708,813 };
	//const int boardy[] = { 1152,1073,812,706,605,499,394,288,182,76 };
	//const uint16_t baddr[] = { 0x215,0x21F,0x229,0x233,0x23D,0x247,0x251,0x25B };
	SDL_Rect r;

	// fill with black color
	//gScreenSurface = SDL_GetWindowSurface(gWindow);
	//SDL_FillRect(gScreenBuffer,NULL,SDL_MapRGB(gScreenBuffer->format,0,0,0));
    SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, 255);
    SDL_RenderClear(gRenderer);

	// update background image
	if (playblack) {
		SDL_Rect s = { 0,0,PNG_BOARD_W,PNG_BOARD_H };
		SDL_RenderCopy(gRenderer,gImgBoardB,&s,&dstRect);
	} else {
		SDL_Rect s = { 0,0,PNG_BOARD_W,PNG_BOARD_H };
		SDL_RenderCopy(gRenderer,gImgBoardW,&s,&dstRect);
	}

	// draw cursor
	if (cursor >= 16) { // chess board
		r.x = dstRect.x + boardx[cursor%8];
		r.y = dstRect.y + boardy[cursor/8];
		r.w = boardw;
		r.h = boardh;
	} else { // function keys
		r.x = dstRect.x + boardx[cursor%8];
		r.y = dstRect.y + boardy[cursor/8];
		r.w = (boardw);
		r.h = (boardh)>>1; // 1/2 of the height
	}

	SDL_SetRenderDrawColor(gRenderer,COLOR_CURSOR);
	SDL_RenderFillRect(gRenderer,&r);

	// Draw the figures
	// 0x215,21F,229,233,23D,247,251,25B following each 8bytes
	// B=0x10,T=0x13,S=0x12,L=0x11,K/D?=0x15/16 (wh |= 0x10, bl |= 0x20)

	// fixed beta version 
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
		uint8_t position = EMU_GetRAM(0x2e0+i);
		uint8_t value = EMU_GetRAM(0x1a0+i);
		//board[(uint8_t)mem[0x2e0+i]] = (uint8_t)mem[0x1a0+i];
		board[(uint8_t)position] = (uint8_t)value;
	}
	// calculation of current board based on following considerations:
	//  - 0x6c contains number of moves
	//  - 0x1e if bit 6 is set (0x40) then the move history contains the best move for human player (reduce length by one for drawing) on ERR move this field contains 0xFF
	//  - 0x4b0+x contains start field address (history of moves)
	//  - 0x570+x contains destination field address (history of moves)
	//  - rochade is indicated by a single move of king over 2 pieces (rook to be moved)
	//  - en passent is indicated by a movement of striking piece only (killed pawn to be removed)

	uint8_t src=0;
	uint8_t dest=0;
	uint8_t val;

	uint8_t info_reduction=(EMU_GetRAM(0x1e)==0x40)?(1):(0);
	uint8_t move_count=EMU_GetRAM(0x6c);

	for (i=0;i!=(uint8_t)move_count-info_reduction;i++) {
		src = (uint8_t)EMU_GetRAM(0x4b0+i);
		dest = (uint8_t)EMU_GetRAM(0x570+i);
		val = board[(src&0x7f)];
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
		// promotion support:
		//  encoded in msb of src & dest 
		//   s=1,d=1 -> knight (0x.1)
		//   s=0,d=1 -> bishop (0x.2)
		//   s=1,d=0 -> rook   (0x.3)
		//   s=0,d=0 -> queen  (0x.5)
		if ((val == 0x10 && (dest&0x7F) >= 0x5B) || (val == 0x20 && (dest&0x7F) <= 0x1c)) {
			val &= 0x30;
			printf("%02x",((src&0x80)>>6)|((dest&0x80)>>7));
			switch (((src&0x80)>>6)|((dest&0x80)>>7)) {
				case 3: val |= 0x01; break;
				case 1: val |= 0x02; break;
				case 2: val |= 0x03; break;
				default: val |= 0x05; break;
			}
			src &= 0x7F;
			dest &= 0x7F;
		}

		// execute standard move
		//if ((val >= 0x10 && val <= 0x16) || (val >= 0x20 && val <= 26)) 
		board[dest] = val;
		board[src] = 0x00;
	}

	// draw last move played (src & dest)
	if (move_count > 0) {
		SDL_SetRenderDrawColor(gRenderer,COLOR_LASTMOVE);
		r.x = dstRect.x + boardx[BADDR2X(src,playblack)];
		r.y = dstRect.y + boardy[BADDR2Y(src,playblack)+2];
		r.w = boardw;
		r.h = boardh>>4;
		SDL_RenderFillRect(gRenderer,&r);
		r.y += ((boardh*15)>>4);
		SDL_RenderFillRect(gRenderer,&r);
		r.y -= ((boardw*15)>>4);
		r.w = boardw>>4;
		r.h = boardh;
		SDL_RenderFillRect(gRenderer,&r);
		r.x += ((boardw*15)>>4);
		SDL_RenderFillRect(gRenderer,&r);
		//SDL_RenderDrawRect(gRenderer,&r);
		r.x = dstRect.x + boardx[BADDR2X(dest,playblack)];
		r.y = dstRect.y + boardy[BADDR2Y(dest,playblack)+2];
		r.w = boardw;
		r.h = boardh>>4;
		SDL_RenderFillRect(gRenderer,&r);
		r.y += ((boardh*15)>>4);
		SDL_RenderFillRect(gRenderer,&r);
		r.y -= ((boardw*15)>>4);
		r.w = boardw>>4;
		r.h = boardh;
		SDL_RenderFillRect(gRenderer,&r);
		r.x += ((boardw*15)>>4);
		SDL_RenderFillRect(gRenderer,&r);
	}

	SDL_Rect s;

	// draw board
	for (i=0;i!=8;i++) {
		for (o=0;o!=8;o++) {
			tmp = board[(uint8_t)baddr[i]+o];
			if (tmp) {
				if (tmp & 0x10) {
					s.y = 1;
				} else {
					s.y = 4+PNG_FIGURE_H;
				}
				if ((tmp&0x0F) >= 5) tmp--; // mod for king & queen 
				s.x = 1+((tmp&0x0F)*(PNG_FIGURE_W+2));
				s.w = PNG_FIGURE_W;
				s.h = PNG_FIGURE_H;

				r.x = (playblack)?(boardx[7-o]):(boardx[o]);
				r.x += dstRect.x;
				r.y = (playblack)?(boardy[((7-i)+2)]):(boardy[i+2]);
				r.y += dstRect.y;
				r.w = boardw;
				r.h = boardh;
				SDL_RenderCopy(gRenderer,gImgBoardF,&s,&r);
				//SDL_BlitScaled(gImgBoardF,&s,gScreenBuffer,&r);
			}
		}
	}
	return 1;
}


int GFX_EnableMouse(int nomouse) {
	gNoMouse = nomouse;
	return 0;
}
