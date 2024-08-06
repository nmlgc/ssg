/*                                                                           */
/*   FontUty.cpp   フォントの処理いろいろ                                    */
/*                                                                           */
/*                                                                           */

#include "FONTUTY.H"
#include "LOADER.H"
#include "DirectXUTYs/DD_UTY.H"

// Glyph selection inside the 16×16 font //
std::optional<PIXEL_LTRB> Glyph16x16(char c)
{
	PIXEL_LTWH src;
	src.w = 16;
	src.h = 16;

	// GrTama における文字の並びは次のようになっている。 //
	// (変更する可能性もあるので十分に注意すること)      //
	// ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789              //
	// abcdefghijklmnopqrstuvwxyz!?#\<>=,+-              //

	if((c >= 'A') && (c <= 'Z')) {
		src.left = ((c - 'A') << 4); src.top = (480 - 32);
	} else if((c >= 'a') && (c <= 'z')) {
		src.left = ((c - 'a') << 4); src.top = (480 - 16);
	} else if((c >= '0') && (c <= '9')) {
		src.left = (((c - '0') << 4) + 416); src.top = (480 - 32);
	} else {
		switch(c) {
		case('!'):	src.left = ((0 << 4) + 416); src.top = (480 - 16);	break;
		case('?'):	src.left = ((1 << 4) + 416); src.top = (480 - 16);	break;
		case('#'):	src.left = ((2 << 4) + 416); src.top = (480 - 16);	break;
		case('\\'):	src.left = ((3 << 4) + 416); src.top = (480 - 16);	break;
		case('<'):	src.left = ((4 << 4) + 416); src.top = (480 - 16);	break;
		case('>'):	src.left = ((5 << 4) + 416); src.top = (480 - 16);	break;
		case('='):	src.left = ((6 << 4) + 416); src.top = (480 - 16);	break;
		case(','):	src.left = ((7 << 4) + 416); src.top = (480 - 16);	break;
		case('+'):	src.left = ((8 << 4) + 416); src.top = (480 - 16);	break;
		case('-'):	src.left = ((9 << 4) + 416); src.top = (480 - 16);	break;
		default:	return std::nullopt;
		}
	}
	return src;
}


// 16x16 透過フォントで文字列出力(高速) //
extern void GrpPut16(int x, int y, const char *s)
{
	int		sx,tx,ty;

	sx = x;

	for(;(*s)!='\0';s++,x+=14){
		auto maybe_src = Glyph16x16(*s);
		if(maybe_src) {
			auto& src = maybe_src.value();

			tx = x;ty = y;
			if(tx>=0 && tx<630)	// 安全対策???
				GrpBlt(&src, tx, ty, GrTama);
		}
	}
}

// 上と同じだが、ｘ移動幅が１６ //
extern void GrpPut16c2(int x, int y, const char *s)
{
	int		sx,tx,ty;

	sx = x;

	for(;(*s)!='\0';s++,x+=16){
		auto maybe_src = Glyph16x16(*s);
		if(maybe_src) {
			auto& src = maybe_src.value();

			tx = x;ty = y;
			//if(tx>=0 && tx<630)	// 安全対策???
			GrpBlt(&src,tx,ty,GrTama);
		}
	}
}

// 16x16 透過フォントで文字出力(クリッピング有) //
extern void GrpPutc(int x, int y, char c)
{
	auto maybe_src = Glyph16x16(c);
	if(maybe_src) {
		auto& src = maybe_src.value();

		GrpBlt(&src, x, y, GrTama);
	}
}

// 05x07 べた貼りフォント //
extern void GrpPut57(int x, int y, const char *s)
{
	PIXEL_LTRB	src;
	int		sx,tx,ty;

	sx = x;

	for(;(*s)!='\0';s++,x+=6){
		if((*s)>='0' && (*s)<='9'){
			src = PIXEL_LTWH{ (((*s - '0') << 3) + 128), 80, 5, 7 };
		}
		else{
			continue;
		}

		tx = x;ty = y;
		if(tx>=0 && tx<630)	// 安全対策???
			GrpBlt(&src,tx,ty,GrTama);
	}
}

// 07x11 音楽室用フォント
extern void GrpPut7B(int x, int y, const char *s)
{
	PIXEL_LTRB	src;
	for(;(*s)!='\0';s++,x+=8){
		if((*s)>='0' && (*s)<='9'){
			src = PIXEL_LTWH{ (((*s - '0') << 3) + 184), 456, 7, 11 };
		}
		else if((*s)=='-'){
			src = PIXEL_LTWH{ (264 + 0), 456, 7, 11 };
		}
		else if((*s)==':'){
			src = PIXEL_LTWH{ (264 + 8), 456, 7, 11 };
		}
		else{
			continue;
		}

		if((x >= 0) && (x < 630)) { // 安全対策???
			GrpBlt(&src, x, y, GrMusic);
		}
	}
}

