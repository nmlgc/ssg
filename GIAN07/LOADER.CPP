/*                                                                           */
/*   LOADER.cpp   グラフィック、サウンド等のロード                           */
/*                                                                           */
/*                                                                           */

#include "LOADER.H"
#include "CONFIG.H"
#include "ENEMY.H"
#include "GAMEMAIN.H"
#include "GIAN.H"
#include "SCROLL.H"
#include "WindowSys.h"
#include "ssg/Sound.h"
#include "ssg/internal/LZ.hpp"
#include "ssg/internal/Stage.hpp"
#include "hatoyama/engine/format_bmp.h"
#include "hatoyama/engine/path.h"
#include "hatoyama/engine/snd.h"
#include "hatoyama/engine/thread.h"
#include <assert.h>

// Hardcoded loop points for ZUN's original MIDI files
// ---------------------------------------------------

struct MID_LOOP_FOR_HASH {
	HASH hash;
	MID_LOOP loop;
};

// These loop points were detected using https://github.com/nmlgc/mly, with the
// command line
//
// 	<file.mid mly loop-find
//
// unless otherwise specified.
static constinit const auto LOOPS = HashesSorted<MID_LOOP_FOR_HASH, 65>({{
	// Original soundtrack (MUSIC.DAT)
	// -------------------------------
	// clang-format off

	/* #01 秋霜玉　～ Clockworks */
	{ "04a44d2751f0cc155b9dbcabf7886bf999801f27f31ffee8566a18449e95ba4f"_B3, {  16557,  94317 } },
	/* #02 フォルスストロベリー */
	{ "e040b0ae4a9a36be23f88d5b0f66c6c5e60f0dfe648404c143f9095b75d1f036"_B3, {  75361, 183841 } },
	/* #03 プリムローズシヴァ */
	{ "4727240794872e31d2591850b0d662af13fa25a77d9798c925cca61da114df43"_B3, {  31561,  93001 } },
	/* #04 幻想帝都 */
	{ "d01300e4053bb07abc909fba83b0b1addb00104ee1ac3c9ad981e348b9c66622"_B3, {   4800,  89280 } },
	/* #05 ディザストラスジェミニ */
	{ "694543bea96390d6a6e85771e19561853515f456839f2672516516a6fbd074a4"_B3, {   1184,  77984 } },
	/* #06 華の幻想　紅夢の宙 */
	{ "b58f7178ca77351f8d014efce152c50f6057adabbd969efe93610b65161f2a2e"_B3, {  73666, 181186 } },
	/* #07 天空アーミー */
	{ "ac10269db4ad75f752fe8a52531238871baf8dcd87f6b2e90d130645e9bf0841"_B3, {   8640, 104640 } },
	/* #08 スプートニク幻夜 */
	{ "c8402fa2769f9341fb71db27d24550d5e534a652e71a2f21e45417a3a5764f3a"_B3, {  31606, 158326 } },
	/* #09 機械サーカス　～ Reverie */
	{ "bde78f7de7bb640ce3e65589b7ccc79b4b702a32475c902eaf04170b6b1da538"_B3, {    961,  93121 } },
	/* #10 カナベラルの夢幻少女 */
	{ "c804078d44983f3163220d51f22c035dddc19e78e5bbd129564bcb66447096ae"_B3, { 108481, 223681 } },
	/* #11 魔法少女十字軍 */
	{ "203b58f72fe30a532575de19ba58de11d0836d20c033eead82c4d4575f359ad1"_B3, {   8641, 127681 } },
	/* #12 アンティークテラー */
	{ "0a0a45aa7bae3a6b7ebb082970960e1679c4ae4ce5c3e011d12e73899b99390f"_B3, {   1198, 140398 } },
	/* #13 夢機械　～ Innocent Power */
	{ "052382df9912024fc1bcb11c60ca4555998d7206be2f7beefddef22e80ad5e3d"_B3, {    961,  62401 } },
	/* #14 幻想科学 ～ Doll's Phantom */
	{ "0b8a97180a2229c42d22556c734668c8c9ccec9d1f32e0c936d7e7bb7a1fddab"_B3, {  75601, 185041 } },
	/* #15 少女神性　～ Pandora's Box */
	{ "109d226ed66538074c2e15d4637631e77506321a66b644e627c77fd91d94ce3e"_B3, {  90510, 183630 } },
	/* #16 シルクロードアリス */
	{ "46c876e99f605c5c8e5957d6adcecc2231c5e795f7284cca7f496a42a3758b2f"_B3, {   6721, 141121 } },
	/* #17 魔女達の舞踏会　～ Magus */
	{ "a0a5ccd7c1b0e78c0365f5290fbf453ddc3d97298a1cf1f6ccc66365785d69c4"_B3, {    961,  77761 } },
	/* #18 二色蓮花蝶　～ Ancients */
	{ "67b2b4690067193ea1703569eaa0eeada59a6577ff10f4e0fb7f7b0e55559435"_B3, {   2880, 144960 } },
	/* #19 ハーセルヴス */
	{ "f8cfe5c314ad1d8b7ed435cd876dae2b7750c00f03c1fa9a748428344feaa27e"_B3, {  86401, 101761 } },
	/* #20 タイトルドメイド */
	{ "4931e9b4220ecd94d6006fec0805c882f3409840af15dc5a5336d36a65e4e70f"_B3, {    961,  43201 } },
	// -------------------------------

	// Arranged soundtrack (https://www16.big.or.jp/~zun/html/music_old.html)
	// ----------------------------------------------------------------------
	// The first row corresponds to ZUN's original MIDIs, the second and third
	// rows to the edited non-echo and echo versions in the arranged soundtrack
	// BGM packs.

	/* #01 秋霜玉　～ Clockworks */
	{ "02838ce71bcb2922278d86331af10caebb893ad4dbd7bf66771501dd16640fda"_B3, {  16321,  94081 } },
	{ "cf98509de1158239e06b0e4cba330b47ef10c9d6325aff13fb57f3e1c177308a"_B3, {  16321,  94081 } },
	{ "de54464adade220c4e682d833160b768cbdf7c86a74061064472bb9d2e700799"_B3, {  16321,  94081 } },
	/* #02 フォルスストロベリー
		<ssg_02.mid mly cut 466: | mly loop-unfold 240: | mly loop-find */
	{ "2cada452b1d1d1cbcff2e2f2430217fcbbcf4887e44dcbbd7c48d99ee39771f4"_B3, {  75361, 183841 } },
	{ "1e6532a487f574b28d8540d46245884030812c2fba36d86722e6f6ba61feb8d8"_B3, {  75361, 183841 } },
	{ "3fd235970a395feba43fac52b8843d225d2828cb9f178ac73833ae26b6e4492c"_B3, {  75361, 183841 } },
	/* #03 プリムローズシヴァ
	   Uses Reverb Macro 0 (Room 1), doesn't need an echo edit. */
	{ "63b0ed5d24e83b20ca603052e01a427477a89d9ec1903b188bd184eed09cf034"_B3, {    961,  62401 } },
	{ "ff336846794befbbf4188dcdc7496b3f029896b343b8978553527ecc324677fe"_B3, {    961,  62401 } },
	/* #04 幻想帝都
		<ssg_04.mid mly smf0 | mly loop-find */
	{ "2232a7c30b6bee76709342c62997fafab08a483fcde9d3595ba3b0e5d1819d18"_B3, {   4800,  89280 } },
	{ "8b5f5b50209c725fe7cc6b5506d8f57d5df91cf6fa135e88ca9c6f22dbfda2ea"_B3, {   4800,  89280 } },
	{ "b7c704265773d3f2efa64dbda283c6e0a45ac8d0e28075136d2152e01936a446"_B3, {   4800,  89280 } },
	/* #05 ディザストラスジェミニ
		<ssg_05.mid mly smf0 | mly cut 386: | mly loop-unfold 226: | mly loop-find */
	{ "26b734bec6e53b3ca02ce82d67f5bc2473f892ac7bc3b0f92703f66e55bfef17"_B3, {  62520, 139320 } },
	{ "6cb425f3501c6c36dbf563f2f515a99e5cabd9772b198f8a5720a2871a178e5f"_B3, {  62520, 139320 } },
	{ "eb2a7526fc9f7d1bbd86aeb99f5055b7d2f75d99240746fdab159645eb0fd1d3"_B3, {  62520, 139320 } },
	/* #06 華の幻想　紅夢の宙
		<ssg_06.mid mly cut 494: | mly loop-unfold 270: | mly loop-find */
	{ "6e53bcd7c38a0e54d167d673ecbf0b47404442832c55b858c22edb1b63747939"_B3, {  73681, 181201 } },
	{ "f0f4ce32eb747727ca160da4aab6b6432f9365d3b7c76ea411c089d79e87d2ef"_B3, {  73681, 181201 } },
	{ "4f24b13ad086126c0825363aafd7f6036b042a828c0ef18cd7d42db7989e8300"_B3, {  73681, 181201 } },
	/* #07 天空アーミー */
	{ "adba179af78f437b209eef82dc5f043087dfc42f8e584bd7688a57f510a3c26a"_B3, {   8640, 104640 } },
	{ "9c08d7f4dee7bc344431bf7e1a1edbada0593b41adb9046e19dc890d8c1c4bf2"_B3, {   8640, 104640 } },
	{ "f6ff4062e72a2ddf9cd04a1376446b7a87de65b29b30d8cc2370dc328e741dee"_B3, {   8640, 104640 } },
	/* #08 スプートニク幻夜 */
	{ "75087ec2ce1237d6dfe62e543d174b558343a8d2b09a0e7e767f0fead08644ea"_B3, {  31606, 158326 } },
	{ "07f3114ca2cb648fbcddeb584d9c5c4522ccc05bfca2723267c91de6e87c50c9"_B3, {  31606, 158326 } },
	{ "47dc15bc5bf03e6cd09703bd546a387f8bee58ce9c6b1f45946c74c98745adce"_B3, {  31606, 158326 } },
	/* #09 機械サーカス　～ Reverie
		Every supposed loop modulates up by a semitone 16 measures before it
		ends and remains in that new key at the start of the next loop, so the
		piece technically doesn't loop at all. The original stays in G♯m
		throughout. */

	/* #10 カナベラルの夢幻少女
		<ssg_10.mid mly smf0 | mly loop-find */
	{ "d959e251a069bb198c8c79dcdd61dacfdbb63041bd163018bf4befd3331d30d1"_B3, {    961, 116161 } },
	{ "5bb79c1e0c1fbf94f11a980bed4f43a55107eafa3d9f427f5043101a300b0b3d"_B3, {    961, 116161 } },
	{ "58faba5c7f48a29dae657979b139a2fa55335652cff116ddb4a5fe5b25722a7f"_B3, {    961, 116161 } },
	/* #11 魔法少女十字軍 */
	{ "b70b6b7ed80b1c605a4b97b26a9d1c564ecab3699e980ba512b40e947260e77c"_B3, {   8641, 127681 } },
	{ "1e04bea18f35790e7a8ada516b9f8f98f6ecf64163f604f7c0f8f0699a4952bc"_B3, {   8641, 127681 } },
	{ "cdc8c3fbd717048a98cb5f92fabeef78a42892a3fa9ef91ea2c0396049aa659c"_B3, {   8641, 127681 } },
	/* #12 アンティークテラー
		<ssg_12.mid mly cut 602: | mly loop-unfold 312: | mly loop-find */
	{ "15eca8c0ea2e60e752a93cbcb09043a7213b4b7993b8493d529a4cd440176c63"_B3, {  16081, 155281 } },
	{ "b81e3526f2f6690664106f825d3af8d21a843526e1ceb32191f4e9e82298f1ec"_B3, {  16081, 155281 } },
	{ "5b7dfe45bf71fd138f9b439c4fbfeb4f990a9d96c8abd5c41f80317d36875bb7"_B3, {  16081, 155281 } },
	/* #13 夢機械　～ Innocent Power
		Has a unique ending section that starts in Gm and then modulates
		through Em and Fm before it fades out on F♯m. */

	/* #14 幻想科学 ～ Doll's Phantom
		<ssg_14.mid mly cut 550: | mly loop-unfold 322: | mly loop-find */
	{ "3b94f8d5b87cfc2db73dee57744d2de5a4a0daa67cb8b122b0fef26f4fc5cd63"_B3, {  75601, 185041 } },
	{ "acdd1bff05f7fdab9ba6569fd7512c58038be69aef26f7b59cf3ed9463f0cdf8"_B3, {  75601, 185041 } },
	{ "bbd1bfefdd54506f92e1a3b5d77c61884ff11ae2e41512568d36fa7e1507ac1c"_B3, {  75601, 185041 } },
	/* #15 少女神性　～ Pandora's Box
		<ssg_15.mid mly cut 522: | mly loop-unfold 328: | mly loop-find */
	{ "7728681d569155c34a71e28e3c75fc104d7b439835fb33fad2a94b6c46e8fe59"_B3, {  90220, 183340 } },
	{ "581ee417e1b3aa578a26c2acc56dd2cffb3bdde722877cfe80d56486122463b4"_B3, {  90220, 183340 } },
	{ "0e401c20edc6dbc29c7b9f968e4f9686e09218b22400035261f71aa3ef123551"_B3, {  90220, 183340 } },
	/* #16 シルクロードアリス
		<ssg_16.mid mly cut 624: | mly loop-unfold 344: | mly loop-find */
	{ "384f15ef0325b4806f878e5144cccf2a502217705106e2b7e918a4106554e74c"_B3, {  30721, 165121 } },
	/*	    <16.mid mly cut 614: | mly loop-unfold 334: | mly loop-find
	   Uses custom reverb settings, doesn't need an echo edit. */
	{ "16732bcc91a128f1a9dd5595f99f8fc0b36a3c5781a50e9eeaecc491d08fb89c"_B3, {  25921, 160321 } },
	/* #17 魔女達の舞踏会
		Has a unique 8-bar ending section that first appears in Cm and then
		loops in C♯m while fading out. The fade starts during the first loop,
		so we can't loop this section in-game (unless Romantique Tp were to
		re-record it without the fade). */

	/* #18 二色蓮花蝶　～ Ancients
	   Uses Reverb Macro 1 (Room 2), doesn't need an echo edit. */
	{ "9a695e4659a1293e2d08fe287eb55f4dc6279beab2317004ae520029e552d602"_B3, {   3841, 145921 } },
	{ "3adb2cc55b56bb773704ad564546a07b43df0c7bc4e80928dd03d4aad03a27ce"_B3, {   2881, 144961 } },
	/* #19 ハーセルヴス
		Features a unique and very beautiful ending section. Let's let it
		fade out to silence for dramatic effect. */
	{ "0f29ba3a086246621cf0624638a044fc6c6622fee49744da66cf4ae8641d3475"_B3, {     -1,     -1 } },
	{ "276e1fa39fe52368986bed512b3a248034e17ab79b23b00122be3cd2ed3a6187"_B3, {     -1,     -1 } },
	{ "5e596cc00a3c99d73e301c858b085f34b13ad33be9417368eec185e7e53b5848"_B3, {     -1,     -1 } },
	// clang-format on
	// ----------------------------------------------------------------------
}});

