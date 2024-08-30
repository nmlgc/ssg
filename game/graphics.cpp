/*
 *   Common graphics interface, independent of a specific rendering API
 *
 */

#include "game/graphics.h"
#include "game/input.h"
#include "game/string_format.h"
#include "platform/file.h"
#include "platform/graphics_backend.h"

uint8_t Grp_FPSDivisor = 0;

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
static constexpr std::u8string_view EXT = u8".BMP";

static NUM_TYPE ScreenshotNum = 0;
static std::u8string ScreenshotBuf;

void Grp_SetScreenshotPrefix(std::u8string_view prefix)
{
	const auto cap = (prefix.length() + STRING_NUM_CAP<NUM_TYPE> + EXT.size());
	ScreenshotBuf.resize_and_overwrite(cap, [&](auto *p, size_t) {
		return (std::ranges::copy(prefix, p).out - p);
	});
}

// Increments the screenshot number to the next file that doesn't exist yet,
// then opens a write stream for that file.
std::unique_ptr<FILE_STREAM_WRITE> Grp_NextScreenshotStream()
{
	if(ScreenshotBuf.size() == 0) {
		return nullptr;
	}

	// Prevent the theoretical infinite loop...
	while(ScreenshotNum < (std::numeric_limits<NUM_TYPE>::max)()) {
		const auto prefix_len = ScreenshotBuf.size();
		StringCatNum<4>(ScreenshotNum++, ScreenshotBuf);
		ScreenshotBuf += EXT;
		auto ret = FileStreamWrite(ScreenshotBuf.c_str(), true);
		ScreenshotBuf.resize(prefix_len);
		if(ret) {
			return ret;
		}
	}
	return nullptr;
}
// -----------

uint8_t GRAPHICS_PARAMS::Scale4x(void) const
{
	return window_scale_4x;
}

WINDOW_SIZE GRAPHICS_PARAMS::ScaledRes(void) const
{
	const auto scale = ((Scale4x() == 0)
		? Grp_WindowScale4xMax()
		: Scale4x()
	);
	return ((GRP_RES * scale) / 4);
}

uint8_t Grp_WindowScale4xMax(void)
{
	const auto factors = ((GrpBackend_DisplaySize() * 4) / GRP_RES);
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
		: std::views::iota(params.api, int8_t{ params.api + 1 })
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
	GrpBackend_Flip((SystemKey_Data & SYSKEY_SNAPSHOT)
		? Grp_NextScreenshotStream()
		: nullptr
	);
}