// 得点アイテムのスコアを描画 //
extern void GrpPutScore(int x, int y, const char *s)
{
	PIXEL_LTRB	src;
	int		sx,tx,ty;

	sx = x;

	for(;(*s)!='\0';s++,x+=6){
		if((*s)>='0' && (*s)<='9'){
			src = PIXEL_LTWH{ (((*s - '0') << 3) + 128), 88, 5, 7 };
		}
		else{
			continue;
		}

		tx = x;ty = y;
		if(tx>=0 && tx<630)	// 安全対策???
			GrpBlt(&src,tx,ty,GrTama);
	}
}

// MIDI 用フォントを描画する //
extern void GrpPutMidNum(int x, int y, int n)
{
	char	buf[10];
	int		i;
	PIXEL_LTRB	src;

	sprintf(buf,"%3d",n);

	// n = 1Byte ならば３桁以内に収まるハズ //
	for(i=0;i<3;i++,x+=5){
		if(buf[i]>='0' && buf[i]<='9'){
			src = PIXEL_LTWH{ (80 + ((buf[i] - '0') * 4)), 432, 4, 5 };
			GrpBlt(&src, x, y, GrMusic);
		}
		if(buf[i]=='-'){
			src = PIXEL_LTWH{ (80 + (10 * 4)), 432, 4, 5 };
			GrpBlt(&src, x, y, GrMusic);
		}
	}
}

struct GDI_FONTS {
	ENUMARRAY<HFONT, FONT_ID> fonts;

	GDI_FONTS() noexcept
	{
		LOGFONTW logfont = {
			.lfEscapement = 0,
			.lfOrientation = 0,
			.lfItalic = false,
			.lfUnderline = false,
			.lfStrikeOut = false,
			.lfCharSet = SHIFTJIS_CHARSET,
			.lfOutPrecision = OUT_TT_ONLY_PRECIS,
			.lfClipPrecision = CLIP_DEFAULT_PRECIS,
			.lfQuality = PROOF_QUALITY,
			.lfPitchAndFamily = FIXED_PITCH,
			.lfFaceName = L"ＭＳ ゴシック"
		};

		// でかいフォント //
		logfont.lfHeight = 24;
		logfont.lfWidth = 12;
		logfont.lfWeight = FW_MEDIUM;
		fonts[FONT_ID::LARGE] = CreateFontIndirectW(&logfont);

		// 並なフォント //
		logfont.lfHeight = 16;
		logfont.lfWidth = 8;
		logfont.lfWeight = FW_NORMAL;
		fonts[FONT_ID::NORMAL] = CreateFontIndirectW(&logfont);

		// ちっこいフォント //
		logfont.lfHeight = 14;
		logfont.lfWidth = 7;
		logfont.lfWeight = FW_NORMAL;
		fonts[FONT_ID::SMALL] = CreateFontIndirectW(&logfont);
	}

	~GDI_FONTS() {
		for(auto& font : fonts) {
			if(font) {
				DeleteObject(font);
			}
		}
	}
};
GDI_FONTS GDIFonts;

TEXTRENDER_GDI<GRAPHICS_DDRAW, SURFACE_DDRAW> TextObj = {
	DxObj, GrText, GDIFonts.fonts
};

PIXEL_SIZE DrawGrdFont(
	TEXTRENDER_RECT_ID rect_id, std::span<const Narrow::string_view> strs
)
{
	PIXEL_SIZE extent = { 0, 0 };
	TextObj.Prerender(rect_id, [&](TEXTRENDER_SESSION auto& s) {
		// ここら辺は、一種の常套手段か？ //
		const auto old = GetPixel(s.hdc, 0, 0);
		SetPixelV(s.hdc, 0, 0, RGB(255, 255, 255));
		const auto temp = GetPixel(s.hdc, 0, 0);
		SetPixelV(s.hdc, 0, 0, old);

		s.SetFont(FONT_ID::NORMAL);
		for(const auto& str : strs) {
			s.Put({ (extent.w + 2), 2 }, str, RGBA{   0,   0, 128 });
			s.Put({ (extent.w + 1), 2 }, str, RGBA{   0,   0, 128 });
			s.Put({ (extent.w + 1), 1 }, str, RGBA{ 255, 255, 255 });
			s.Put({ (extent.w + 0), 1 }, str, RGBA{ 255, 255, 255 });
			extent += s.Extent(str);
		}

		constexpr auto FSIZE = 8;
		const PIXEL_COORD w = extent.w;
		const PIXEL_COORD h = extent.h;
		for(auto y = (s.rect.top + 1); y < (s.rect.top + h); y++) {
			const auto gradient = (255 + FSIZE - (y * FSIZE));
			const COLORREF color = RGB(gradient, gradient, 255);
			for(auto x = (s.rect.left + 1); x < (s.rect.left + w); x++) {
				if(GetPixel(s.hdc, x, y) == temp) { // RGB(255, 255, 255)
					SetPixelV(s.hdc, x, y, color);
				}
			}
		}
	});
	return extent;
}