bool LoadMIDIWithPotentialLoop(BUFFER_OWNED buf, const HASH& hash)
{
	const auto ret = Mid_Load(std::move(buf));
	if(!ret) {
		return false;
	}
	const auto loop_it = LOOPS.Lookup(hash);
	if(loop_it != LOOPS.end()) {
		Mid_SetLoop(loop_it->loop);
	}
	return ret;
}
// ---------------------------------------------------

// Packfile loading //
bool GrpBMPLoadP(const PACKFILE_READ& in, fil_no_t filno, SURFACE_ID sid)
{
	auto maybe_bmp = BMPLoad(in.MemExpand(filno));

	// If this fails, we're going to crash due to the uninitialized surface
	// anyway. Might as well announce it in debug mode.
	assert(maybe_bmp);

	// Still necessary to avoid generation of exception handling code in
	// Release mode.
	if(!maybe_bmp) {
		return false;
	}

	auto& bmp = maybe_bmp.value();
	return GrpSurface_Load(sid, std::move(bmp));
}

bool Snd_SELoadP(const PACKFILE_READ& in, fil_no_t filno, uint8_t id, int max)
{
	return Snd_SELoad(in.MemExpand(filno), id, max);
}

bool LoadSound(const PACKFILE_READ& in);

// Packfile cache //
// -------------- //

