/*                                                                           */
/*   DS_UTY.cpp   効果音関連の処理                                           */
/*                                                                           */
/*                                                                           */

#define DIRECTSOUND_VERSION 0x0700

#include <dsound.h>
#include "platform/file.h"
#include "platform/snd_backend.h"
#include "DirectXUTYs/DX_ERROR.H"	// エラー処理用関数
#pragma message("DS_UTY : Version 0.03 : Update 2000/02/04")

#pragma comment(lib,"dsound.lib")

#if   DIRECTSOUND_VERSION == 0x0700
	#pragma message("      DirectSound : Version 7.00")
#elif DIRECTSOUND_VERSION == 0x0600
	#pragma message("      DirectSound : Version 6.00")
#elif DIRECTSOUND_VERSION == 0x0500
	#pragma message("      DirectSound : Version 5.00")
#elif DIRECTSOUND_VERSION == 0x0300
	#pragma message("      DirectSound : Version 3.00")
#else
	#pragma message("      DirectSound : Version <= 6.00???")
#endif

#ifndef DSBCAPS_CTRLDEFAULT
	#define DSBCAPS_CTRLDEFAULT \
		(DSBCAPS_CTRLPAN | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLFREQUENCY)
#endif

//#pragma optimize("",off)

// 構造体 //

#pragma pack(push, PBG_WAVEHEAD_PACK)
#pragma pack(2)

typedef struct tagWAVEHEAD{
	DWORD	RIFF;
	DWORD	RiffLen;
	DWORD	WAVE;
	DWORD	FMT;
	DWORD	WaveLen;

	WORD	tag;
	WORD	ch;
	DWORD	SamplePerSec;
	DWORD	AvgBytePerSec;
	WORD	BlockAlign;
	WORD	BitPerSample;

	DWORD	DATA;
	DWORD	DataLen;
} WAVEHEAD;

typedef struct tagWAVEHEAD_FACT{
	DWORD	RIFF;
	DWORD	RiffLen;
	DWORD	WAVE;
	DWORD	FMT;
	DWORD	WaveLen;

	WORD	tag;
	WORD	ch;
	DWORD	SamplePerSec;
	DWORD	AvgBytePerSec;
	WORD	BlockAlign;
	WORD	BitPerSample;

	DWORD	FACT;
	WORD	Dummy_2;
	WORD	Dummy_3;
	WORD	Dummy_4;
	WORD	Dummy_5;

	DWORD	DATA;
	DWORD	DataLen;
} WAVEHEAD_FACT;

#pragma pack(pop, PBG_WAVEHEAD_PACK)


typedef struct tagSE_STRUCT{
	BYTE				*data;		//
	DWORD				size;		//
	int					max;		//
	SND_INSTANCE_ID	now;
	IDirectSoundBuffer	*buf[1];	//
} SE_STRUCT,*LPSE_STRUCT;


// 補助 //
void SndRelease(uint8_t id);
bool SndFillBuffer(IDirectSoundBuffer *ds, uint8_t *data, size_t size);

// サウンド停止用補助関数 //
static void _Snd_Stop(LPSE_STRUCT se,int no);

LPDIRECTSOUND			Snd_DS = NULL;
LPSE_STRUCT				Snd_OBJ[SND_OBJ_MAX];


bool SndBackend_Init(void)
{
	HRESULT		dsrval;

	const auto fail = [](std::u8string_view error) {
		DebugOut(error);
		SndBackend_Cleanup();
		return false;
	};

	dsrval = DirectSoundCreate(nullptr, &Snd_DS, nullptr);
	if(dsrval != DS_OK) {
		return fail(u8"DirectSoundCreate に失敗");
	}

	extern HWND hWndMain;
	dsrval = Snd_DS->SetCooperativeLevel(hWndMain, DSSCL_NORMAL);
	if(dsrval != DS_OK) {
		return fail(u8"SetCooperativeLevel(DSound) に失敗");
	}

	return TRUE;
}

void SndBackend_Cleanup(void)
{
	// ＤＳオブジェクトを解放 //
	if(Snd_DS) {
		Snd_DS->Release();
		Snd_DS = nullptr;
	}
}

bool SndBackend_SEInit(void)
{
	return true;
}

void SndBackend_SECleanup(void)
{
	// ここで SE_STRUCT 関連の解放を行う //
	for(auto i = 0; i < SND_OBJ_MAX; i++) {
		SndRelease(i);
		// 念のため... //
		Snd_OBJ[i] = nullptr;
	}
}

