/*                                                                           */
/*   DI_UTY.cpp   Direct Input 関連の関数                                    */
/*                                                                           */
/*                                                                           */

#define DIRECTINPUT_VERSION 0x0700

#include <dinput.h>
#include "DI_UTY.H"
//#include "../GIAN_MAIN/GIAN.H"
#pragma message(PBGWIN_DI_UTY_H)

#pragma comment(lib,"dinput.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "winmm.lib")

#if   DIRECTINPUT_VERSION == 0x0700
	#pragma message("      DirectInput : Version 7.00")
#elif DIRECTINPUT_VERSION == 0x0600
	#pragma message("      DirectInput : Version 6.00")
#elif DIRECTINPUT_VERSION == 0x0500
	#pragma message("      DirectInput : Version 5.00")
#elif DIRECTINPUT_VERSION == 0x0300
	#pragma message("      DirectInput : Version 3.00")
#endif

//#pragma optimize("",off)

typedef struct tagINPUT_OBJ{
	LPDIRECTINPUT			pdi;
	LPDIRECTINPUTDEVICE		pdev;
} INPUT_OBJ;

// グローバル変数(Private)の実体 //
bool	UsingDirectInput = false;
INPUT_OBJ			InputObj;
InputConfig			IConfig;



bool Key_Start(void)
{
	HRESULT		dirval;

	// 初期値の準備 //
	memset(IConfig.bIsUsed,FALSE,256);
	IConfig.bIsUsed[DIK_ESCAPE] = TRUE;
	IConfig.bIsUsed[DIK_RETURN] = TRUE;
	IConfig.bIsUsed[DIK_UP]     = TRUE;
	IConfig.bIsUsed[DIK_DOWN]   = TRUE;
	IConfig.bIsUsed[DIK_LEFT]   = TRUE;
	IConfig.bIsUsed[DIK_RIGHT]  = TRUE;

	IConfig.KeyUp    = DIK_NUMPAD8;	IConfig.bIsUsed[DIK_NUMPAD8] = TRUE;
	IConfig.KeyDown  = DIK_NUMPAD2;	IConfig.bIsUsed[DIK_NUMPAD2] = TRUE;
	IConfig.KeyLeft  = DIK_NUMPAD4;	IConfig.bIsUsed[DIK_NUMPAD4] = TRUE;
	IConfig.KeyRight = DIK_NUMPAD6;	IConfig.bIsUsed[DIK_NUMPAD6] = TRUE;

	IConfig.KeyULeft  = DIK_NUMPAD7;	IConfig.bIsUsed[DIK_NUMPAD7] = TRUE;
	IConfig.KeyURight = DIK_NUMPAD9;	IConfig.bIsUsed[DIK_NUMPAD9] = TRUE;
	IConfig.KeyDLeft  = DIK_NUMPAD1;	IConfig.bIsUsed[DIK_NUMPAD1] = TRUE;
	IConfig.KeyDRight = DIK_NUMPAD3;	IConfig.bIsUsed[DIK_NUMPAD3] = TRUE;

	IConfig.KeyTama  = DIK_Z;		IConfig.bIsUsed[DIK_Z]       = TRUE;
	IConfig.KeyBomb  = DIK_X;		IConfig.bIsUsed[DIK_X]       = TRUE;
	IConfig.KeyShift = DIK_LSHIFT;	IConfig.bIsUsed[DIK_LSHIFT]  = TRUE;

	// DirectInput オブジェクトの作成 //
	extern HINSTANCE hInstanceMain;
	dirval = DirectInputCreate(
		hInstanceMain, DIRECTINPUT_VERSION, &InputObj.pdi, nullptr
	);
	if(dirval != DI_OK){
		dirval = DirectInputCreate(
			hInstanceMain, 0x0300, &InputObj.pdi, nullptr
		);
		if(dirval != DI_OK) return FALSE;
	}

	// DirectInput デバイス (Keyboard) の作成 //
	dirval = InputObj.pdi->CreateDevice(GUID_SysKeyboard,&(InputObj.pdev),NULL);
	if(dirval != DI_OK){
		Key_End();
		return FALSE;
	}

	// 協調レベルのセット //
	extern HWND hWndMain;
	dirval = InputObj.pdev->SetCooperativeLevel(
		hWndMain, (DISCL_NONEXCLUSIVE | DISCL_FOREGROUND)
	);
	if(dirval != DI_OK){
		Key_End();
		return FALSE;
	}

	// キーボードのデータフォーマットをセットする //
	dirval = InputObj.pdev->SetDataFormat(&c_dfDIKeyboard);
	if(dirval != DI_OK){
		Key_End();
		return FALSE;
	}

	dirval = InputObj.pdev->Acquire();
	if(dirval != DI_OK){
		Key_End();
		return FALSE;
	}

	// ＤＩを使用した読み込み //
	UsingDirectInput = true;

	return TRUE;
}

