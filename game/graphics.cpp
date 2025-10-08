/*
 *   Common graphics interface, independent of a specific rendering API
 *
 */

// GCC 15 throws `error: conflicting declaration 'typedef struct max_align_t
// max_align_t'` if this appears after a module import.
#include <webp/encode.h>

#include <SDL3/SDL_surface.h>

#include "game/graphics.h"
#include "game/defer.h"
#include "game/format_bmp.h"
#include "game/input.h"
#include "game/string_format.h"
#include "platform/file.h"
#include "platform/path.h"
#include "platform/graphics_backend.h"

uint8_t Grp_FPSDivisor = 0;
std::chrono::steady_clock::duration Grp_ScreenshotTimes[
	GRP_SCREENSHOT_EFFORT_COUNT
];

// Paletted graphics //
// ----------------- //

PALETTE PALETTE::Fade(uint8_t alpha, uint8_t first, uint8_t last) const
{
	PALETTE ret = *this;
	const uint16_t a16 = alpha;
	const auto src_end = (cbegin() + last + 1);
	for(auto src_it = (cbegin() + first); src_it < src_end; src_it++) {
		ret[src_it - cbegin()] = RGBA{
			.r = static_cast<uint8_t>((src_it->r * a16) / 255),
			.g = static_cast<uint8_t>((src_it->g * a16) / 255),
			.b = static_cast<uint8_t>((src_it->b * a16) / 255),
		};
	}
	return ret;
}

void Grp_PaletteSetDefault(void)
{
	if(GrpBackend_PixelFormat().IsChanneled()) {
		return;
	}
	PALETTE palette = {0};
	RGB216::ForEach([&](const RGB216& col) {
		const auto index = col.PaletteIndex();
		palette[index].r = (col.r * (255 / RGB216::MAX));
		palette[index].g = (col.g * (255 / RGB216::MAX));
		palette[index].b = (col.b * (255 / RGB216::MAX));
	});
	GrpBackend_PaletteSet(palette);
}
// ----------------- //

// Screenshots
// -----------

using NUM_TYPE = unsigned int;

static NUM_TYPE ScreenshotNum = 0;
static std::u8string ScreenshotBuf;

void Grp_ScreenshotSetPrefix(std::u8string_view prefix)
{
	const auto cap = (prefix.length() + STRING_NUM_CAP<NUM_TYPE> + 5);
	ScreenshotBuf.resize_and_overwrite(cap, [&](auto *p, size_t) {
		return (std::ranges::copy(prefix, p).out - p);
	});
}

// Increments the screenshot number to the next file with the given extension
// that doesn't exist yet, then opens a write stream for that file.
std::unique_ptr<FILE_STREAM_WRITE> Grp_NextScreenshotStream(
	std::u8string_view ext
)
{
	if(ScreenshotBuf.size() == 0) {
		return nullptr;
	}

	// Users might delete the directory while the game is running, after all.
	if(!SDL_CreateDirectory(ScreenshotBuf.c_str())) {
		ScreenshotBuf.clear();
		return nullptr;
	}

	// Prevent the theoretical infinite loop...
	while(ScreenshotNum < (std::numeric_limits<NUM_TYPE>::max)()) {
		const auto prefix_len = ScreenshotBuf.size();
		StringCatNum<4>(ScreenshotNum++, ScreenshotBuf);
		ScreenshotBuf += ext;
		auto ret = FileStreamWrite(
			ScreenshotBuf.c_str(), FILE_FLAGS::FAIL_IF_EXISTS
		);
		ScreenshotBuf.resize(prefix_len);
		if(ret) {
			return ret;
		}
	}
	return nullptr;
}

static bool ScreenshotSaveBMP(SDL_Surface *src)
{
	if(!BMPSaveSupports(src->format)) {
		assert(!"Unsupported pixel format?");
		return false;
	}
	assert(src->w < std::numeric_limits<PIXEL_COORD>::max());
	assert(src->h < std::numeric_limits<PIXEL_COORD>::max());
	const auto stream = Grp_NextScreenshotStream(u8".BMP");
	if(!stream) {
		return false;
	}

	std::array<BGRA, BMP_PALETTE_SIZE_MAX> bgra_memory;
	const auto palette = [src, &bgra_memory]() -> std::span<BGRA> {
		const auto *palette = SDL_GetSurfacePalette(src);
		if(!palette) {
			return {};
		}
		assert(palette->ncolors == (sizeof(uint8_t) << 8));
		for(const int i : std::views::iota(0, palette->ncolors)) {
			bgra_memory[i] = {
				.b = palette->colors[i].b,
				.g = palette->colors[i].g,
				.r = palette->colors[i].r,
				.a = palette->colors[i].a,
			};
		}
		return bgra_memory;
	}();

	const PIXEL_SIZE bmp_size = { .w = src->w, .h = -src->h };

	// SDL_BITSPERPIXEL() for `SDL_PIXELFORMAT_XRGB8888` would return 24, not
	// 32!
	const auto bpp = (SDL_BYTESPERPIXEL(src->format) * 8);

	const auto pixels = std::span(
		static_cast<std::byte *>(src->pixels), (src->h * src->pitch)
	);
	return BMPSave(stream.get(), bmp_size, 1, bpp, palette, pixels);
}