namespace DAT {
	enum class PACK_ID : uint8_t {
		ENEMY,
		GRAPH,
		GRAPH2,
		MUSIC,
		SOUND,
		COUNT,
	};

	constexpr ENUMARRAY<const std::u8string_view, PACK_ID> BASENAMES = {{
		u8"ENEMY.DAT",
		u8"GRAPH.DAT",
		u8"GRAPH2.DAT",
		u8"MUSIC.DAT",
		u8"SOUND.DAT",
	}};

	constexpr std::u8string_view NOT_FOUND = u8"☐ ";
	constexpr std::u8string_view FOUND = u8"☑ ";

	// Packfiles can be in these four states:
	//
	// 1) `THREAD` blank, `PACKFILE_READ` default-constructed:
	//    Uninitialized, at the start of the process.
	//
	// 2) `THREAD` joinable, `PACKFILE_READ` default-constructed:
	//    In the process of loading the packfile and running any post-loading
	//    code.
	//
	// 3) `THREAD` blank, `PACKFILE_READ` empty:
	//    Invalid packfile, after FilStartR() returned an error. Currently not
	//    distinguished from 1), but we might in the future.
	//
	// 4) `THREAD` blank, `PACKFILE_READ` valid:
	//    Checksums are validated, and the packfile is ready for extraction.
	//
	// A single `std::variant` per pack might look like the better way to
	// represent this, but actually comes with two drawbacks:
	// • Since `THREAD` rejoins on destruction, an attempt to change the
	//   variant type from within the thread would deadlock the program.
	// • Suddenly, we also have to handle the `valueless_by_exception` case in
	//   the Packfile() function, which can then no longer return the neat
	//   `const PACKFILE_READ&`.
	class PACK {
	private:
		PACKFILE_READ pack;
		BUFFER_OWNED pack_buf;
		THREAD load_thread;
		std::u8string filename_with_found_prefix;

