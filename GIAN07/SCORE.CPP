/*                                                                           */
/*   Score.cpp   スコア入出力関数                                            */
/*                                                                           */
/*                                                                           */

#include "LZ_UTY.H"
#include "LEVEL.H"
#include "SCORE.H"
#include <ranges>

using NR_SCORE_LIST = std::span<NR_NAME_DATA, NR_RANK_MAX>;
using NR_CONST_SCORE_LIST = std::span<const NR_NAME_DATA, NR_RANK_MAX>;

// constexpr uint64_t PBG_MASK_VALUE = 0x342d3a4de92c68ba;
// constexpr uint64_t PBG_MASK_VALUE = 0xe92cd3a4d8ba6342;
constexpr uint64_t PBG_MASK_VALUE = 0xb97eb2c6542d3a41;



const auto ScoreFileName = _PATH("秋霜SC.DAT");				// スコアデータ格納ファイル名
NR_SCORE_STRING		ScoreString[NR_RANK_MAX];	// スコアデータ文字列格納先

// スコアデータ格納先(名前空間で保護)
static std::unique_ptr<NR_SCORE_DATA> ScoreData = nullptr;

static bool LoadScoreData(void);	// スコアデータを読み込む
static void ReleaseScoreData(void);	// スコアデータを解放する
static std::optional<NR_SCORE_LIST> GetNList(uint8_t Dif);	// 難易度でポインタを振り分ける
static bool SetDefaultScoreData(void);	// スコアデータ初期値をセット

static bool _LoadSC(NR_SCORE_LIST NData, BIT_DEVICE_READ& bd);
static void _SaveSC(NR_CONST_SCORE_LIST NData, BIT_DEVICE_WRITE& bd);

template <std::unsigned_integral T> T _xGet(
	BIT_DEVICE_READ& bd, uint64_t& ExMask
)
{
	ExMask = (((ExMask & 0x800000000000000) >> 60) + (ExMask << 1));

	T ret = 0;
	if constexpr(sizeof(T) == 1) {
		ret = bd.GetBits(8);
	} else {
		for(const auto word : std::views::iota(0u, (sizeof(T) / 2))) {
			ret |= (static_cast<T>(bd.GetBits(16)) << (word * 16));
		}
	}
	return (ret - static_cast<T>(ExMask));
}

template <std::unsigned_integral T> void _xPut(
	BIT_DEVICE_WRITE& bd, T data, uint64_t& ExMask
)
{
	ExMask = (((ExMask & 0x800000000000000) >> 60) + (ExMask << 1));

	const auto temp = (data + static_cast<T>(ExMask));

	if constexpr(sizeof(T) == 1) {
		bd.PutBits(temp, 8);
	} else {
		for(const auto word : std::views::iota(0u, (sizeof(T) / 2))) {
			bd.PutBits((temp >> (word * 16)), 16);
		}
	}
}


// 現在のスコア列を取得する(名前挿入アリ) //
// NData == NULL の場合、挿入しません     //
int SetScoreString(NR_NAME_DATA *NData, uint8_t Dif)
{
//	static char			*WTable[4] = {"WIDE ","HOMING","LASER","?????"};
	NR_SCORE_STRING		*Res;
	int					i,num,rank;
	int64_t temp;

	Res = ScoreString;


	// スコアデータを読み込む //
	if(NData != nullptr) {
		rank = IsHighScore(NData, Dif);
		if(rank==0) return 0;
	}
	else{
		rank = NR_RANK_MAX;
	}

	// ポインタをセットするぞ //
	if(!LoadScoreData()) return 0;
	auto maybe_p = GetNList(Dif);
	if(!maybe_p) {
		ReleaseScoreData();
		return 0;
	}
	auto p = maybe_p.value();

	if((rank != 0) && (NData != nullptr)) {
		// まずは、スコアを下方向に押し出すのだ //
		for(i=NR_RANK_MAX-1; i>=rank; i--){
			p[i] = p[i-1];		// 構造体から構造体への代入
		}

		// 新しいデータを挿入だ //
		p[rank-1] = *NData;
	}

	// データの格納開始 //
	temp = num = 0;
	for(i=0; i<NR_RANK_MAX; i++){
		if(temp < p[i].Score){
			temp  = p[i].Score;
			num += 1;
		}

		Res[i].Rank = num;
		Res[i].x    = (640 + (50  + i*24*20)) << 6;
		Res[i].y    = (100 + i*48) << 6;
		Res[i].bMoveEnable = true;

		strcpy(Res[i].Name  , p[i].Name);

		//strcpy(Res[i].Weapon, WTable[p[i].Weapon % 4]);
		Res[i].Weapon = p[i].Weapon % 4;

		sprintf(Res[i].Score,"%11I64d",p[i].Score);
		sprintf(Res[i].Evade, "%6u", p[i].Evade);
		sprintf(Res[i].Stage,"%1d" ,p[i].Stage);
	}

	ReleaseScoreData();

	return rank;
}


