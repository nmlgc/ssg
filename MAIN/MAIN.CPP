#include "../GIAN07/CONFIG.H"
#include "../GIAN07/ENTRY.H"
#include "../GIAN07/GAMEMAIN.H"
#include "strings/title.h"
#include "platform/input.h"
#include "platform/snd_backend.h"
#include "platform/unicode.h"
#include "game/bgm.h"
#include "game/input.h"
#include <windows.h>
#include <mmsystem.h>

#pragma comment(lib, "user32.lib")

extern constexpr auto APP_CLASS = _UNICODE("GIAN07");
extern constexpr auto APP_NAME = _UNICODE(GAME_TITLE);


long FAR __stdcall WndProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam);
static BOOL AppInit(HINSTANCE hInstance,int nCmdShow);

//void SaveSnapshot(HWND hWnd);


// グローバル変数 //
HINSTANCE	hInstanceMain;
HWND	hWndMain;
BOOL	bIsActive;
BOOL	bMouseVisible;
DWORD	WaitTime;


int __stdcall wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nShowCmd)
{
	MSG			msg;
	DWORD		tempdw;

	// 他のところで起動していたらそいつをRestoreする //
	auto old_gian = FindWindowW(APP_CLASS, nullptr);
	if(old_gian) {
		SetForegroundWindow(old_gian);
		SendMessageW(old_gian,WM_SYSCOMMAND,SC_RESTORE,0);
		return FALSE;
	}

	if(!AppInit(hInstance,nShowCmd)){
		return FALSE;
	}

	while(1){
		if(PeekMessageW(&msg,NULL,0,0,PM_NOREMOVE)){
			if(!GetMessageW(&msg,NULL,0,0)){
				return msg.wParam;
			}
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
		else if(bIsActive){
			// １フレーム進める //
			tempdw = timeGetTime();
			if(
				((tempdw - WaitTime) >= FRAME_TIME_TARGET) ||
				(ConfigDat.FPSDivisor.v == 0)
			) {
				Key_Read();

				bool quit = false;
				GameMain(quit);
				if(quit) {
					// DestroyWindow->PostMessage
					// タスクバーにアイコンが残るのを防止だっけ？
					PostMessage(hWndMain, WM_SYSCOMMAND, SC_CLOSE, 0);
				}

				WaitTime = tempdw;
			}
			//else Sleep(1);
		}
		else{
			WaitMessage();
		}
	}
}


long FAR __stdcall WndProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam)
{
	PAINTSTRUCT		ps;
	HDC				hdc;
	static	BOOL	ExitFlag = FALSE;

	switch(message){
		case WM_ACTIVATEAPP:
			bIsActive = (BOOL)wParam;
			if(bIsActive){
				bMouseVisible = FALSE;
				BGM_Pause();
				SndBackend_PauseAll();
			}
			else{
				bMouseVisible = TRUE;
				BGM_Resume();
				SndBackend_ResumeAll();
			}
		break;

		case WM_SETCURSOR:
			if(bMouseVisible) SetCursor(LoadCursor(NULL,IDC_ARROW));
			else              SetCursor(NULL),ShowCursor(TRUE);
		return 1;

		case WM_ERASEBKGND:
		return 1;

		case WM_PAINT:
			hdc = BeginPaint(hWnd,&ps);
			EndPaint(hWnd,&ps);
		return 1;

		// IME 関連のメッセージは無視 //
		case WM_IME_CHAR:		case WM_IME_COMPOSITION:		case WM_IME_COMPOSITIONFULL:
		case WM_IME_CONTROL:	case WM_IME_ENDCOMPOSITION:		case WM_IME_KEYDOWN:
		case WM_IME_KEYUP:		case WM_IME_NOTIFY:				case WM_IME_SELECT:
		case WM_IME_SETCONTEXT:	case WM_IME_STARTCOMPOSITION:

		#if(WINVER >= 0x0500)
			case WM_IME_REQUEST:
		#endif

		return 1;

		case WM_DESTROY:
			PostQuitMessage(0);
		break;

		case WM_SYSCOMMAND:
			if(wParam == SC_CLOSE){
				ExitFlag = TRUE;
				XCleanup();
				ShowCursor(true);
				MoveWindow(hWndMain,0,0,0,0,TRUE);
				ShowWindow(hWndMain,SW_HIDE);
				DestroyWindow(hWndMain);
			}
			break;

		case WM_SYSKEYDOWN:
			if(ExitFlag) return 0;
		break;

		case WM_KEYDOWN:
		/*	switch(wParam){
				case(VK_ESCAPE):
					DestroyWindow(hWnd);
				break;
			}*/
		return 0;

		default:
		break;
	}

	return DefWindowProcW(hWnd,message,wParam,lParam);
}

static BOOL AppInit(HINSTANCE hInstance,int nCmdShow)
{
	WNDCLASSW	wc;
	HMENU		hMenu;

	wc.style			= CS_DBLCLKS;							//
	wc.lpfnWndProc		= WndProc;								//
	wc.cbClsExtra		= 0;									//
	wc.cbWndExtra		= 0;									//
	wc.hInstance		= hInstance;							//
	wc.hIcon			= (HICON)LoadIcon(hInstance,"NEO_TAMA_ICON");		//
	wc.hCursor			= 0;									// 後で変更
	wc.hbrBackground	= (HBRUSH)GetStockObject(BLACK_BRUSH);			//
	wc.lpszMenuName		= 0;									// 後で変更
	wc.lpszClassName	= APP_CLASS;							//

	if(!RegisterClassW(&wc)){
		return FALSE;
	}

	hWndMain = CreateWindowExW(
		0,												//
		APP_CLASS,										//
		APP_NAME,										//
		(WS_VISIBLE|WS_SYSMENU|WS_EX_TOPMOST|WS_POPUP),	//
		0,												//
		0,												//
		GetSystemMetrics(SM_CXSCREEN),					//
		GetSystemMetrics(SM_CYSCREEN),					//
		NULL,											//
		NULL,											//
		hInstance,										//
		NULL);											//

	if(!hWndMain) return FALSE;
	hInstanceMain = hInstance;

	//ShowWindow(hWndMain,nCmdShow);
	//UpdateWindow(hWndMain);
	ShowCursor(FALSE);

	hMenu = GetSystemMenu(hWndMain,FALSE);
	DeleteMenu(hMenu,SC_MAXIMIZE,MF_BYCOMMAND);
	DeleteMenu(hMenu,SC_MINIMIZE,MF_BYCOMMAND);
	DeleteMenu(hMenu,SC_SIZE    ,MF_BYCOMMAND);

	if(!XInit()) {
		DestroyWindow(hWndMain);
		return false;
	}

	return TRUE;
}