	public:
		bool Load(std::u8string_view path_data, PACK_ID id);
		const std::u8string& FilenameWithFoundPrefix(void) const {
			return filename_with_found_prefix;
		}

		const PACKFILE_READ& BlockUntilLoaded(void) {
			if(load_thread.Joinable()) {
				load_thread.Join();
			}
			return pack;
		}

		void AbortLoading(void)
		{
			if(load_thread.Joinable()) {
				load_thread.Abort();
			}
		}
	};
	ENUMARRAY<PACK, PACK_ID> Packs;

	// For MUSIC.DAT, we want to start asynchronously calculating all hashes
	// once the process starts.
	std::vector<HASH> MusicHashes;

	const PACKFILE_READ& Packfile(PACK_ID id)
	{
		return Packs[id].BlockUntilLoaded();
	}

	void LoadMusicHashes(const PACKFILE_READ& in, const THREAD_STOP& st)
	{
		MusicNum = in.info.size();
		MusicHashes.reserve(MusicNum);

		for(auto i = 0; i < MusicNum; i++) {
			if(st) {
				break;
			}
			if(const auto file = in.MemExpand(i)) {
				if(st) {
					break;
				}
				MusicHashes.emplace_back(Hash(BUFFER_BORROWED{ file }));
			} else {
				assert(!"Failure extracting BGM file?");
			}
		}
	};

