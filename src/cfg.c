// cfg.c
// 
// 241020
// markus ekler
// 
// cfg handler (default config & cfg management)
//

// INCLUDE
#include "config.h"

#include "cfg.h"

// VARS
Config cfg;

// FUNCTIONS
int CFG_Init(void) {
	cfg.force_quit = 0; // exit program - only needed for _usage
	strcpy(cfg.romfile,""); // filename of romfile
	cfg.force_level = 0; // 0..8 - 0=start with default level
	cfg.nosound = 0;
	cfg.noautomove = 0;
	//warning realistic mode not yet implemented
	//cfg.realistic = 0;
	cfg.uci = 0; // uci mode
	cfg.forcegfx = 0; // force board drawing in uci mode
	cfg.nomouse = 0; // disable mouse support (can be annoying when playing with keyboard)

	return 1;
}

Config *CFG_GetConfig(void) {
	return &cfg;
}

void _usage(char *fn) {
	printf("                      _     _     _       ____  \n");
	printf(" _ __ ___   ___ _ __ | |__ (_)___| |_ ___|___ \\\n"); 
	printf("| '_ ` _ \\ / _ \\ '_ \\| '_ \\| / __| __/ _ \\ __) |\n");
	printf("| | | | | |  __/ |_) | | | | \\__ \\ || (_) / __/ \n");
	printf("|_| |_| |_|\\___| .__/|_| |_|_|___/\\__\\___/_____|\n");
	printf("               |_|                              \n");
	printf("LUCIFER S LIFE\n");
	printf("\n\nUsage:\n%s [OPTION] [ROM]\n",fn);
	printf("  Option:\n");
	printf("   -leveln    - start with levelx e.g. -level2 with n=1..8\n");
	printf("   -black     - play as black (default is white)\n");
	printf("   -nosound   - disable sound output\n");
	printf("   -noauto    - disable automove\n");
	printf("   -realistic - play with realtime emulation\n");
	printf("   -uci       - UCI game mode\n");
	printf("   -forcegfx  - force board display in UCI mode\n");
	printf("   -nomouse   - disable mouse control\n");
	printf("   -h / -?  - this screen\n");
}

void CFG_ParseArgs(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
		printf("argv[i]=%s\n",argv[i]);
		if (strcmp(argv[i],"-level1") == 0) {
			cfg.force_level = 1;
		} else if (strcmp(argv[i],"-level2") == 0) {
			cfg.force_level = 2;
		} else if (strcmp(argv[i],"-level3") == 0) {
			cfg.force_level = 3;
		} else if (strcmp(argv[i],"-level4") == 0) {
			cfg.force_level = 4;
		} else if (strcmp(argv[i],"-level5") == 0) {
			cfg.force_level = 5;
		} else if (strcmp(argv[i],"-level6") == 0) {
			cfg.force_level = 6;
		} else if (strcmp(argv[i],"-level7") == 0) {
			cfg.force_level = 7;
		} else if (strcmp(argv[i],"-level8") == 0) {
			cfg.force_level = 8;
		} else if (strcmp(argv[i],"-black") == 0) {
			cfg.playblack = 1;
		} else if (strcmp(argv[i],"-playblack") == 0) {
			cfg.playblack = 1;
		} else if (strcmp(argv[i],"-nosound") == 0) {
			cfg.nosound = 1;
		} else if (strcmp(argv[i],"-noauto") == 0) {
			cfg.noautomove = 1;
		} else if (strcmp(argv[i],"-noautomove") == 0) {
			cfg.noautomove = 1;
		// 241221 ME: realistic feature discontinued
		//} else if (strcmp(argv[i],"-realistic") == 0) {
		//	cfg.realistic = 1;
		} else if (strcmp(argv[i],"-uci") == 0) {
			cfg.uci = 1;
		} else if (strcmp(argv[i],"-forcegfx") == 0) {
			cfg.forcegfx = 1;
		} else if (strcmp(argv[i],"-nomouse") == 0) {
			cfg.nomouse = 1;
		} else if (strcmp(argv[i],"-h") == 0 || strcmp(argv[i],"-?") == 0) {
			cfg.force_quit = 1;
			_usage(argv[0]);
		} else {
			// unknown string so this must be the rom file
			strcpy(cfg.romfile,argv[i]);
			printf("Setting romfilename to %s\n",argv[i]);
		}
    }
}
