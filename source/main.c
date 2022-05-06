#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <3ds.h>
#include <string.h>
#include <sys/stat.h>
#include "archive.h"
#define FB_SIZE 0x400000
#define MIN_PC 0
#define MAX_PC 300
u8 *fb;
Result readgamecoin() {
	memset(fb, 0, FB_SIZE);
	Result ret = archive_readfile(GameCoin_Extdata, "/gamecoin.dat", fb, 0x14);
	if (ret != 0) {
		printf("Failed to read file, %08x\n", (unsigned int) ret);
		return ret;
	}
	return 0;
}
Result writegamecoin() {
	Result ret = archive_writefile(GameCoin_Extdata, "/gamecoin.dat", fb, 0x14);
	if (ret != 0) {
		printf("Failed to write file, %08x\n", (unsigned int) ret);
		return ret;
	}
	return 0;
}
u16 getcoins() {
	Result ret = readgamecoin();
	if (ret != 0) return 0xffff;
	return (u16)fb[5] << 8 | fb[4];
}
Result setcoins(u16 count) {
	Result ret = readgamecoin();
	if (ret != 0) return ret;
	fb[4] = (u8)(count & 0xff);
	fb[5] = (u8)(count >> 8);
	ret = writegamecoin();
	if (ret != 0) return ret;
	return 0;
}
int main(int argc, char* argv[]) {
	gfxInitDefault();
	consoleInit(GFX_TOP, NULL);
	fb = (u8*)malloc(0x400000);
	u16 pc;
	u16 pc_;
	bool success = false;
	if (fb == NULL) {
		printf("Failed to allocate memory\n");
	} else {
		memset(fb, 0, FB_SIZE);
		Result ret = open_extdata();
		if (ret == 0) success = true;
		else printf("\n");
	}
	if (success) printf("Opened extdata\n");
	else printf("Press START to exit\n");
	bool change = true;
	if (success) {
		pc = getcoins();
		pc_ = pc;
		if (pc == 0xffff) success = false;
	}
	while (aptMainLoop()) {
		gspWaitForVBlank();
		gfxSwapBuffers();
		hidScanInput();
		u32 kDown = hidKeysDown();
		if (success) {
#define DECREASE(x) { change = true; pc_ = pc_ > MIN_PC + x ? pc_ - x : MIN_PC; }
#define INCREASE(x) { change = true; pc_ = pc_ < MAX_PC - x ? pc_ + x : MAX_PC; }
			if (kDown & KEY_DOWN ) DECREASE(1);
			if (kDown & KEY_UP   ) INCREASE(1);
			if (kDown & KEY_LEFT ) DECREASE(25);
			if (kDown & KEY_RIGHT) INCREASE(25);
			if (kDown & KEY_B) {
				change = true;
				pc_ = pc;
			}
			if (kDown & KEY_Y) {
				
			}
			if (kDown & KEY_A) {
				change = true;
				if (pc_ != pc) {
					Result ret = setcoins(pc_);
					if (ret == 0) {
						pc = pc_;
					} 
				}
			}
		}
		if (kDown & KEY_START) {
			consoleClear();
			break;
		}
		if (success && change) {
			change = false;
			consoleClear();
			printf("Current Play Coins: %i\n", pc);
			printf("    New Play Coins: %i\n\n", pc_);
			printf("    A = Set Play Coins\n    B = Revert\nStart = Exit\n    Y = System Number Selection\n");
			printf(" Down = -1\n   Up = +1\n Left = -25\nRight = +25\n");
		}
	}
	free(fb);
	close_extdata();
	gfxExit();
	return 0;
}