void Key_End(void)
{
	/*
	char	buf[100];

	sprintf(buf,"Dev:%x  DI:%x",InputObj.pdev,InputObj.pdi);
	DebugOut(buf);
*/
	if(InputObj.pdev){
		InputObj.pdev->Unacquire();
		InputObj.pdev->Release();
		InputObj.pdev = NULL;
	}

	if(InputObj.pdi){
		InputObj.pdi->Release();
		InputObj.pdi = NULL;
	}

	UsingDirectInput = false;
}

INPUT_BITS Key_Read_STD(void)
{
	INPUT_BITS	val = 0;
//	JOYINFOEX	joy;
//	UINT		x,y;
	BYTE		buf[256];

	GetKeyboardState(buf);

	if(buf[VK_UP]&0x80)			val |= KEY_UP;
	if(buf[VK_NUMPAD8]&0x80)	val |= KEY_UP;
	if(buf[VK_DOWN]&0x80)		val |= KEY_DOWN;
	if(buf[VK_NUMPAD2]&0x80)	val |= KEY_DOWN;
	if(buf[VK_LEFT]&0x80)		val |= KEY_LEFT;
	if(buf[VK_NUMPAD4]&0x80)	val |= KEY_LEFT;
	if(buf[VK_RIGHT]&0x80)		val |= KEY_RIGHT;
	if(buf[VK_NUMPAD6]&0x80)	val |= KEY_RIGHT;

	if(buf[VK_NUMPAD7]&0x80)	val |= (KEY_UP|KEY_LEFT);
	if(buf[VK_NUMPAD9]&0x80)	val |= (KEY_UP|KEY_RIGHT);
	if(buf[VK_NUMPAD1]&0x80)	val |= (KEY_DOWN|KEY_LEFT);
	if(buf[VK_NUMPAD3]&0x80)	val |= (KEY_DOWN|KEY_RIGHT);

	if(buf['Z']&0x80)			val |= KEY_TAMA;
	if(buf[VK_SPACE]&0x80)		val |= KEY_TAMA;
	if(buf['X']&0x80)			val |= KEY_BOMB;

	if(buf[VK_SHIFT]&0x80)		val |= KEY_SHIFT;

	if(buf[VK_RETURN]&0x80)		val |= KEY_RETURN;
	if(buf[VK_ESCAPE]&0x80)		val |= KEY_ESC;

	return val;
}

INPUT_BITS Key_Read_DI(void)
{
	static BYTE		buf[300];

	HRESULT		dirval;
	INPUT_BITS	val = 0;

	dirval = InputObj.pdev->GetDeviceState(256,(LPVOID)buf);
	if(dirval != DI_OK){
		if(dirval == DIERR_INPUTLOST){
			dirval = InputObj.pdev->Acquire();
			if(dirval==DI_OK) return 0;
		}

		// キー入力ＡＰＩを通常に戻す //
		Key_End();
		return 0;
	}

	if(buf[DIK_UP]&0x80)			val |= KEY_UP;
	if(buf[IConfig.KeyUp]&0x80)		val |= KEY_UP;

	if(buf[DIK_DOWN]&0x80)			val |= KEY_DOWN;
	if(buf[IConfig.KeyDown]&0x80)	val |= KEY_DOWN;

	if(buf[DIK_LEFT]&0x80)			val |= KEY_LEFT;
	if(buf[IConfig.KeyLeft]&0x80)	val |= KEY_LEFT;
	if(buf[DIK_RIGHT]&0x80)			val |= KEY_RIGHT;
	if(buf[IConfig.KeyRight]&0x80)	val |= KEY_RIGHT;

	if(buf[IConfig.KeyULeft]&0x80)	val |= KEY_ULEFT;
	if(buf[IConfig.KeyURight]&0x80)	val |= KEY_URIGHT;
	if(buf[IConfig.KeyDLeft]&0x80)	val |= KEY_DLEFT;
	if(buf[IConfig.KeyDRight]&0x80)	val |= KEY_DRIGHT;

	if(buf[IConfig.KeyTama]&0x80)	val |= KEY_TAMA;
	//if(buf[DIK_SPACE]&0x80)		val |= KEY_TAMA;
	if(buf[IConfig.KeyBomb]&0x80)	val |= KEY_BOMB;

	if(buf[IConfig.KeyShift]&0x80)	val |= KEY_SHIFT;

	if(buf[DIK_RETURN]&0x80)	val |= KEY_RETURN;
	if(buf[DIK_ESCAPE]&0x80)	val |= KEY_ESC;

	return val;
}


