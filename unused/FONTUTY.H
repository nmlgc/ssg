/*
 *   Unused double-spiral moiré animation
 *
 *   This animation was intended to be used for the staff roll sprites, but was
 *   effectively commented out for the final release and replaced with regular
 *   sprite blitting. A recording of the originally intended effect can be
 *   found at
 *
 *   	https://rec98.nmlgc.net/blog/2022-12-31
 *
 *   Moved here because the effect directly writes pixels into the back buffer,
 *   and might therefore be hard to port to non-DirectDraw renderers.
 */

typedef struct tagEXPOINT {
	int		x, y;		// 元座標(中心からの相対座標)
	int		l;			// 極座標における長さ
	BYTE	d;			// 極座標における角度

	PIXELFORMAT::LARGEST	c;			// 色情報
} EXPOINT;

typedef struct tagExtraFontInfo {
	int		Width;		// 元画像の幅
	int		Height;		// 元画像の高さ

	int		DataSize;	// 点の数

	EXPOINT	*Data;		// データ格納先
} ExtraFontInfo;

ExtraFontInfo *CreateExtraFont(SURFACE_DDRAW& Surface, PIXEL_LTRB *pSrc);	// フォント生成
void DeleteExtraFont(ExtraFontInfo *pFont);							// フォント削除

// Renders [pFont] to the back buffer at the given left/top position. Setting
// [t] renders the [t]th frame of a double-spiral moiré animation instead; in
// that case, [radius_max] can be used to restrict this animation to the given
// radius in the center of [pFont], which won't render any pixels outside.
void DrawExtraFont(ExtraFontInfo *pFont, int ox, int oy, int t = 0, int radius_max = 0);
