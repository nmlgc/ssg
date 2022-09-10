/*                                                                           */
/*   D3_TEX.h   テクスチャ管理用関数                                         */
/*                                                                           */
/*                                                                           */

#ifndef PBGWIN_D3_TEX_H
#define PBGWIN_D3_TEX_H		"D3_TEX : Version 0.02 : Update 1999/12/13"


// 更新履歴 //



// 定数＆マクロ //
#define TEXTURE_MAX		20				// テクスチャの枚数(最大)


// 関数 //
FBOOL TexInit(void);							// 最適なテクスチャフォーマットを検索する
FBOOL TexCreateTexture(char *name,DWORD txid);	// テクスチャをロードする
FVOID TexReleaseTexture(DWORD txid);			// テクスチャを解放する
FVOID TexSetTexture(DWORD txid);				// テクスチャハンドルをセットする


#endif