static bool ScreenshotSaveWebP(SDL_Surface *src, int z)
{
	if((src->w > WEBP_MAX_DIMENSION) || (src->h > WEBP_MAX_DIMENSION)) {
		return false;
	}

	WebPPicture pic;
	if(!WebPPictureInit(&pic)) {
		return false;
	}
	defer(WebPPictureFree(&pic));

	pic.width = src->w;
	pic.height = src->h;
	pic.argb_stride = src->w;

	// Must also be set to opt into lossless import!
	pic.use_argb = true;

	decltype(WebPPictureImportRGBX) *import_func_32bpp = nullptr;
	switch(src->format) {
	case SDL_PIXELFORMAT_ARGB8888:
		// Yup, "argb" is little-endian and this is actually BGRA...
		pic.argb = std::bit_cast<uint32_t *>(src->pixels);
		break;
	case SDL_PIXELFORMAT_XRGB8888:
		// … but these are big-endian!
		import_func_32bpp = WebPPictureImportBGRX;
		break;
	case SDL_PIXELFORMAT_ABGR8888:
		import_func_32bpp = WebPPictureImportRGBA;
		break;
	case SDL_PIXELFORMAT_INDEX8: {
		// The WebP repo has equivalent code in WebPImportColorMappedARGB(),
		// but Linux distributions typically don't package the `extras` module
		// this function belongs to.
		const auto *palette = SDL_GetSurfacePalette(src);
		if(!palette) {
			return false;
		}
		assert(palette->ncolors == (sizeof(uint8_t) << 8));
		if(!WebPPictureAlloc(&pic)) {
			return false;
		}
		auto *src_p = std::bit_cast<uint8_t *>(src->pixels);
		auto *dst_p = pic.argb;
		for(const auto y : std::views::iota(0, pic.height)) {
			for(const auto x : std::views::iota(0, pic.width)) {
				const auto c = palette->colors[src_p[x]];
				dst_p[x] = ((c.b << 0) | (c.g << 8) | (c.r << 16) | 0xFF000000);
			}
			src_p += src->w;
			dst_p += pic.argb_stride;
		}
		break;
	}
	default:
		return false;
	}
	if(import_func_32bpp) {
		const auto *bytes = std::bit_cast<uint8_t *>(src->pixels);
		if(!import_func_32bpp(&pic, bytes, (src->w * sizeof(uint32_t)))) {
			return false;
		}
	}

	WebPConfig config;
	if(!WebPConfigInit(&config)) {
		return false;
	}
	if(!WebPConfigLosslessPreset(&config, z)) {
		return false;
	}
	config.thread_level = 1;

	WebPMemoryWriter wrt;
	WebPMemoryWriterInit(&wrt);
	defer(WebPMemoryWriterClear(&wrt));
	pic.writer = WebPMemoryWrite;
	pic.custom_ptr = &wrt;

	const auto ret = WebPEncode(&config, &pic);
	if(!ret) {
		return false;
	}
	const auto stream = Grp_NextScreenshotStream(u8".webp");
	if(!stream) {
		return false;
	}
	return stream->Write({ wrt.mem, wrt.size });
}

bool Grp_ScreenshotSave(
	SDL_Surface *src, const std::chrono::steady_clock::time_point t_start
)
{
	constexpr auto DURATION_FAILED = std::chrono::steady_clock::duration(-1);

	assert(src->w >= 0);
	assert(src->h >= 0);
	if(SDL_MUSTLOCK(src)) {
		SDL_LockSurface(src);
	}

	auto ret = false;
	auto effort = Grp_ScreenshotEffort;
	if(effort != 0) {
		ret = ScreenshotSaveWebP(src, (effort - 1));
		if(!ret) {
			Grp_ScreenshotTimes[effort] = DURATION_FAILED;
		}
	}
	if(!ret) {
		effort = 0;
		ret = ScreenshotSaveBMP(src);
	}
	const auto t_end = std::chrono::steady_clock::now();
	Grp_ScreenshotTimes[effort] = (ret ? (t_end - t_start) : DURATION_FAILED);

	if(SDL_MUSTLOCK(src)) {
		SDL_UnlockSurface(src);
	}

	return ret;
}
// -----------