// ０：ハイスコアでない  それ以外：順位 //
int IsHighScore(const NR_NAME_DATA *NData, uint8_t Dif)
{
	int				Rank;

	// ロードできないので失敗！ //
	if(!LoadScoreData()) return 0;

	// 難易度でポインタを振り分ける //
	const auto maybe_temp = GetNList(Dif);
	if(!maybe_temp) {
		ReleaseScoreData();
		return 0;
	}
	const auto temp = maybe_temp.value();

	// 該当個所はあるかな？ //
	for(Rank=0; Rank<NR_RANK_MAX ; Rank++){
		// スコアが等しい場合は、後から入ったほうが下の順位に //
		// なるようにするのだ（前作[秋霜玉(仮] と同じね）     //
		if(NData->Score > temp[Rank].Score) break;
	}
	if(Rank == NR_RANK_MAX) Rank = -1;

	// 解放を行う //
	ReleaseScoreData();

	// 順位を返す //
	return Rank+1;
}


// スコアデータを書き出す //
bool SaveScoreData(NR_NAME_DATA *NData, uint8_t Dif)
{
	int				Rank;

	// スコアデータを読み込む //
	Rank = IsHighScore(NData, Dif);

	// これは、ハイスコアじゃないね //
	if(Rank == 0) {
		return false;
	}

	// ポインタをセットする //
	if(!LoadScoreData()) {
		return false;
	}
	auto maybe_temp = GetNList(Dif);
	if(!maybe_temp) {
		ReleaseScoreData();
		return false;
	}
	auto temp = maybe_temp.value();

	// まずは、スコアを下方向に押し出す //
	for(auto i = (temp.size() - 1); i >= Rank; i--) {
		temp[i] = temp[i-1];		// 構造体から構造体への代入
	}

	// 新しいデータを挿入だ //
	temp[Rank-1] = *NData;

	// 実際にファイルに出力 //
	BIT_DEVICE_WRITE bd;
	_SaveSC(ScoreData->Easy,    bd);
	_SaveSC(ScoreData->Normal,  bd);
	_SaveSC(ScoreData->Hard,    bd);
	_SaveSC(ScoreData->Lunatic, bd);
	_SaveSC(ScoreData->Extra,   bd);
	ReleaseScoreData();

	return bd.Write(ScoreFileName);
}


// スコアデータを読み込む //
static bool LoadScoreData(void)
{
	bool bInit = false;

	// すでにロード済みの場合(これは失敗にしない) //
	if(ScoreData) {
		return true;
	}

	ScoreData = std::unique_ptr<NR_SCORE_DATA>(
		new (std::nothrow) NR_SCORE_DATA
	);
	if(ScoreData == nullptr) {
		return false;
	}

	// ビット読み込みモードでファイルを開く //
	auto bd = BitFilCreateR(ScoreFileName);
	while(1){
		if(!_LoadSC(ScoreData->Easy,    bd)) break;
		if(!_LoadSC(ScoreData->Normal,  bd)) break;
		if(!_LoadSC(ScoreData->Hard,    bd)) break;
		if(!_LoadSC(ScoreData->Lunatic, bd)) break;
		if(!_LoadSC(ScoreData->Extra,   bd)) break;

		bInit = true;
		break;
	}

	if(!bInit){
		// ファイルが存在しないか不正な場合、新たに作成する //
		// この時点では、ファイルに対して書き込みは行わない //
		return SetDefaultScoreData();
	}

	return true;
}


static void ReleaseScoreData(void)
{
	// 解放～ //
	ScoreData = nullptr;
}


