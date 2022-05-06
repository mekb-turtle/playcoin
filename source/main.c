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
#define LEN_PC 6
#define RED     "\x1b[1;31m"
#define GREEN   "\x1b[1;32m"
#define YELLOW  "\x1b[1;33m"
#define BLUE    "\x1b[1;34m"
#define MAGENTA "\x1b[1;35m"
#define CYAN    "\x1b[1;36m"
#define RESET   "\x1b[0m"
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
SwkbdCallbackResult swCb(void *user, const char **msg, const char *text, unsigned int len) {
	u16 v = atoi(text) & 0xffff;
	bool f = false;
	char* m = malloc(100);
	if (v < MIN_PC) { f = true; sprintf(m, "Cannot be less than %i", MIN_PC); }
	if (v > MAX_PC) { f = true; sprintf(m, "Cannot be greater than %i", MAX_PC); }
	if (f) { *msg = m; return SWKBD_CALLBACK_CONTINUE; }
	return SWKBD_CALLBACK_OK;
}
int main(int argc, char* argv[]) {
	gfxInitDefault();
	consoleInit(GFX_TOP, NULL);
	fb = (u8*)malloc(FB_SIZE);
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
		if (kDown & KEY_START) {
			consoleClear();
			break;
		}
		if (success) {
#define DECREASE(x) { change = true; pc_ = pc_ > MIN_PC + x ? pc_ - x : MIN_PC; }
#define INCREASE(x) { change = true; pc_ = pc_ < MAX_PC - x ? pc_ + x : MAX_PC; }
			if (kDown & KEY_DOWN ) DECREASE(1);
			if (kDown & KEY_UP   ) INCREASE(1);
			if (kDown & KEY_LEFT ) DECREASE(25);
			if (kDown & KEY_RIGHT) INCREASE(25);
			bool a = false;
			if (change) {
			} else if (kDown & KEY_B) {
				if (pc_ != pc) {
					change = true;
					pc_ = pc;
				}
			} else if (kDown & KEY_Y) {
				SwkbdState sw;
				swkbdInit(&sw, SWKBD_TYPE_NUMPAD, 2, 3);
				swkbdSetFeatures(&sw, SWKBD_FIXED_WIDTH | SWKBD_DARKEN_TOP_SCREEN
					| SWKBD_ALLOW_HOME | SWKBD_ALLOW_RESET | SWKBD_ALLOW_POWER);
				swkbdSetValidation(&sw, SWKBD_NOTBLANK_NOTEMPTY, 0, 0);
				swkbdSetButton(&sw, SWKBD_BUTTON_LEFT, "Cancel", false);
				swkbdSetButton(&sw, SWKBD_BUTTON_RIGHT, "OK", true);
				char* t = malloc(LEN_PC);
				char* t_ = malloc(LEN_PC);
				sprintf(t_, "%i", pc_);
				swkbdSetInitialText(&sw, t_);
				swkbdSetHintText(&sw, t_);
				swkbdSetFilterCallback(&sw, swCb, NULL);
				SwkbdButton button = SWKBD_BUTTON_NONE;
				button = swkbdInputText(&sw, t, LEN_PC);
				if (button == SWKBD_BUTTON_RIGHT) {
					if (swkbdGetResult(&sw) == SWKBD_D1_CLICK1) {
						u8 v = atoi(t) & 0xffff;
						if (pc_ != v) {
							change = true;
							pc_ = v;
						}
						if (pc_ != pc) {
							change = true;
							if (setcoins(pc_) == 0) pc = v;
						}
					}
				}
			} else a = true;
			u16 pc1 = pc_ == MAX_PC ? MIN_PC : MAX_PC;
			//pc_ < ((MAX_PC-MIN_PC)>>1) + MIN_PC ? MAX_PC : MIN_PC;
			if (a && (kDown & KEY_A || kDown & KEY_X)) {
				if (kDown & KEY_X) {
					change = true;
					pc_ = pc1;
				}
				if (pc_ != pc) {
					change = true;
					if (setcoins(pc_) == 0) pc = pc_;
				}
			}
			if (change) {
				change = false;
				consoleClear();
				printf("Current Play Coins: %s%i%s\n", CYAN, pc, RESET);
				printf("    New Play Coins: %s%i%s\n\n", CYAN, pc_, RESET);
				printf("%s    A%s = Set Play Coins\n", RED, RESET);
				printf("%s    B%s = Revert\n", YELLOW, RESET);
				printf("%s    Y%s = System Number Selection\n", GREEN, RESET);
				printf("%s    X%s = Set to %i\n", BLUE, RESET, pc1);
				printf("Start = Exit\n");
				printf(" Down = -1\n");
				printf("   Up = +1\n");
				printf(" Left = -25\n");
				printf("Right = +25\n");
			}
		}
	}
	free(fb);
	close_extdata();
	gfxExit();
	return 0;
}
