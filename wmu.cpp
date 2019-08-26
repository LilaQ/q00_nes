#define _CRT_SECURE_NO_DEPRECATE
#include "SDL2/include/SDL.h"
#include <string>
#include <string.h>
#include <thread>
#ifdef _WIN32
	#include <Windows.h>
	#include <WinUser.h>
	#include "SDL2/include/SDL_syswm.h"
#endif // _WIN32
#include "commctrl.h"
#include "ppu.h"
#include "cpu.h"
#include "main.h"
#undef main

using namespace::std;

SDL_Window* mainWindow;				//	Main Window

void initWindow(SDL_Window *win, string filename) {
	mainWindow = win;
	char title[50];
	string rom = filename;
	if (filename.find_last_of("\\") != string::npos)
		rom = filename.substr(filename.find_last_of("\\") + 1);
	snprintf(title, sizeof title, "[ q00.nes ][ rom: %s ]", rom.c_str());
	SDL_SetWindowTitle(mainWindow, title);
	SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	SDL_GetWindowWMInfo(mainWindow, &wmInfo);
	HWND hwnd = wmInfo.info.win.window;
	HMENU hMenuBar = CreateMenu();
	HMENU hFile = CreateMenu();
	HMENU hEdit = CreateMenu();
	HMENU hHelp = CreateMenu();
	HMENU hConfig = CreateMenu();
	HMENU hSound = CreateMenu();
	HMENU hPalettes = CreateMenu();
	HMENU hDebugger = CreateMenu();
	HMENU hSavestates = CreateMenu();
	HMENU hVol = CreateMenu();
	AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hFile, L"[ main ]");
	AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hConfig, L"[ config ]");
	AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hVol, L"[ vol ]");
	AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hSavestates, L"[ savestates ]");
	AppendMenu(hMenuBar, MF_STRING, 11, L"[ ||> un/pause ]");
	AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hDebugger, L"[ debug ]");
	AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hHelp, L"[ help ]");
	AppendMenu(hFile, MF_STRING, 9, L"» load rom");
	AppendMenu(hFile, MF_STRING, 7, L"» reset");
	AppendMenu(hFile, MF_STRING, 1, L"» exit");
	AppendMenu(hHelp, MF_STRING, 3, L"» about");
	AppendMenu(hDebugger, MF_STRING, 4, L"» CHR table");
	AppendMenu(hDebugger, MF_STRING, 5, L"» Nametables");
	AppendMenu(hDebugger, MF_STRING, 10, L"» OAM tables");
	AppendMenu(hSavestates, MF_STRING, 12, L"» save state");
	AppendMenu(hSavestates, MF_STRING, 13, L"» load state");
	AppendMenu(hConfig, MF_POPUP, (UINT_PTR)hSound, L"[ sound ]");
	AppendMenu(hSound, MF_STRING, 14, L"» disable/enable");
	AppendMenu(hSound, MF_STRING, 15, L"» disable/enable SC1");
	AppendMenu(hSound, MF_STRING, 16, L"» disable/enable SC2");
	AppendMenu(hSound, MF_STRING, 17, L"» disable/enable SC3");
	AppendMenu(hSound, MF_STRING, 18, L"» disable/enable SC4");
	AppendMenu(hSound, MF_STRING, 23, L"» toggle 8-bit remix mode");
	AppendMenu(hVol, MF_STRING, 24, L"» 10%");
	AppendMenu(hVol, MF_STRING, 25, L"» 20%");
	AppendMenu(hVol, MF_STRING, 26, L"» 30%");
	AppendMenu(hVol, MF_STRING, 27, L"» 40%");
	AppendMenu(hVol, MF_STRING, 28, L"» 50%");
	AppendMenu(hVol, MF_STRING, 29, L"» 60%");
	AppendMenu(hVol, MF_STRING, 30, L"» 70%");
	AppendMenu(hVol, MF_STRING, 31, L"» 80%");
	AppendMenu(hVol, MF_STRING, 32, L"» 90%");
	AppendMenu(hVol, MF_STRING, 33, L"» 100%");
	SetMenu(hwnd, hMenuBar);

	//	Enable WM events for SDL Window
	SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);
}

