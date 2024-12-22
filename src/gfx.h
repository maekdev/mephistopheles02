#ifndef __GFX_H
#define __GFX_H

// DEFINES
#define TARGET_FPS 20
#define FRAME_TIME (1000 / TARGET_FPS)  // time per frame in ms

// PROTOTYPES
int GFX_Init(void);
int GFX_Destroy(void);
int GFX_InputHandler(int playblack);
int GFX_Update(void);
int GFX_DrawLEDs(int state);
int GFX_DrawBoard(int playblack);
int GFX_EnableMouse(int nomouse);

// work in progress
void GFX_ResizeCalc(void);


int  SND_Init(uint8_t nosound);
void SND_Play(uint8_t t);
int  SND_Destroy(void);

#endif // __GFX_H
