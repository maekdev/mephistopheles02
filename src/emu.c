// emu.c
//
// 241007
// markus ekler
//
// emulator abstraction layer
// change in Makefile to select emulator engine
// experimental: VREMU restore savegame implementation

// INCLUDES
#include "config.h"

#include <sys/stat.h>
#include "miniz.h"
#include "md5.h"

#include "emu.h"
#ifdef _BUILTINROM
#include "rom.h"
#endif // BUILTINROM

// VARIABLES
uint8_t mem[0x10000]; // change ram to mem --> rom + ram
uint8_t m_input_mux = 0;
uint8_t m_mux = 0;
//uint8_t m_dac = 0;
int cursor_sel = -1;
int cursor_timer = 0;
// internal handling of button press: automatic release after n cycles
#define CURSOR_TIMEOUT			500000 // experimental number should work...
#define SET_CURSORTIMER()		(cursor_timer = CURSOR_TIMEOUT)
#define GET_CURSORTIMEOUT()		(cursor_timer == 0)
#define HANDLE_TIMER()			((cursor_timer > 0)?(cursor_timer--):(cursor_timer))

// INTERNAL READ AND WRITE FUNCTIONS
uint8_t _read(uint16_t addr) {
	// mem organization: 
	//		0x0000..0x0800 - 2k RAM 
	//			
	//      0x8000..0xFFFF - 32k ROM
	int offset = addr-0x3000;
	int i=0;
	// return ROM code for upper memory aray
	if (addr >= 0x8000) {
		//return rom[addr & 0x7FFF];
		return mem[addr & 0xFFFF];
	}
	// return RAM & I/O
	if (addr < 0x800) {
		//return ram[addr];
		return mem[addr];
	} else if (addr >= 0x3000 && addr <= 0x3007) {
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
	return 0x00;
}

void _write(uint16_t addr,uint8_t val) {
	if (addr == 0x2000) {
		m_input_mux = val ^ 0xFF;
		/*if (m_dac != (val & 0x80)) {
			DAC_trigger(val&0x80);
		}j
		m_dac = val & 0x80;*/
	} else if (addr == 0x2800) {
		m_mux = val;
	} else if (addr < 0x8000) {
		//ram[addr] = val;	
		mem[addr] = val;
	}
}


// EMULATOR DEPENDENT IMPLEMENTATION / ABSTRACTION
#ifdef _VREMU6502
#include "vrEmu6502.h"

// EMULATOR INSTANCE
VrEmu6502 *my6502 = NULL;
vrEmu6502Interrupt *nmipin = NULL;

// helper functions to comply with vrEmuFormat
uint8_t memory_read(uint16_t addr,bool isDbg) {
	return _read(addr);
}

int EMU_Init(void) {
	my6502 = vrEmu6502New(CPU_65C02,memory_read,_write);
	if (my6502) {
		nmipin = vrEmu6502Nmi(my6502);
		vrEmu6502Reset(my6502);
		return 1;
	}
	return 0;
}

int EMU_Reset(void) {
	vrEmu6502Reset(my6502);
	return 1;
}

int EMU_Tick(void) {
	static int nmi_timer = 1;
	vrEmu6502Tick(my6502);

	HANDLE_TIMER();
	if (GET_CURSORTIMEOUT()) {
		cursor_sel = -1; // automatic reset of cursor
	}
	
	nmi_timer--;
	if (nmi_timer <= 0) {
		*nmipin = 0; // set nmi pin
		nmi_timer = 4096;
	}
	return 1;
}

int EMU_SaveState(char *fn) {
	// savestate file format:
	// 2kB RAM
	// PC (uint16_t)
	// AC (uint8_t)
	// IX (uint8_t)
	// IY (uint8_t)
	// SP (uint8_t)
	// FLAGS (uint8_t)
	FILE *f;
	f = fopen(fn,"wb");
	if (f == NULL) {
		printf("ERR: EMU_SaveState - fopen failed.\n");
		return 0;
	}
	//fwrite(ram,sizeof(uint8_t),2048,f);
	fwrite(mem,sizeof(uint8_t),2048,f);
	uint16_t pc = vrEmu6502GetPC(my6502);
	fwrite(&pc,sizeof(uint16_t),1,f);
	uint8_t acc = vrEmu6502GetAcc(my6502);
	fwrite(&acc,sizeof(uint8_t),1,f);
	uint8_t ix = vrEmu6502GetX(my6502);
	fwrite(&ix,sizeof(uint8_t),1,f);
	uint8_t iy = vrEmu6502GetY(my6502);
	fwrite(&iy,sizeof(uint8_t),1,f);
	uint8_t sp = vrEmu6502GetStackPointer(my6502); // sp
	fwrite(&sp,sizeof(uint8_t),1,f);
	uint8_t flags = vrEmu6502GetStatus(my6502); // flags
	fwrite(&flags,sizeof(uint8_t),1,f);

	fclose(f);
	printf("EMU_SaveState(): %s successfully written.\n",fn);

	return 1;	
}

int EMU_RestoreState(char *fn) {
	FILE *f;
	int i;
	uint16_t pc;
	uint8_t acc;
	uint8_t ix;
	uint8_t iy;
	uint8_t sp;
	uint8_t backup[64];
	f = fopen(fn,"rb");
	if (f == NULL) {
		printf("ERR: EMU_RestoreState - fopen failed.\n");
		return 0;
	}
	fread(mem,sizeof(uint8_t),2048,f);
	fread(&pc,sizeof(uint16_t),1,f);
	fread(&acc,sizeof(uint8_t),1,f);
	fread(&ix,sizeof(uint8_t),1,f);
	fread(&iy,sizeof(uint8_t),1,f);
	fread(&sp,sizeof(uint8_t),1,f);

	fclose(f);

	// we use assembler code to set a,x,y,sp as vrEmu does not allow for direct access to these registers; stored in RAM afterwards restore RAM
	uint8_t opcodes[] = { 0xa2, 0xff, 0x9a, 0xa2, 0xff, 0xa0, 0xff, 0xa9, 0xff, 0x4c, 0x09, 0x00 , 0,0,0 };
	//uint8_t opcodes[] = { 0x4c, 0x09, 0x00 , 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
	// save backup of opcodes
	for (i=0;i!=64;i++) {
		backup[i] = mem[0x0000+i];
	}

	// SP: A2 <val> 9A = LDX <val>; TXS (transfer x to sp)
	// X : A2 <val>; LDX
	// Y : A0 <val>; LDY
	// A : A9 <val>; LDA
	// endless loop: JUMP 0x00f0; 4C 09 00
	opcodes[1] = sp;
	opcodes[4] = ix;
	opcodes[6] = iy;
	opcodes[8] = acc;
	
	for (i=0;i!=12;i++) 
		mem[0x0000+i] = opcodes[i];

	vrEmu6502SetPC(my6502,0x0000);
	for (i=0;i!=24;i++) {
		vrEmu6502Tick(my6502);
		/* Debug: 
		printf("%04x: %02x\n",vrEmu6502GetCurrentOpcodeAddr(my6502),vrEmu6502GetCurrentOpcode(my6502));
		printf("SP: %02x\n",vrEmu6502GetStackPointer(my6502));
		*/
	}
	printf("PC: %04x\n",vrEmu6502GetPC(my6502));

	// restore opcodes from backup
	for (i=0;i!=64;i++) 
		mem[0x0000+i] = backup[i];
	// restore pc
	vrEmu6502SetPC(my6502,pc);

	printf("EMU_RestoreState(): %s successfully restored.\n",fn);
	/* Debug 
	printf("PC=0x%04x (^=0x%04x)\n",pc,vrEmu6502GetPC(my6502));
	printf("X =0x%02x (^=0x%02x)\n",ix,vrEmu6502GetX(my6502));
	printf("Y =0x%02x (^=0x%02x)\n",iy,vrEmu6502GetY(my6502));
	printf("A =0x%02x (^=0x%02x)\n",acc,vrEmu6502GetAcc(my6502));
	printf("SP=0x%02x (^=0x%02x)\n",sp,vrEmu6502GetStackPointer(my6502));
	*/
	
	return 1;
}

void EMU_Destroy(void) {
	vrEmu6502Destroy(my6502);
	my6502 = NULL;
}

#endif // _VREMU6502

#ifdef _M6502
#include "m6502.h"

// PROTOTYPES
m6502 cpu;

uint8_t rb(void *userdata,uint16_t addr) {
	// mem organization: 
	//		0x0000..0x7FFF - 32k RAM (only 2k for mephisto)
	//      0x8000..0xFFFF - 32k ROM
	return _read(addr);
}

void wb(void* userdata,uint16_t addr,uint8_t val) {
	_write(addr,val);
}

int EMU_Init(void) {
	m6502_init(&cpu);
	cpu.read_byte = &rb;
	cpu.write_byte = &wb;
	cpu.pc = 0xFFFC;
	cpu.m65c02_mode = 1;
	m6502_gen_res(&cpu);
	return 1;
}

int EMU_Reset(void) {
	m6502_gen_res(&cpu);
	return 1;
}


int EMU_Tick(void) {
	static int nmi_timer = 1;
	m6502_step(&cpu);

	HANDLE_TIMER();
	if (GET_CURSORTIMEOUT()) {
		cursor_sel = -1; // automatic reset of cursor
	}
	
	nmi_timer--;
	if (nmi_timer <= 0) {
		m6502_gen_nmi(&cpu);
		nmi_timer = 4096;
	}
	return 1;
}

void EMU_Destroy(void) {

}

int EMU_SaveState(char *fn) {
	// savestate file format:
	// 2kB RAM
	// PC (uint16_t)
	// AC (uint8_t)
	// IX (uint8_t)
	// IY (uint8_t)
	// SP (uint8_t)
	// FLAGS (uint8_t)
	FILE *f;
	f = fopen(fn,"wb");
	if (f == NULL) {
		printf("ERR: EMU_SaveState - fopen failed.\n");
		return 0;
	}
	fwrite(mem,sizeof(uint8_t),2048,f);
	/*fwrite(&(cpu.pc),sizeof(uint16_t),1,f);
	fwrite(&(cpu.a),sizeof(uint8_t),1,f);
	fwrite(&(cpu.x),sizeof(uint8_t),1,f);
	fwrite(&(cpu.y),sizeof(uint8_t),1,f);
	fwrite(&(cpu.sp),sizeof(uint8_t),1,f);
	*/
	fwrite(&(cpu),sizeof(cpu),1,f);

	fclose(f);
	printf("EMU_SaveState(): %s successfully written.\n",fn);

	return 1;	
}

int EMU_RestoreState(char *fn) {
	FILE *f;
	f = fopen(fn,"rb");
	if (f == NULL) {
		printf("ERR: EMU_RestoreState - fopen failed.\n");
		return 0;
	}
	
	fread(mem,sizeof(uint8_t),2048,f);
	/*fread(&(cpu.pc),sizeof(uint16_t),1,f);
	fread(&(cpu.a),sizeof(uint8_t),1,f);
	fread(&(cpu.x),sizeof(uint8_t),1,f);
	fread(&(cpu.y),sizeof(uint8_t),1,f);
	fread(&(cpu.sp),sizeof(uint8_t),1,f);
	*/
	fread(&(cpu),sizeof(cpu),1,f);

	fclose(f);

	// one specialty for m6502: refresh local address to prevent segfaults
	cpu.read_byte = &rb;
	cpu.write_byte = &wb;

	printf("EMU_RestoreState(): %s successfully restored.\n",fn);
	return 1;
}


#endif // _M6502


// COMMON FUNCTIONS
uint8_t EMU_GetRAM(uint16_t addr) {
	return mem[addr];
}

void EMU_SetButton(int b) {
	cursor_sel = b;
	SET_CURSORTIMER();
}

uint16_t EMU_GetGameState(void) {
	// this is a helper function, analyzing memory content for current gamestate
	// LED info @ 0x0004 & 0x0005
	// 8181 - white's turn / waiting for human
	// 8101 - white's turn / calculating
	// 4101 - black's turn / calculating
	// 4141 - black's turn / calculation done / waiting for human to move the pieces (=0x0001-0x0003 contains board position / led coding)
	// 9181 --> 0x0004 & 0x10 --> ERR lighting

	/* too complicated
	if (ram[0x0004] == 0x81 && ram[0x0005] == 0x81) {
		return STATE_WHITE;
	}
	if (ram[0x0004] == 0x81 && ram[0x0005] == 0x01) {
		return STATE_WHITECALC;
	}
	if (ram[0x0004] == 0x41 && ram[0x0005] == 0x01) {
		return STATE_BLACKCALC;
	}
	if (ram[0x0004] == 0x41 && ram[0x0005] == 0x41) {
		return STATE_BLACK;
	}
	*/
	//if (ram[0x0004] & 0x80) {
	/*if (mem[0x0004] & 0x80) {
		return STATE_WHITE;
	}
	//if (ram[0x0004] & 0x40) {
	if (mem[0x0004] & 0x40) {
		return STATE_BLACK;
	}
	return STATE_INVALID;
	*/
	uint16_t ret = 0;
	ret = (mem[0x0004]<<8);
	ret |= mem[0x0005];
	return ret;
}

void EMU_SetLevel(int lev) {
	// super unsophisticated level setting: wait 2s, push the buttons & done
	int i;
	for (i=0;i!=4000000;i++) {
		EMU_Tick();
	}
	EMU_SetButton(128+5); // LEV press
	for (i=0;i!=1000000;i++) {
		EMU_Tick();
	}
	EMU_SetButton(128+8+((lev-1)%8)); // number press
	for (i=0;i!=1000000;i++) {
		EMU_Tick();
	}
	EMU_SetButton(128+6); // ENT press
}

uint8_t LED2Coord(uint8_t c) {
	switch (c) {
		case 0x01: return 0;
		case 0x02: return 1;
		case 0x04: return 2;
		case 0x08: return 3;
		case 0x10: return 4;
		case 0x20: return 5;
		case 0x40: return 6;
		case 0x80: return 7;
	}
	return 0xFF; // error should never be reached
}


int EMU_AutoMove(void) {
	if (mem[0x0000] != 0x00 && mem[0x0002] != 0x00) {
		uint8_t x = LED2Coord(mem[0x0000]);
		uint8_t y = LED2Coord(mem[0x0002]);
		EMU_SetButton(8*y+x);
		// experimental value of 700k found to work ok / better -> check for LED change, but works for now
		for (int i=0;i!=700000;i++) EMU_Tick();
		return 1;
	}
	return 0;
}

int EMU_GetLED(int i,int state) {
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

int EMU_LoadROM(char *fn) {
	struct stat buffer;
	mz_zip_archive zip_archive;
	FILE *f;

	char romfiles[6][_MAX_PATH] = {
		"", // placeholder for default
		"mondial2.zip",
		"mondial2.bin",
		"rom.bin",
		"rom/mondial2.zip",
		"rom/rom.bin"
	};
	// copy default from config
	strcpy(romfiles[0],fn);
	memset(&zip_archive,0,sizeof(zip_archive));
	// check each entry
	for (int i=0;i!=6;i++) {
		printf("  testing %s\n",romfiles[i]);
		if (stat(romfiles[i],&buffer) == 0) { // file exists let's try to open and load
			if (strstr(romfiles[i],".zip")) {
				// open zip archive
				if (!mz_zip_reader_init_file(&zip_archive,romfiles[i],0)) {
					printf("  not found %s\n",romfiles[i]);
					continue;
				}
				int filecount = mz_zip_reader_get_num_files(&zip_archive);
				// check each file if it could be the ROM
				for (int o=0;o!=filecount;o++) {
					mz_zip_archive_file_stat file_stat;
					if (!mz_zip_reader_file_stat(&zip_archive,0,&file_stat)) {
						continue;
					}
					if (file_stat.m_uncomp_size == 0x8000) { // file with 32kB found
						if (mz_zip_reader_extract_to_mem(&zip_archive,o,mem+0x8000,0x8000,0)) {

							mz_zip_reader_end(&zip_archive);
							return 1;
						}
					}
				}

				// close zip archive
				mz_zip_reader_end(&zip_archive);
			} else {
				f = fopen(romfiles[i],"rb");
				if (f == NULL) {
					continue;
				}
				fread(mem+0x8000,sizeof(uint8_t),0x8000,f);
				fclose(f);		
				return 1;
			}
		}
	}
	// not successful for external files not try to load builtin rom
	#ifdef __ROM_H
	printf("Loading default ROM from builtin memory...\n");
	for (int i=0;i!=0x8000;i++) {
		mem[0x8000+i] = rom[i];
	}
	return 1;
	#endif // __ROM_H

	return 0;
}

int EMU_VerifyROM(void) {
	// ebba8cb5 84984f3e cc07462d a8693dc5  mondial2.bin
	uint8_t md5result[16]= { 
		0xeb,0xba,0x8c,0xb5,
		0x84,0x98,0x4f,0x3e,
		0xcc,0x07,0x46,0x2d,
		0xa8,0x69,0x3d,0xc5 };
	uint8_t match = 1;

	MD5Context ctx;
	md5Init(&ctx);
	md5Update(&ctx,mem+0x8000,0x8000);
	md5Finalize(&ctx);
	
	printf("ROM file MD5 hash: ");
	for (int i=0;i!=16;i++) {
		printf("%02x",(ctx.digest)[i]);
		if (md5result[i] != (ctx.digest)[i]) 
			match = 0;
	}
	printf("\n");

	if (match) {
		printf("EMU_VerifyROM: valid ROM binary detected.\n");
		return 1;
	}
	
	printf("EMU_VerifyROM: hash mismatch / unknown ROM binary.\n");

	return 0;
}

uint16_t EMU_GetLastMove(int16_t offset) {
	// this returns the hist memory position of offset
	// HighByte = dest
	// LowByte = src
	int16_t index=mem[0x6c]+offset;//EMU_GetRAM(0x6c);
	uint16_t ret = mem[0x570+index];
	ret = (ret << 8);
	ret |= (mem[0x4b0+index]);
	return ret;
}

uint16_t EMU_GetLEDPos(void) {
	// this function returns current LED status
	// HighByte = x
	// LowByte = y
	uint16_t ret = mem[0x0000] << 8;
	ret |= mem[0x0002];
	return ret;
}
