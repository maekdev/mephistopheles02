// main.c
// 
// 241007
// markus ekler
//
// completely rewritten meph02 version

// TODO
//  X add last movement highlight (box arround opponent's last move)A
//  X control keypress by function
//  X auto-move piece after calculation
//  X send gfx functions into seperate header+src
//  X support for including png files as resources into binary
//  X integrated resources with stb_image (maybe only for win32 version needed)
//  X clean makefile: move stuff to library folder and compile libraries with seperate Makefile
//  X add global config handler (single config.h + cfg_file?)
//  X independent emulator code / seamless library change for (vr65 & m6502)
//  X bugfix for rochade (black's move)
//  X implement loading/saving game state
//  X support for reading of ROM file from external file (compiler option for built-in or external file)
//  X zip support for external file (only needed for ROM file)
//  X implement verfication of rom file / md5 checksum
//  X savestate support with F2 selecting the state, F3 saving, F4 restoring
//  X Support for "Play as black" (<- this is a tough nut)
//		X included as command-line feature
//		X supported by engine
//		X graphics created
// 		X graphics implemented in gameplay
//		X automove when playing as black
//		X keyboard keys adapted
//  X reduce cpu load
//		X only refresh 2x per second
//		X only refresh on keypress
//  	X idle CPU on human to move (this requires major redesign)
//		X only run EMU when needed (new game mode / set as default)
//  X command-line features (should be part of cfg)
//		X set mephisto strength level
//		X set rom file 
//		X nosound
// 		X noautomove (why should anybody want to use this)
//		- realistic-mode (emulate mephisto's calculation time)
//		- immediate (default - does not need a configuration - include automove & and quickcalc)
//		X nomouse as option
//      - verbose mode (needed as alternative for printf -> logoutput)
//		- setscalemul & setscalediv - add possibility to modify displayscale (implemented with resize)
//		o UCI mode
//  X implement auto-move for white rochade (rook movement)
//  X implement auto-move for computer promotion (requires keypress)
//  X bugfix GFX_DrawBoard for piece promotion
//  X bugfix on graphics (mirror issue)
//  X bugfix automove prevents mem option for normal gameplay (combine with LED detect)
//  - implement error handling (detection and correction on ERR+ LED) - questionable: just don't cause errors
//  X audio output (work in progress)
//		- realistic / real-time output
//      X need some test program to record audio / output in testfile + analysis
//  	X implement audio feedback for black stops thinking --> white to move
//		X audio tune on power up SND_PLAY(0x19)
//		X audio tune on keypress SND_PLAY(0x01)
//  X mouse support for button control
//  o UCI game mode support
//   	o change printf statements (special log option + disable on uci mode)
//		o implement stdin/stdout
//		o new option force gfx
//  o win32 support
//  X create new graphics with LibreOffice Draw (pieces, black & white)
//  X implement new graphics for background / board (black & white)
//		X board/figure display independent of window size
//      X led display independet of window size
//		X new function to draw in the middle of visible area and blit accordingly
//  X change SDL implementation to Renderer/Texture usage instead of Blit
//  X bugfix: when rochade is suggested move of player, the automatic delay is inserted
//  o bugfix: DrawBoard (gfx) fails in endgame with only one move left
//  o Makefile install (for linux version)

#include "config.h"

#include "cfg.h"
#include "gfx.h"
#include "emu.h"

// Global Variables
uint16_t state_last = 0x0000;
uint16_t state_cur = 0x0000;

uint16_t observer(uint16_t ls,uint16_t s,int playblack) {
	// indicate state transitions and actions to take
	if (ls == s) {
		return 0;
	} else {
		// for debug only
		//printf("OBSERVER: state change from %04x to %04x\n",ls,s);
		if (playblack) {
			if (STATE_WHITECALC(ls) && !STATE_WHITECALC(s)) {
				SND_Play(0x05);
			} else if (STATE_BLACK(ls) && !STATE_WHITECALC(s)) {
				SND_Play(0x05);
			}
		} else {
			if (STATE_BLACKCALC(ls) && !STATE_BLACKCALC(s)) {
				SND_Play(0x05);
			} else if (STATE_WHITE(ls) && !STATE_BLACKCALC(s)) {
				SND_Play(0x05);
			}
		}
	}
	return 1;
}

// UCI game loop
void UCI_main(void) {
	printf("Starting in UCI mode...");

}