GRAPHICS_FULLSCREEN_FLAGS GRAPHICS_PARAMS::FullscreenFlags(void) const
{
	using F = GRAPHICS_PARAM_FLAGS;
	return {
		.fullscreen = !!(flags & F::FULLSCREEN),
		.exclusive = !!(flags & F::FULLSCREEN_EXCLUSIVE),
		.fit = static_cast<GRAPHICS_FULLSCREEN_FIT>(
			std::to_underlying(flags & F::FULLSCREEN_FIT) >> 2
		),
	};
}

bool GRAPHICS_PARAMS::ScaleGeometry(void) const
{
	return !!(flags & GRAPHICS_PARAM_FLAGS::SCALE_GEOMETRY);
}

uint8_t GRAPHICS_PARAMS::Scale4x(void) const
{
	const auto fs = FullscreenFlags();
	if(fs.fullscreen) {
		return (fs.exclusive ? 4 : 0);
	}
	return window_scale_4x;
}

WINDOW_SIZE GRAPHICS_PARAMS::ScaledRes(void) const
{
	const auto fs = FullscreenFlags();
	if(fs.fullscreen) {
		if(fs.exclusive) {
			return GRP_RES;
		}
		const auto display_s = GrpBackend_DisplaySize(true);
		switch(fs.fit) {
		case GRAPHICS_FULLSCREEN_FIT::INTEGER: {
			const auto factors = (display_s / GRP_RES);
			return (GRP_RES * std::min(factors.w, factors.h));
		}
		case GRAPHICS_FULLSCREEN_FIT::ASPECT: {
			const auto factor_w = (static_cast<float>(display_s.w) / GRP_RES.w);
			const auto factor_h = (static_cast<float>(display_s.h) / GRP_RES.h);
			const auto scale = std::min(factor_w, factor_h);
			return {
				.w = static_cast<PIXEL_COORD>(GRP_RES.w * scale),
				.h = static_cast<PIXEL_COORD>(GRP_RES.h * scale),
			};
		}
		case GRAPHICS_FULLSCREEN_FIT::STRETCH:
			return display_s;
		case GRAPHICS_FULLSCREEN_FIT::COUNT:
			std::unreachable();
		}
	}
	const auto scale = ((window_scale_4x == 0)
		? Grp_WindowScale4xMax()
		: window_scale_4x
	);
	return ((GRP_RES * scale) / 4);
}

void GRAPHICS_PARAMS::SetFlag(
	GRAPHICS_PARAM_FLAGS flag,
	std::underlying_type_t<GRAPHICS_PARAM_FLAGS> value
)
{
	EnumFlagSet(flags, flag, value);
}

uint8_t Grp_WindowScale4xMax(void)
{
	const auto factors = ((GrpBackend_DisplaySize(false) * 4) / GRP_RES);
	return std::min(factors.w, factors.h);
}

std::optional<GRAPHICS_INIT_RESULT> Grp_Init(
	std::optional<const GRAPHICS_PARAMS> maybe_prev, GRAPHICS_PARAMS params
)
{
	const auto api_count = GrpBackend_APICount();
	const auto device_count = GrpBackend_DeviceCount();
	if((api_count > 0) && (params.api >= api_count)) {
		params.api = -1;
	}
	params.device_id = std::min(GrpBackend_DeviceCount(), uint8_t{ 0 });
	return GrpBackend_Init(maybe_prev, params);
}

std::optional<GRAPHICS_INIT_RESULT> Grp_InitOrFallback(GRAPHICS_PARAMS params)
{
	if(const auto ret = Grp_Init(std::nullopt, params)) {
		return ret;
	}

	// Start with the defaults and try looking for a different working
	// configuration
	const auto api_count = GrpBackend_APICount();
	const auto device_count = std::max(GrpBackend_DeviceCount(), uint8_t{ 1 });

	const auto api_it = ((api_count > 0)
		? std::views::iota(int8_t{ -1 }, api_count)
		: std::views::iota(params.api, Cast::down<int8_t>(params.api + 1))
	);

	for(const auto api : api_it) {
		for(const auto device_id : std::views::iota(0u, device_count)) {
			params.api = api;
			params.device_id = device_id;
			if(const auto ret = Grp_Init(std::nullopt, params)) {
				return ret;
			}
		}
	}
	return std::nullopt;
}

void Grp_Flip(void)
{
	GrpBackend_Flip((SystemKey_Data & SYSKEY_SNAPSHOT) && ScreenshotBuf.size());
}