	bool PACK::Load(std::u8string_view path_data, PACK_ID id)
	{
		if(pack) {
			return true;
		} else if(filename_with_found_prefix.empty()) {
			static_assert(NOT_FOUND.size() == FOUND.size());
			const auto basename = BASENAMES[id];
			const auto cap = (
				NOT_FOUND.size() + path_data.size() + basename.size()
			);
			filename_with_found_prefix.resize_and_overwrite(
				cap, [&](char8_t* buf, size_t) {
					std::ranges::in_out_result p = { path_data.begin(), buf };
					p = std::ranges::copy(NOT_FOUND, p.out);
					p = std::ranges::copy(path_data, p.out);
					p = std::ranges::copy(basename, p.out);
					return (p.out - buf);
				}
			);
		}
		auto *stream = SDL_IOFromFile(
			(filename_with_found_prefix.c_str() + NOT_FOUND.size()), "rb"
		);
		if(!stream) {
			return false;
		}
		std::ranges::copy(FOUND, filename_with_found_prefix.begin());
		load_thread = ThreadStart([this, stream = std::move(stream), id](
			const THREAD_STOP& st
		) mutable {
			pack_buf = SDL_LoadFile_IO(stream, true);
			auto in = FilStartR(BUFFER_BORROWED{ pack_buf });
			if(id == PACK_ID::MUSIC) {
				LoadMusicHashes(in, st);
			} else if(id == PACK_ID::SOUND) {
				LoadSound(in);
			}
			pack = std::move(in);
		});
		return true;
	}

	bool Check(void)
	{
		const auto path_data = PathForData();
		bool ret = true;
		for(const auto i : std::views::iota(0u, BASENAMES.size())) {
			const auto id = Cast::down_enum<DAT::PACK_ID>(i);
			ret &= Packs[id].Load(path_data, id);
		}
		return ret;
	}

	// ｎ番目の曲をロードする //
	bool LoadMusic(fil_no_t filno)
	{
		const auto& music = Packs[PACK_ID::MUSIC].BlockUntilLoaded();
		if(filno >= MusicHashes.size()) {
			return false;
		}
		return LoadMIDIWithPotentialLoop(
			music.MemExpand(filno), MusicHashes[filno]
		);
	}

	bool LoadMusicByHash(const HASH& hash)
	{
		Packs[PACK_ID::MUSIC].BlockUntilLoaded();
		const auto ret = std::ranges::find(MusicHashes, hash);
		if(ret == MusicHashes.cend()) {
			return false;
		}
		return LoadMusic(ret - MusicHashes.begin());
	}
}
// -------------- //

// Missing packfile screen
// -----------------------

extern bool IsDraw();

namespace DAT_MISSING {
	constexpr Narrow::string_view TITLE = "Missing game data files";

	bool FoundAll = false;

	bool FnRecheck(INPUT_BITS key) {
		if((key == KEY_BOMB) || (key == KEY_ESC)) {
			return false;
		} else if(Input_OptionKeyDelta(key) && DAT::Check()) {
			FoundAll = true;
			return false;
		}
		return true;
	}

	constexpr auto CENTER = WINDOW_FLAGS::CENTER;
	WINDOW_LABEL Title = { TITLE.data(), CENTER };
	std::array<WINDOW_CHOICE, (DAT::BASENAMES.size() + 6)> Info = {{
		{ },
		{ },
		{ },
		{ },
		{ },
		{ },
		{ },
		{ "Must be provided from an original game copy.", "", CENTER },
		{ },
		{ "Recheck", "", FnRecheck, CENTER },
		{ "Quit", "", CWinExitFn, CENTER },
	}};
	WINDOW_MENU Menu = { std::span(Info), [](bool) {}, &Title };
	WINDOW_SYSTEM Window = { Menu };