INPUT_BITS Pad_Read(const std::span<const INPUT_PAD_BINDING> PadBindings)
{
	JOYINFOEX	joy;
	int			x,y;
	INPUT_BITS	val = 0;

	joy.dwSize  = sizeof(joy);
	joy.dwFlags = JOY_RETURNBUTTONS|JOY_RETURNX|JOY_RETURNY;
	if(joyGetPosEx(JOYSTICKID1, &joy) == JOYERR_NOERROR){
		x = joy.dwXpos>>11;
		y = joy.dwYpos>>11;

		for(const auto& binding : PadBindings) {
			if(binding.first > 0) {
				if(joy.dwButtons & (1 << (binding.first - 1))) {
					val |= binding.second;
				}
			}
		}

		if(x<=12)	val |= KEY_LEFT;
		if(x>=20)	val |= KEY_RIGHT;
		if(y<=12)	val |= KEY_UP;
		if(y>=20)	val |= KEY_DOWN;

		return val;
	}

	return 0;
}


void Key_Read(const std::span<const INPUT_PAD_BINDING> PadBindings)
{
	const auto val = (UsingDirectInput ? Key_Read_DI() : Key_Read_STD());
	if(val == 0) {
		Pad_Data = Pad_Read(PadBindings);
	}

	Key_Data = (val | Pad_Data);
}


extern int Key_PadConfig(INPUT_BITS Key)
{
	JOYINFOEX	joy;
	int			ret;

	joy.dwSize  = sizeof(joy);
	joy.dwFlags = JOY_RETURNBUTTONS|JOY_RETURNX|JOY_RETURNY;
	if(joyGetPosEx(JOYSTICKID1, &joy) != JOYERR_NOERROR) return 0;

	switch(joy.dwButtons) {
		case JOY_BUTTON1:		ret = 1;	break;
		case JOY_BUTTON2:		ret = 2;	break;
		case JOY_BUTTON3:		ret = 3;	break;
		case JOY_BUTTON4:		ret = 4;	break;
		case JOY_BUTTON5:		ret = 5;	break;
		case JOY_BUTTON6:		ret = 6;	break;
		case JOY_BUTTON7:		ret = 7;	break;
		case JOY_BUTTON8:		ret = 8;	break;
		case JOY_BUTTON9:		ret = 9;	break;
		case JOY_BUTTON10:		ret = 10;	break;
		case JOY_BUTTON11:		ret = 11;	break;
		case JOY_BUTTON12:		ret = 12;	break;
		case JOY_BUTTON13:		ret = 13;	break;
		case JOY_BUTTON14:		ret = 14;	break;
		case JOY_BUTTON15:		ret = 15;	break;
		case JOY_BUTTON16:		ret = 16;	break;
		case JOY_BUTTON17:		ret = 17;	break;
		case JOY_BUTTON18:		ret = 18;	break;
		case JOY_BUTTON19:		ret = 19;	break;
		case JOY_BUTTON20:		ret = 20;	break;
		case JOY_BUTTON21:		ret = 21;	break;
		case JOY_BUTTON22:		ret = 22;	break;
		case JOY_BUTTON23:		ret = 23;	break;
		case JOY_BUTTON24:		ret = 24;	break;
		case JOY_BUTTON25:		ret = 25;	break;
		case JOY_BUTTON26:		ret = 26;	break;
		case JOY_BUTTON27:		ret = 27;	break;
		case JOY_BUTTON28:		ret = 28;	break;
		case JOY_BUTTON29:		ret = 29;	break;
		case JOY_BUTTON30:		ret = 30;	break;
		case JOY_BUTTON31:		ret = 31;	break;
		case JOY_BUTTON32:		ret = 32;	break;

		case 0:		return -1;

		default:	// 同時押しで、無効化できる //
			ret = 0;
		break;
	}
/*
	for(ret=1; ret<=32; ret++){
		val = 1 << (ret-1);
		if(val == joy.dwButtons) break;
	}

	if(ret == 33){
		val = 0;
		ret = 0;
	}
*/

	return ret;
}


//#pragma optimize("",on)
