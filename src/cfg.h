#ifndef __CFG_H
#define __CFG_H

// TYPEDEF
typedef struct {
	int force_quit;
	int playblack;
	char romfile[_MAX_PATH];
	int force_level;
	int nosound;
	int noautomove;
	int realistic;
	int uci;
	int forcegfx;
	int nomouse;
} Config;

// PROTOTYPES
int CFG_Init(void);
Config* CFG_GetConfig(void);
void CFG_ParseArgs(int argc,char *argv[]);

#endif // __CFG_H