	void Proc(bool& quit)
	{
		CWinMove(&Window);
		if(Window.State == CWIN_DEAD) {
			if(FoundAll) {
				SProjectInit();
			} else {
				quit = true;
			}
		}
		if(IsDraw()) {
			GrpBackend_Clear();
			CWinDraw(&Window);
			Grp_Flip();
		}
	}

	void Init(void)
	{
		for(const auto i : std::views::iota(0u, DAT::BASENAMES.size())) {
			const auto id = Cast::down_enum<DAT::PACK_ID>(i);
			const auto& title = DAT::Packs[id].FilenameWithFoundPrefix();
			Info[1 + i].Title = title;
		}
		const auto w = (std::max)(CWinTextExtent(TITLE).w, (std::ranges::max)(
			std::views::transform(Info, [](const auto& info) {
				return (CWinItemExtent(info.Title).w + 8);
			})
		));

		Grp_PaletteSetDefault();

		Window.Init(w);
		Window.OpenCentered(w, (Info.size() - 2));
		GameMain = Proc;
	}
}
// -----------------------

void LoaderInit(void)
{
	if(!DAT::Check()) {
		DAT_MISSING::Init();
	} else {
		// Call a different *Init() function here to quickly test a different
		// game state.
		SProjectInit();
	}
}

void LoaderCleanup(void)
{
	// Cleanly shut down any loading threads that might not have joined yet
	for(auto& pack : DAT::Packs) {
		pack.AbortLoading();
	}
}



// グローバル変数 //
uint32_t	MusicNum = 0;	// 曲数
PALETTE SProjectPalette;
PALETTE FacePalette[FACE_MAX];
PALETTE EndingPalette[ENDING_PIC_MAX];
static PALETTE EnemyPalette;



// 秘密の関数 //
static int LoadedStage = 0;

// あるステージのグラフィックをロードする //
bool LoadGraph(int stage)
{
	LoadedStage = stage;
	const auto& graph = DAT::Packfile(DAT::PACK_ID::GRAPH);

	// 音楽室用 //
	if(stage==GRAPH_ID_MUSICROOM){
		return (
			GrpBMPLoadP(graph, 0, SURFACE_ID::SYSTEM) &&
			GrpBMPLoadP(graph, (19 + 4), SURFACE_ID::MUSIC)
		);
	}
	// タイトル画面用 //
	if(stage==GRAPH_ID_TITLE){
		return (
			GrpBMPLoadP(graph, 0, SURFACE_ID::SYSTEM) &&
			GrpBMPLoadP(graph, (20 + 4), SURFACE_ID::TITLE)
		);
		// LoadPaletteFrom(SURFACE_ID::ENEMY);
	}
	// お名前登録画面用 //
	if(stage==GRAPH_ID_NAMEREGIST){
		return (
			GrpBMPLoadP(graph, 0, SURFACE_ID::SYSTEM) &&
			GrpBMPLoadP(graph, (21 + 4), SURFACE_ID::NAMEREG)
		);
	}
	// 西方Ｐｒｏｊｅｃｔ表示用 //
	if(stage==GRAPH_ID_SPROJECT){
		if(!GrpBMPLoadP(graph, 31, SURFACE_ID::SPROJECT)) {
			return false;
		}
		GrpBackend_PaletteGet(SProjectPalette);

		// if(!GrpBMPLoadP(graph, (21 + 4), SURFACE_ID::NAMEREG)) {
		// 	return false;
		// }
		return true;
	}
	// エンディング全画像ロード(パレット含む) //
	if(stage==GRAPH_ID_ENDING){
		const auto& in = DAT::Packfile(DAT::PACK_ID::GRAPH2);

		if(!GrpBMPLoadP(in, 0, SURFACE_ID::ENDING_CREDITS)) {
			return false;
		}
		for(auto i = 0; i < ENDING_PIC_MAX; i++) {
			if(!GrpBMPLoadP(in, (1 + i), (SURFACE_ID::ENDING_PIC + i))) {
				return false;
			}
			GrpBackend_PaletteGet(EndingPalette[i]);
		}
		return true;
	}


	// エキストラステージシステム用 //
	if(stage == STAGE_EXTRA) {
		if(!GrpBMPLoadP(graph, 0, SURFACE_ID::SYSTEM)) {
			return false;
		}
		if(!GrpBMPLoadP(graph, (27 + 1), SURFACE_ID::ENEMY)) {
			return false;
		}
		GrpBackend_PaletteGet(EnemyPalette);

		if(!GrpBMPLoadP(graph, 27, SURFACE_ID::MAPCHIP)) {
			return false;
		}

		// 諸事情により、ここにいるのです //
		if(!GrpBMPLoadP(graph, 26, SURFACE_ID::BOMBER)) {
			return false;
		}

		return true;
	}

	// エキストラステージボス用(1) //
	if(stage == GRAPH_ID_EXBOSS1){
		if(!GrpBMPLoadP(graph, 29, SURFACE_ID::ENEMY)) {
			return false;
		}
		GrpBackend_PaletteGet(EnemyPalette);
		return true;
	}

	// エキストラステージボス用(2) //
	if(stage == GRAPH_ID_EXBOSS2){
		if(!GrpBMPLoadP(graph, 30, SURFACE_ID::ENEMY)) {
			return false;
		}
		GrpBackend_PaletteGet(EnemyPalette);
		return true;
	}

	if((stage < 0) || (stage > STAGE_MAX)) {
		return false;
	}

	// マップチップのロードは後で変換すること //
	if(!GrpBMPLoadP(graph, 0, SURFACE_ID::SYSTEM)) {
		return false;
	}
	if(!GrpBMPLoadP(graph, (stage + 0), SURFACE_ID::ENEMY)) {
		return false;
	}
	GrpBackend_PaletteGet(EnemyPalette);

	// 本当は STAGE_MAX とすべき
	// const fil_no_t MapChipID[STAGE_MAX] = { 7, 7, 8, 9, 10, 11 };
	const fil_no_t MapChipID[STAGE_MAX] = { 7, 8, 9, 10, 11, 12 };
	if(!GrpBMPLoadP(graph, MapChipID[stage - 1], SURFACE_ID::MAPCHIP)) {
		return false;
	}

	// 諸事情により、ここにいるのです //
	return GrpBMPLoadP(graph, 26, SURFACE_ID::BOMBER);
}

