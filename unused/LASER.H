/*
 *   Unused laser code
 *
 */

/*
////反射物(鏡?) 構造体////
typedef struct{
	int		x,y;		// 反射物の中心座標

	uint32_t	l;	// リフレクターの長さ(中心から先端まで,つまり全体でl*2)
	uint8_t	d;	// 反射物の角度(0 <= d < 128)
} REFLECTOR;
*/

extern int  llaser_set(WORD *ind);	// 無限遠レーザーをセットする(セットできた数を返すのだ)

////レーザーの各種変数たち////
//extern REFLECTOR	Reflector[RT_MAX];		// 反射物構造体
//extern uint16_t	ReflectorNow;	// 反射物の個数