// 難易度でポインタを振り分ける //
static std::optional<NR_SCORE_LIST> GetNList(uint8_t Dif)
{
	if(!ScoreData) {
		return {};
	}

	switch(Dif){
		case(GAME_EASY):	return ScoreData->Easy;
		case(GAME_NORMAL):	return ScoreData->Normal;
		case(GAME_HARD):	return ScoreData->Hard;
		case(GAME_LUNATIC):	return ScoreData->Lunatic;
		case(GAME_EXTRA):	return ScoreData->Extra;
		default:
			return {};
	}
}

// スコアデータ初期値をセット //
static bool SetDefaultScoreData(void)
{
	if(nullptr == ScoreData) {
		return false;
	}

	for(auto i = 0; i < (GAME_EXTRA + 1); i++) {
		auto maybe_temp = GetNList(i);
		if(!maybe_temp) {
			return false;
		}
		auto temp = maybe_temp.value();
		for(size_t j = 0; j < temp.size(); j++) {
			strcpy(temp[j].Name, "????????");
			temp[j].Score  = ((temp.size() - j) * 1200000); // 1500000;
			temp[j].Evade  = ((temp.size() - j) * 50);
			temp[j].Stage  = ((i < 4) ? (temp.size() - j) : 1);
			temp[j].Weapon = j % 3;
		}
	}

	return true;
}

static bool _LoadSC(NR_SCORE_LIST NData, BIT_DEVICE_READ& bd)
{
	uint64_t CheckSum = 0;
	uint64_t Mask = PBG_MASK_VALUE;
	uint8_t flag = 0;

	for(auto& nd : NData) {
		CheckSum = 0;
		if(flag != bd.GetBit()) return false;
		else                    flag = 1-flag;

		// 名前を獲得する //
		for(auto& c : nd.Name) {
			c = _xGet<uint8_t>(bd, Mask);
			CheckSum += c;
		}
		if(flag != bd.GetBit()) return false;
		else                    flag = 1-flag;

		// 得点を獲得する //
		nd.Score = _xGet<uint64_t>(bd, Mask);
		CheckSum += nd.Score;
		if(flag != bd.GetBit()) return false;
		else                    flag = 1-flag;

		// かすりを獲得する //
		nd.Evade = _xGet<uint32_t>(bd, Mask);
		CheckSum += nd.Evade;
		if(flag != bd.GetBit()) return false;
		else                    flag = 1-flag;

		// ステージを獲得する //
		nd.Stage = _xGet<uint8_t>(bd, Mask);
		CheckSum += nd.Stage;
		if(flag != bd.GetBit()) return false;
		else                    flag = 1-flag;

		// ウエポンを獲得する //
		nd.Weapon = _xGet<uint8_t>(bd, Mask);
		CheckSum += nd.Weapon;
		if(flag != bd.GetBit()) return false;
		else                    flag = 1-flag;

		// チェックサム比較 //
		if(CheckSum != _xGet<uint64_t>(bd, Mask)) {
			return false;
		}
	}

	return true;
}

static void _SaveSC(NR_CONST_SCORE_LIST NData, BIT_DEVICE_WRITE& bd)
{
	uint64_t CheckSum = 0;
	uint64_t Mask = PBG_MASK_VALUE;
	uint8_t flag = 0;

	for(const auto& nd : NData) {
		CheckSum = 0;
		bd.PutBit(flag); flag = 1-flag;	// ビット挿入

		// 名前を出力する //
		for(auto& c : nd.Name) {
			CheckSum += c;
			_xPut(bd, static_cast<unsigned char>(c), Mask);
		}
		bd.PutBit(flag); flag = 1-flag;	// ビット挿入

		// 得点を出力する //
		CheckSum += nd.Score;
		_xPut(bd, static_cast<uint64_t>(nd.Score), Mask);
		bd.PutBit(flag); flag = 1-flag;	// ビット挿入

		// かすりを出力する //
		CheckSum += nd.Evade;
		_xPut(bd, nd.Evade, Mask);
		bd.PutBit(flag); flag = 1-flag;	// ビット挿入

		// ステージを出力する //
		CheckSum += nd.Stage;
		_xPut(bd, nd.Stage, Mask);
		bd.PutBit(flag); flag = 1-flag;	// ビット挿入

		// ウエポンを出力する //
		CheckSum += nd.Weapon;
		_xPut(bd, nd.Weapon, Mask);
		bd.PutBit(flag); flag = 1-flag;	// ビット挿入

		// チェックサムを出力する //
		_xPut(bd, CheckSum, Mask);
	}
}