void ReloadGraph(void)
{
	assert(LoadedStage != 0);
	LoadGraph(LoadedStage);
}

bool LoadFace(uint8_t FaceID, uint8_t FileNo)
{
	if(FaceID >= FACE_MAX) {
		return false;
	}
	const auto& graph = DAT::Packfile(DAT::PACK_ID::GRAPH);
	if(!GrpBMPLoadP(graph, (13 + FileNo), (SURFACE_ID::FACE + FaceID))) {
		return false;
	}

	// パレットを保存する //
	GrpBackend_PaletteGet(FacePalette[FaceID]);

	return true;
}

// 敵のパレットにする
extern void LoadPaletteFromEnemy(void)
{
	if(GrpBackend_PixelFormat().IsPalettized()) {
		GrpBackend_PaletteSet(EnemyPalette);
	}
}

void FreeStageData(void)
{
	// メモリを解放だ！ //
	StageSet(nullptr);
	ECL_Head = nullptr;
	ScrollInfo.DataHead = nullptr;
}

// ＥＣＬ&ＳＣＬデータ列をメモリ上にロードする //
bool LoadStageData(uint8_t stage)
{
	// Re-assigning the `std::unique_ptr`s only frees them one by one, and we
	// want them all freed in any case.
	FreeStageData();

	const auto& enemy = DAT::Packfile(DAT::PACK_ID::ENEMY);

	// エキストラステージシステム用 //
	if(stage == STAGE_EXTRA) {
		// ECL Load
		if(!enemy_set(enemy.MemExpand(24), stage)) {
			return false;
		}

		// SCL Load
		if(!StageSet(enemy.MemExpand(25))) {
			return false;
		}

		// MapData Load
		if((ScrollInfo.DataHead = enemy.MemExpand(26)) == nullptr) {
			return false;
		}
	}
	else{
		// 各データをロードする //
		if((stage < 1) || (stage > STAGE_MAX)) {
			return false;
		}

		// ECL Load
		if(!enemy_set(enemy.MemExpand(stage + 0 - 1), stage)) {
			return false;
		}

		// SCL Load
		if(!StageSet(enemy.MemExpand(stage + 6 - 1))) {
			return false;
		}

		// MapData Load
		if((ScrollInfo.DataHead = enemy.MemExpand(stage + 12 - 1)) == nullptr) {
			return false;
		}
	}

	// スクロール用変数の初期化 //
	ScrollInit();

	return true;
}

BUFFER_OWNED LoadEndingScript(void)
{
	FreeStageData();

	const auto& enemy = DAT::Packfile(DAT::PACK_ID::ENEMY);
	return enemy.MemExpand(47);
}


// ｎ番目の曲をロードする //
bool LoadMusic(unsigned int no)
{
	return DAT::LoadMusic(no);
}