// Main game loop
int main(int argc, char* argv[]) {
	int running = 1; // To keep the main loop running
	uint8_t ledtimer = 0;
	uint8_t last_ledtimer = 0;
	Config *cfg;

	if (!CFG_Init()) {
		fprintf(stderr,"ERR: Initial configuration failed.\n");
		return 1;
	}

    if (!EMU_Init()) {
		fprintf(stderr,"ERR: emulator init failed.\n");
		return 3;
	}

	CFG_ParseArgs(argc,argv);

	cfg = CFG_GetConfig();
	if (cfg->force_quit) {
		// just exit the program
		return 0;
	}
	if (cfg->uci) {
		cfg->nosound = 1;
		UCI_main();
		return 0;
	}
	
	printf("Mephisto: loading ROM file...\n");
	if (!EMU_LoadROM(cfg->romfile)) {
		fprintf(stderr,"ERROR - no romfile found.\n");
		return 4;
	}
	printf("Ok.\n");
	if (!EMU_VerifyROM()) {
		fprintf(stderr,"ERROR - ROM verification failed.\n");
		return 5;
	}
	EMU_Reset(); // important. missing this will prevent correct start

	// run a serious amount of cycles...
	for (int i=0;i!=5000000;i++)
        EMU_Tick();  // Emulator ticking is separate from the rendering and input
	
	if (cfg->force_level) {
		printf("Mephisto: Force Level %i\n",cfg->force_level);
		EMU_SetLevel(cfg->force_level);
	}

    if (!GFX_Init()) {
        fprintf(stderr,"ERR: Failed to initialize graphics!\n");
        return 2;
    }
	GFX_ResizeCalc();

	if (!SND_Init(cfg->nosound)) {
		fprintf(stderr,"ERR: Failed to initialize sound!\n");
		return 6;
	}

	// set mouse input
	GFX_EnableMouse(cfg->nomouse);

	if (cfg->playblack) {
		printf("Playing with black.\n");
		EMU_SetButton(128); // PLAY press
		for (int i=0;i!=1000000;i++) 
			EMU_Tick();
	} else {
		printf("Playing with white.\n");
	}

	// greeting sound
	SND_Play(0x19);

    uint32_t lastFrameTime = SDL_GetTicks();  // Time tracking for the main loop
	//uint32_t lastEmuTime = SDL_GetTicks(); // Time tracking for emulator realistic mode
	uint32_t lastRefreshTime = SDL_GetTicks()-500;
    while (running) {
        // Get the current time
        uint32_t currentTime = SDL_GetTicks();
        uint32_t deltaTime = currentTime - lastFrameTime;

        // Process events (input) / Input-Handler
		switch (GFX_InputHandler(cfg->playblack)) {
			case -1: 
				running = 0;
				break;
			case 1:
				lastRefreshTime -= 500; // force refresh
				// emulated 0.5s for each keypress (engine processing of input)
				// -> rochade movement requires 3s (=6000000) for processing. Strange but ok...
				// -> rochade queenside requires 6s (=12000000) for processing
				//for (int i=0;i!=12000000;i++) 
				for (int i=0;i!=1000000;i++) 
					EMU_Tick();
		}

        // Emulation Handler (this code snippet is an artefact of the realistic code)
		// 241221: realistic feature discarded // do not use
		//uint32_t tickcount = (currentTime - lastEmuTime)*2000; // 2MHz crystal = 2000 ticks per ms
		//if (cfg->realistic /* && tickcount > 200000 */) { // add tickcount threshold not to overdo it...
		//	for (int i=0;i!=tickcount;i++) {
   		//     	EMU_Tick();  // Emulator ticking is separate from the rendering and input
		//	}
		//	lastEmuTime = currentTime;
		//}

		// emu state handling
		state_cur = EMU_GetGameState();

		// how this works: 
		if (cfg->playblack) { // play black color
			if (STATE_BLACK(state_cur)) {
				uint8_t info_reduction=(EMU_GetRAM(0x1e)==0x40)?(1):(0);
				if (EMU_GetLastMove(-1-info_reduction) == 0x615f || EMU_GetLastMove(-1-info_reduction) == 0x5d5f) { 
					printf("EMU: +++Black Rochade detected. Add artificial delay.\n");
					for (int i=0;i!=12000000;i++) EMU_Tick();
				}
				if (EMU_GetLastMove(-3) == 0x615f) { // black rochade king side -> check if automove is needed
					if (EMU_GetLEDPos() == 0x0101 || EMU_GetLEDPos() == 0x0401) { // rook positions lighthing up
						printf("EMU: Black Rochade Kingside detected.\n");
						EMU_AutoMove();
						for (int i=0;i!=1000000;i++) EMU_Tick();
						EMU_AutoMove();
					}
				} else if (EMU_GetLastMove(-3) == 0x5d5f) { // black rochade queen side -> check if automove is needed
					if (EMU_GetLEDPos() == 0x8001 || EMU_GetLEDPos() == 0x1001) { // rook positions lighting up
						printf("EMU: Black Rochade Queenside detected.\n");
						EMU_AutoMove();
						for (int i=0;i!=1000000;i++) EMU_Tick();
						EMU_AutoMove();
					}
				}
			} else { // state_cur == WHITE
				if (STATE_WHITECALC(state_cur)) {
					for (int i=0;i!=1000000;i++) EMU_Tick();
				}
				// based on LEDPos state perform automove
				if (EMU_GetLEDPos() != 0x0000 && STATE_PLAY(state_cur)) {
					// this includes promotion for black
					EMU_AutoMove();
				}
			}
		} else { // play white color
			if (STATE_WHITE(state_cur)) {
				/*if (STATE_WHITE(state_last)) {
					// do nothing: human thinking
					// ... think ... think ... think.
					
				} else*/ 
				uint8_t info_reduction=(EMU_GetRAM(0x1e)==0x40)?(1):(0);
				if (EMU_GetLastMove(-1-info_reduction) == 0x1b19 || EMU_GetLastMove(-1-info_reduction) == 0x1719) {
					printf("EMU: +++White Rochade Detected. Add artificial delay.\n");
					for (int i=0;i!=12000000;i++) EMU_Tick();
				}
				if (EMU_GetLastMove(-3) == 0x1b19) { // white rochade king side -> check if automove is needed
					if (EMU_GetLEDPos() == 0x8001 || EMU_GetLEDPos() == 0x2001) { // LED for king side rook is on --> automove 
						printf("EMU: White Rochade Kingside Detected.\n");
						EMU_AutoMove();
						// experimental delay for second move
						for (int i=0;i!=1000000;i++) EMU_Tick();
						EMU_AutoMove();
					}
				} else if (EMU_GetLastMove(-3) == 0x1719) { // previous move: white rochade queen side -> check if automove is needed
					if (EMU_GetLEDPos() == 0x0101 || EMU_GetLEDPos() == 0x0801) { // LED for queen side rook is on --> automove 
						printf("EMU: White Rochade Queenside Detected.\n");
						EMU_AutoMove();
						// experimental delay for second move
						for (int i=0;i!=1000000;i++) EMU_Tick();
						EMU_AutoMove();
					}
				}
			} else { // state_cur == BLACK
				// check if black is calculating	
				if (STATE_BLACKCALC(state_cur)) { 
					for (int i=0;i!=1000000;i++) EMU_Tick();
				}
				// based on LEDPos state perform automove
				if (EMU_GetLEDPos() != 0x0000 && STATE_PLAY(state_cur)) {
					// this includes promotion for black
					EMU_AutoMove();
				}
			}
		}
		
		// emu state handling
		if (observer(state_last,state_cur,cfg->playblack)) {
			state_last = state_cur;
		}

		// LED Timer Handling (500ms for animations)
		ledtimer = (SDL_GetTicks()/500)%2;
		if (ledtimer != last_ledtimer) {
			last_ledtimer = ledtimer;
		}

        // Update graphics (draw the board, figures, etc.)
		if (currentTime - lastRefreshTime > 500) {
        	GFX_DrawBoard(cfg->playblack);  // Drawing the chess board and figures
			GFX_DrawLEDs(ledtimer); // refresh LEDs on the board
        	GFX_Update();     // Presenting the final frame to the screen
			lastRefreshTime = SDL_GetTicks();
		}

        // Frame rate control (limit to target FPS)
        if (deltaTime < FRAME_TIME) {
            SDL_Delay(FRAME_TIME - deltaTime);  // Delay only if the frame completed too quickly
        }

        lastFrameTime = SDL_GetTicks();  // Update frame time for the next loop
    }

    // Clean up resources
    EMU_Destroy();
	SND_Destroy();
    GFX_Destroy();
    return 0;
}