// ＷＡＶファイルロード(ある制限があるが...) //
bool SndBackend_SELoad(
	BYTE_BUFFER_OWNED buffer, uint8_t id, SND_INSTANCE_ID max
)
{
	LPSE_STRUCT		p;
	HRESULT			dsrval;
	DSBUFFERDESC	dsBD;
	int				i;
	LPSE_STRUCT		*se;

	if(Snd_DS==NULL) return FALSE;

	if(max == 0) {
		return false;
	}
	if(id>=SND_OBJ_MAX) return FALSE;
	se = &Snd_OBJ[id];

	// SE_STRUCT 用のメモリを確保する //
	*se = p = (LPSE_STRUCT)LocalAlloc(LPTR,sizeof(SE_STRUCT)+(max-1)*sizeof(IDirectSoundBuffer *));
	if(p == NULL) return FALSE;

	// データを解凍して読み込む //
	auto cursor = buffer.cursor();
	const auto maybe_whf = cursor.next<WAVEHEAD_FACT>();
	if(!maybe_whf) {
		return false;
	}

	const auto [datalen, tag] = [&cursor](const WAVEHEAD_FACT& whf) {
		if(whf.FACT != MAKEFOURCC('f','a','c','t')) {
			static_assert(sizeof(WAVEHEAD_FACT) >= sizeof(WAVEHEAD));
			const auto& wh = reinterpret_cast<const WAVEHEAD &>(whf);
			cursor.cursor = sizeof(wh);

			// C cast because we also need to cast away constness here… grr.
			return std::pair{ wh.DataLen, (WAVEFORMATEX *)(&wh.tag) };
		}
		cursor.cursor = sizeof(whf);
		return std::pair{ whf.DataLen, (WAVEFORMATEX *)(&whf.tag) };
	}(maybe_whf.value()[0]);

	const auto maybe_data = cursor.next<uint8_t>(datalen);
	if(!maybe_data) {
		return false;
	}
	const auto data = maybe_data.value();

	p->data = (BYTE *)LocalAlloc(LPTR,datalen);
	memcpy(p->data, data.data(), data.size_bytes());

	p->max  = max;
	p->now  = 0;
	p->size = datalen;

	dsBD.dwBufferBytes = datalen;
	dsBD.dwFlags       = DSBCAPS_STATIC|DSBCAPS_CTRLDEFAULT|DSBCAPS_GETCURRENTPOSITION2;
	dsBD.dwReserved    = 0;
	dsBD.dwSize        = sizeof(DSBUFFERDESC);
	dsBD.lpwfxFormat   = tag;

	dsrval = Snd_DS->CreateSoundBuffer(&dsBD,&p->buf[0],NULL);
	if(dsrval != DS_OK){
		DebugOut(u8"CreateSoundBuffer に失敗");
		return FALSE;
	}

	if(!SndFillBuffer(p->buf[0],p->data,p->size)){
		DebugOut(u8"SndFillBuffer に失敗 (PBGUTY_X.LIB)");
		return FALSE;
	}

	for(i=1;i<max;i++){
		dsrval = Snd_DS->DuplicateSoundBuffer(p->buf[0],&p->buf[i]);
		if(dsrval != DS_OK){
			if(!SndFillBuffer(p->buf[i],p->data,p->size)) return FALSE;
		}
	}

	return TRUE;
}

void SndBackend_SEPlay(uint8_t id, int x, bool loop)
{
	HRESULT	dsrval;
	DWORD	st;
	int32_t	pan = ((x - SND_X_MID) / (SND_X_PER_DECIBEL / 100));
	LPSE_STRUCT se;

	if(id>=SND_OBJ_MAX) return;

	se = Snd_OBJ[id];
	if(se == NULL || se->buf[se->now] == NULL) return;

	dsrval = se->buf[se->now]->GetStatus(&st);
	if(dsrval != DS_OK) st=0;
	if((st&DSBSTATUS_PLAYING)==DSBSTATUS_PLAYING) _Snd_Stop(se,se->now);

	se->buf[se->now]->SetPan(pan);
	se->buf[se->now]->Play(0,0,loop ? DSBPLAY_LOOPING : 0);
	se->now = (se->now+1)%(se->max);
}

void SndBackend_SEStop(uint8_t id)
{
	int i;
	LPSE_STRUCT se;

	if(id>=SND_OBJ_MAX) return;

	se = Snd_OBJ[id];
	if(se == NULL) return;

	for(i=0;i<se->max;i++){
		if(se->buf[i]==NULL) continue;
		se->buf[i]->Stop();
		se->buf[i]->SetCurrentPosition(0);
	}
}

// These are no-ops – by default, DirectSound pauses and resumes all playing
// buffers as the window loses and regains focus. This behavior can be changed
// by setting the DSBCAPS_GLOBALFOCUS flag on a buffer, but none of our buffers
// do so yet.
void SndBackend_PauseAll()
{
}
void SndBackend_ResumeAll()
{
}

bool SndFillBuffer(IDirectSoundBuffer *ds, uint8_t *data, size_t size)
{
	LPVOID	pMem1,pMem2;
	DWORD	dwSize1,dwSize2;
	HRESULT	dsrval;

	if(ds == NULL) return FALSE;

	dsrval = ds->Lock(0,size,&pMem1,&dwSize1,&pMem2,&dwSize2,0);
	if(dsrval != DS_OK) return FALSE;

	CopyMemory(pMem1,data,dwSize1);
	if(dwSize2 != 0)
		CopyMemory(pMem2,data+dwSize1,dwSize2);

	ds->Unlock(pMem1,dwSize1,pMem2,dwSize2);

	return TRUE;
}

void SndRelease(uint8_t id)
{
	int i;
	LPSE_STRUCT se;

	if(id>=SND_OBJ_MAX) return;

	se = Snd_OBJ[id];
	if(se == NULL) return;

	for(i=0;i<se->max;i++){
		if(se->buf[i] != NULL){
			se->buf[i]->Release();
			se->buf[i] = NULL;
		}
	}

	if(se->data != NULL) LocalFree(se->data);
	se->data = NULL;

	LocalFree(se);
}

static void _Snd_Stop(LPSE_STRUCT se,int no)
{
	if(se == NULL || se->buf[no]==NULL) return;

	se->buf[no]->Stop();
	se->buf[no]->SetCurrentPosition(0);
}

//#pragma optimize("",on)
