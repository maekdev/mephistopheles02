#ifndef __EMU_H
#define __EMU_H

// DEFINITION / TYPES
/*typedef enum {
	STATE_OFF,
	STATE_WHITE,
	STATE_WHITECALC,
	STATE_BLACK,
	STATE_BLACKCALC,
	STATE_INVALID
} GameState;
*/
#define STATE_WHITE(s) 			(s&0x8000) // including WHITE & WHITECALC
#define STATE_BLACK(s)			(s&0x4000)
#define STATE_WHITECALC(s) 		((s&0x8080)==0x8000)
#define STATE_BLACKCALC(s) 		((s&0x4040)==0x4000)
#define STATE_PLAY(s)			(s&0x0101)

// PROTOTYPES
int EMU_Init(void);
int EMU_Reset(void);
int EMU_Tick(void);
void EMU_Destroy(void);

int EMU_SaveState(char *fn);
int EMU_RestoreState(char *fn);

void EMU_SetButton(int b);
uint16_t EMU_GetGameState(void);
void EMU_SetLevel(int lev);
int EMU_AutoMove(void);
int EMU_GetLED(int i,int state);

uint8_t EMU_GetRAM(uint16_t addr);

int EMU_LoadROM(char *fn);
int EMU_VerifyROM(void);

uint16_t EMU_GetLastMove(int16_t offset);

uint16_t EMU_GetLEDPos(void);

#endif // __EMU_H