void handleWindowEvents(SDL_Event event) {
	//	poll events from menu
	SDL_PollEvent(&event);
	switch (event.type)
	{
		case SDL_SYSWMEVENT:
			if (event.syswm.msg->msg.win.msg == WM_COMMAND) {
				//	Exit
				if (LOWORD(event.syswm.msg->msg.win.wParam) == 1) {
					exit(0);
				}
				//	About
				else if (LOWORD(event.syswm.msg->msg.win.wParam) == 3) {
					//showAbout();
				}
				//	CHR Table
				else if (LOWORD(event.syswm.msg->msg.win.wParam) == 4) {
					drawCHRTable();
				}
				//	Nametables
				else if (LOWORD(event.syswm.msg->msg.win.wParam) == 5) {
					drawNameTables();
				}
				//	Sprite Map
				else if (LOWORD(event.syswm.msg->msg.win.wParam) == 6) {
					/*SDL_Window* tWindow;
					SDL_Renderer* tRenderer;
					SDL_CreateWindowAndRenderer(256, 256, 0, &tWindow, &tRenderer);
					SDL_SetWindowSize(tWindow, 512, 512);
					drawSpriteMap(tRenderer, tWindow);*/
				}
				//	Reset
				else if (LOWORD(event.syswm.msg->msg.win.wParam) == 7) {
					//resetGameboy();
				}
				//	Memory Map
				else if (LOWORD(event.syswm.msg->msg.win.wParam) == 8) {
					//showMemoryMap();
				}
				//	Load ROM
				else if (LOWORD(event.syswm.msg->msg.win.wParam) == 9) {
					/*printf("loading ROM");
					char f[100];
					OPENFILENAME ofn;

					ZeroMemory(&f, sizeof(f));
					ZeroMemory(&ofn, sizeof(ofn));
					ofn.lStructSize = sizeof(ofn);
					ofn.hwndOwner = NULL;  // If you have a window to center over, put its HANDLE here
					ofn.lpstrFilter = "GameBoy Roms\0*.gb\0";
					ofn.lpstrFile = f;
					ofn.nMaxFile = MAX_PATH;
					ofn.lpstrTitle = "[ rom selection ]";
					ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;

					if (GetOpenFileNameA(&ofn)) {
						filename = f;
						resetGameboy();
					}
					*/
				}
				//	OAM
				else if (LOWORD(event.syswm.msg->msg.win.wParam) == 10) {
					drawOAM();
				}
				//	pause / unpause
				else if (LOWORD(event.syswm.msg->msg.win.wParam) == 11) {
					togglePause();
				}
				//	save state
				else if (LOWORD(event.syswm.msg->msg.win.wParam) == 12) {
					//	TODO STUB
					/*printf("saving state\n");
					char f[100];
					OPENFILENAME ofn;

					ZeroMemory(&f, sizeof(f));
					ZeroMemory(&ofn, sizeof(ofn));
					ofn.lStructSize = sizeof(ofn);
					ofn.hwndOwner = NULL;  // If you have a window to center over, put its HANDLE here
					ofn.lpstrFilter = "GameBoy Savestates\0*.gbss\0";
					ofn.lpstrFile = f;
					ofn.lpstrDefExt = "gbss";
					ofn.nMaxFile = MAX_PATH;
					ofn.lpstrTitle = "[ rom selection ]";
					ofn.Flags = OFN_DONTADDTORECENT;

					if (GetSaveFileNameA(&ofn)) {
						filename = f;
						saveState(f, registers, flags, pc, sp);
					}*/
				}
				//	load state
				else if (LOWORD(event.syswm.msg->msg.win.wParam) == 13) {

					//	halt emulation to avoid damaging the data (keep it that way, user must unpause)
					/*unpaused = false;

					printf("loading savestate\n");
					char f[100];
					OPENFILENAME ofn;

					ZeroMemory(&f, sizeof(f));
					ZeroMemory(&ofn, sizeof(ofn));
					ofn.lStructSize = sizeof(ofn);
					ofn.hwndOwner = NULL;
					ofn.lpstrFilter = "GameBoy Savestates\0*.gbss\0";
					ofn.lpstrFile = f;
					ofn.nMaxFile = MAX_PATH;
					ofn.lpstrTitle = "[ savestate selection ]";
					ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;

					if (GetOpenFileNameA(&ofn)) {
						filename = f;
						loadState(f, registers, flags, pc, sp);
					}*/
				}
				//	disable / enable sound
				else if (LOWORD(event.syswm.msg->msg.win.wParam) == 14) {
					//toggleAudio();
				}
				//	disable / enable SC1
				else if (LOWORD(event.syswm.msg->msg.win.wParam) == 15) {
					//toggleSC1();
				}
				//	disable / enable SC2
				else if (LOWORD(event.syswm.msg->msg.win.wParam) == 16) {
					//toggleSC2();
				}
				//	disable / enable SC3
				else if (LOWORD(event.syswm.msg->msg.win.wParam) == 17) {
					//toggleSC3();
				}
				//	disable / enable SC4
				else if (LOWORD(event.syswm.msg->msg.win.wParam) == 18) {
					//toggleSC4();
				}
				//	toggle 8bit remix mode
				else if (LOWORD(event.syswm.msg->msg.win.wParam) == 23) {
					//toggleRemix();
				}
				//	set volume
				else if (LOWORD(event.syswm.msg->msg.win.wParam) == 24 ||
					LOWORD(event.syswm.msg->msg.win.wParam) == 25 ||
					LOWORD(event.syswm.msg->msg.win.wParam) == 26 ||
					LOWORD(event.syswm.msg->msg.win.wParam) == 27 ||
					LOWORD(event.syswm.msg->msg.win.wParam) == 28 ||
					LOWORD(event.syswm.msg->msg.win.wParam) == 29 ||
					LOWORD(event.syswm.msg->msg.win.wParam) == 30 ||
					LOWORD(event.syswm.msg->msg.win.wParam) == 31 ||
					LOWORD(event.syswm.msg->msg.win.wParam) == 32 ||
					LOWORD(event.syswm.msg->msg.win.wParam) == 33
					) {
					//setVolume((float)(LOWORD(event.syswm.msg->msg.win.wParam) - 23) * 0.1);
				}
			}
			//	close a window
			if (event.syswm.msg->msg.win.msg == WM_CLOSE) {
				DestroyWindow(event.syswm.msg->msg.win.hwnd);
				PostMessage(event.syswm.msg->msg.win.hwnd, WM_CLOSE, 0, 0);
			}
			break;
		default:
			break;
	};

	uint8_t* keys = (uint8_t*)SDL_GetKeyboardState(NULL);
	//	pause/unpause
	if (keys[SDL_SCANCODE_SPACE]) {
		togglePause();
		keys[SDL_SCANCODE_SPACE] = 0;
	}
}