bool LoadMusicByHash(const HASH& hash)
{
	return DAT::LoadMusicByHash(hash);
}

bool LoadMIDIBuffer(BUFFER_OWNED buf)
{
	const auto hash = Hash(buf.cursor());
	return LoadMIDIWithPotentialLoop(std::move(buf), hash);
}

// 全てのSoundデータをロードする //
bool LoadSound(const PACKFILE_READ& in)
{
	// サウンドの初期化 //
	// 何らかの理由で使用できなければ、Disable とする //
	if(!(ConfigDat.SoundFlags.v & SNDF_SE_ENABLE) || !Snd_SEInit()) {
		ConfigDat.SoundFlags.v &= (~SNDF_SE_ENABLE);
		return false;
	}

	while(1){
		if(!Snd_SELoadP(in,SOUND_ID_KEBARI  ,SOUND_ID_KEBARI  ,SNDMAX_KEBARI  )) break;
		if(!Snd_SELoadP(in,SOUND_ID_TAME    ,SOUND_ID_TAME    ,SNDMAX_TAME    )) break;
		if(!Snd_SELoadP(in,SOUND_ID_LASER   ,SOUND_ID_LASER   ,SNDMAX_LASER   )) break;
		if(!Snd_SELoadP(in,SOUND_ID_LASER2  ,SOUND_ID_LASER2  ,SNDMAX_LASER2  )) break;
		if(!Snd_SELoadP(in,SOUND_ID_BOMB    ,SOUND_ID_BOMB    ,SNDMAX_BOMB    )) break;
		if(!Snd_SELoadP(in,SOUND_ID_SELECT  ,SOUND_ID_SELECT  ,SNDMAX_SELECT  )) break;
		if(!Snd_SELoadP(in,SOUND_ID_HIT     ,SOUND_ID_HIT     ,SNDMAX_HIT     )) break;
		if(!Snd_SELoadP(in,SOUND_ID_CANCEL  ,SOUND_ID_CANCEL  ,SNDMAX_CANCEL  )) break;
		if(!Snd_SELoadP(in,SOUND_ID_WARNING ,SOUND_ID_WARNING ,SNDMAX_WARNING )) break;
		if(!Snd_SELoadP(in,SOUND_ID_SBLASER ,SOUND_ID_SBLASER ,SNDMAX_SBLASER )) break;
		if(!Snd_SELoadP(in,SOUND_ID_BUZZ    ,SOUND_ID_BUZZ    ,SNDMAX_BUZZ))     break;
		if(!Snd_SELoadP(in,SOUND_ID_MISSILE ,SOUND_ID_MISSILE ,SNDMAX_MISSILE )) break;
		if(!Snd_SELoadP(in,SOUND_ID_JOINT   ,SOUND_ID_JOINT   ,SNDMAX_JOINT   )) break;
		if(!Snd_SELoadP(in,SOUND_ID_DEAD    ,SOUND_ID_DEAD    ,SNDMAX_DEAD    )) break;
		if(!Snd_SELoadP(in,SOUND_ID_SBBOMB  ,SOUND_ID_SBBOMB  ,SNDMAX_SBBOMB  )) break;
		if(!Snd_SELoadP(in,SOUND_ID_BOSSBOMB,SOUND_ID_BOSSBOMB,SNDMAX_BOSSBOMB)) break;
		if(!Snd_SELoadP(in,SOUND_ID_ENEMYSHOT,SOUND_ID_ENEMYSHOT,SNDMAX_ENEMYSHOT)) break;
		if(!Snd_SELoadP(in,SOUND_ID_HLASER, SOUND_ID_HLASER, SNDMAX_HLASER))	break;
		if(!Snd_SELoadP(in,SOUND_ID_TAMEFAST, SOUND_ID_TAMEFAST, SNDMAX_TAMEFAST))	break;
		if(!Snd_SELoadP(in,SOUND_ID_WARP, SOUND_ID_WARP, SNDMAX_WARP))	break;

		// DirectSound can only apply volume onto loaded buffers.
		Snd_UpdateVolumes();
		return true;
	}

	ConfigDat.SoundFlags.v &= (~SNDF_SE_ENABLE);
	Snd_SECleanup();
	return false;
}

bool LoadSound(void)
{
	return LoadSound(DAT::Packfile(DAT::PACK_ID::SOUND));
}


BUFFER_OWNED LoadMusicRoomComment(int no)
{
	if((no < 0) || (no > 19)) {
		return nullptr;
	}
	return DAT::Packfile(DAT::PACK_ID::ENEMY).MemExpand(27 + no);
}

BUFFER_OWNED LoadDemo(int stage)
{
	return DAT::Packfile(DAT::PACK_ID::ENEMY).MemExpand(stage - 1 + 18);
}
