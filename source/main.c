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
#define BLACK        "\x1b[2;30m"
#define DARK_RED     "\x1b[2;31m"
#define DARK_GREEN   "\x1b[2;32m"
#define DARK_YELLOW  "\x1b[2;33m"
#define DARK_BLUE    "\x1b[2;34m"
#define DARK_MAGENTA "\x1b[2;35m"
#define DARK_CYAN    "\x1b[2;36m"
#define GRAY         "\x1b[2;37m"
#define DARK_GRAY    "\x1b[1;30m"
#define RED          "\x1b[1;31m"
#define GREEN        "\x1b[1;32m"
#define YELLOW       "\x1b[1;33m"
#define BLUE         "\x1b[1;34m"
#define MAGENTA      "\x1b[1;35m"
#define CYAN         "\x1b[1;36m"
#define RESET        "\x1b[0m"
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
	bool kDOWN  = false;
	bool kUP    = false;
	bool kLEFT  = false;
	bool kRIGHT = false;
	bool rDOWN  = false;
	bool rUP    = false;
	bool rLEFT  = false;
	bool rRIGHT = false;
	u8 cDOWN  = 0;
	u8 cUP    = 0;
	u8 cLEFT  = 0;
	u8 cRIGHT = 0;
	while (aptMainLoop()) {
		gspWaitForVBlank();
		gfxSwapBuffers();
		hidScanInput();
		u32 kDown = hidKeysDown();
		u32 kHeld = hidKeysHeld();
		u32 kUp   = hidKeysUp();
		if (kDown & KEY_START) {
			consoleClear();
			break;
		}
		if (success) {
			bool b = true;
#define CLEAR(k) (k&KEY_DOWN?1:0)+(k&KEY_UP?1:0)+(k&KEY_LEFT?1:0)+(k&KEY_RIGHT?1:0)>1
			if (CLEAR(kHeld)) kDOWN=kUP=kLEFT=kRIGHT=false;
			if (CLEAR(kDown)) b = kDOWN=kUP=kLEFT=kRIGHT=false;
#undef CLEAR
			if (b) {
#define SET(k, K) (kUp&K ? false : kDown&K ? true : k)
				kDOWN  = SET(kDOWN , KEY_DOWN );
				kUP    = SET(kUP   , KEY_UP   );
				kLEFT  = SET(kLEFT , KEY_LEFT );
				kRIGHT = SET(kRIGHT, KEY_RIGHT);
#undef SET
#define REPEAT_AT 40
#define REPEAT_TO 30
			if (kDOWN ) { if (++cDOWN  >= REPEAT_AT) { cDOWN  = REPEAT_TO; rDOWN  = true; }} else { cDOWN  = 0; rDOWN  = false; }
			if (kUP   ) { if (++cUP    >= REPEAT_AT) { cUP    = REPEAT_TO; rUP    = true; }} else { cUP    = 0; rUP    = false; }
			if (kLEFT ) { if (++cLEFT  >= REPEAT_AT) { cLEFT  = REPEAT_TO; rLEFT  = true; }} else { cLEFT  = 0; rLEFT  = false; }
			if (kRIGHT) { if (++cRIGHT >= REPEAT_AT) { cRIGHT = REPEAT_TO; rRIGHT = true; }} else { cRIGHT = 0; rRIGHT = false; }
#define DECREASE(x) { change = true; pc_ = pc_ > MIN_PC + x ? pc_ - x : MIN_PC; }
#define INCREASE(x) { change = true; pc_ = pc_ < MAX_PC - x ? pc_ + x : MAX_PC; }
#define DOWN(c, r) (r ? c == REPEAT_TO : c == 2)
			if (DOWN(cDOWN , rDOWN )) DECREASE(1);
			if (DOWN(cUP   , rUP   )) INCREASE(1);
			if (DOWN(cLEFT , rLEFT )) DECREASE(25);
			if (DOWN(cRIGHT, rRIGHT)) INCREASE(25);
#undef REPEAT_AT
#undef REPEAT_TO
#undef DECREASE
#undef INCREASE
			}
			if (change) {
			} else if (kDown & KEY_B) {
				if (pc_ != pc) {
					change = true;
					pc_ = pc;
				}
			} else if (kDown & KEY_Y || kDown & KEY_A || kDown & KEY_X) {
				if (kDown & KEY_Y) {
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
						}
					}
				} else if (kDown & KEY_X) {
					change = true;
					pc_ = pc_ == MAX_PC ? MIN_PC : MAX_PC;
				}
				if (pc_ != pc) {
					change = true;
					if (setcoins(pc_) == 0) pc = pc_;
				}
			}
			if (change) {
				change = false;
				consoleClear();
				printf("Current Play Coins%s: %s%i%s\n", DARK_GRAY, CYAN, pc, RESET);
				printf("    New Play Coins%s: %s%i%s\n\n", DARK_GRAY, CYAN, pc_, RESET);
				printf("%s    A%s = %sSet Play Coins\n", RED, DARK_GRAY, RESET);
				printf("%s    B%s = %sRevert\n", YELLOW, DARK_GRAY, RESET);
				printf("%s    Y%s = %sSystem Number Selection\n", GREEN, DARK_GRAY, RESET);
				printf("%s    X%s = %sSet to %i\n", BLUE, DARK_GRAY, RESET, pc_ == MAX_PC ? MIN_PC : MAX_PC);
				printf("Start%s = %sExit\n", DARK_GRAY, RESET);
				printf("   Up%s = %s+1%s\n", DARK_GRAY, GREEN, RESET);
				printf(" Down%s = %s-1%s\n", DARK_GRAY, RED, RESET);
				printf("Right%s = %s+25%s\n", DARK_GRAY, GREEN, RESET);
				printf(" Left%s = %s-25%s\n", DARK_GRAY, RED, RESET);
			}
		}
	}
	free(fb);
	close_extdata();
	gfxExit();
	return 0;
}
