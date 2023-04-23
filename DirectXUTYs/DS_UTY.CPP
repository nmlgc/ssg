/*                                                                           */
/*   DS_UTY.cpp   効果音関連の処理                                           */
/*                                                                           */
/*                                                                           */

#define DIRECTSOUND_VERSION 0x0700

#include <dsound.h>
#include "platform/file.h"
#include "DS_UTY.H"
#pragma message(PBGWIN_DS_UTY_H)

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

//#pragma optimize("",off)

typedef struct tagSE_STRUCT{
	BYTE				*data;		//
	DWORD				size;		//
	int					max;		//
	int					now;		//
	IDirectSoundBuffer	*buf[1];	//
} SE_STRUCT,*LPSE_STRUCT;


bool SndFillBuffer(IDirectSoundBuffer *ds, uint8_t *data, size_t size);

// サウンド停止用補助関数 //
static void _Snd_Stop(LPSE_STRUCT se,int no);

LPDIRECTSOUND			Snd_DS = NULL;
LPSE_STRUCT				Snd_OBJ[SND_OBJ_MAX];
int						SndXMID = SND_XMID_DEFAULT;


bool SndInit(HWND hWnd, int x_mid)
{
	HRESULT		dsrval;

	try{
		dsrval = DirectSoundCreate(NULL,&Snd_DS,NULL);
		if(dsrval != DS_OK) throw("DirectSoundCreate に失敗");

		dsrval = Snd_DS->SetCooperativeLevel(hWnd,DSSCL_NORMAL);
		if(dsrval != DS_OK) throw("SetCooperativeLevel(DSound) に失敗");

		SndXMID = x_mid;
	}
	catch(char *error){
		DebugOut(error);
		SndCleanup();
		return FALSE;
	}

	return TRUE;
}

void SndCleanup(void)
{
	int		i;

	// ここで SE_STRUCT 関連の解放を行う //
	for(i=0;i<SND_OBJ_MAX;i++){
		SndRelease(i);
	}

	// 念のため... //
	for(i=0;i<SND_OBJ_MAX;i++) Snd_OBJ[i] = NULL;

	// ＤＳオブジェクトを解放 //
	RELEASE(Snd_DS);
}

bool SndWAVLoad(BYTE_BUFFER_OWNED buffer, BYTE id, int max)
{
	LPSE_STRUCT		p;
	HRESULT			dsrval;
	DSBUFFERDESC	dsBD;
	int				i;
	LPSE_STRUCT		*se;

	if(Snd_DS==NULL) return FALSE;

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
	const auto& data = maybe_data.value();

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
		DebugOut("CreateSoundBuffer に失敗");
		return FALSE;
	}

	if(!SndFillBuffer(p->buf[0],p->data,p->size)){
		DebugOut("SndFillBuffer に失敗 (PBGUTY_X.LIB)");
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

void SndPlay(uint8_t id)
{
	HRESULT	dsrval;
	DWORD	st;
	LPSE_STRUCT se;

	if(id>=SND_OBJ_MAX) return;

	se = Snd_OBJ[id];
	if(se == NULL || se->buf[se->now] == NULL) return;

	dsrval = se->buf[se->now]->GetStatus(&st);
	if(dsrval != DS_OK) st=0;
	if((st&DSBSTATUS_PLAYING)==DSBSTATUS_PLAYING) _Snd_Stop(se,se->now);

	se->buf[se->now]->SetPan(0);
	se->buf[se->now]->Play(0,0,0);
	se->now = (se->now+1)%(se->max);
}

void SndPlayEX(uint8_t id, int x, bool loop)
{
	HRESULT	dsrval;
	DWORD	st;
	DWORD	pan = (x - SndXMID)>>4;
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

void SndStop(uint8_t id)
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

void SndStopAll(void)
{
	int i;

	for(i=0;i<SND_OBJ_MAX;i++) SndStop(i);
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

bool SndRestore(void)
{
	return FALSE;
}

static void _Snd_Stop(LPSE_STRUCT se,int no)
{
	if(se == NULL || se->buf[no]==NULL) return;

	se->buf[no]->Stop();
	se->buf[no]->SetCurrentPosition(0);
}

//#pragma optimize("",on)
