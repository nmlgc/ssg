/*
 *   Unused snippets from 3D effect code
 *
 */

typedef struct tagTriangle3D{
	Point3D	Current;
	Point3D	Speed;

	POINT	Grp[3];

	BYTE	deg_x,deg_y,deg_z;
	char	roll_x,roll_y,roll_z;
} Triangle3D;

typedef struct tagCircle3D{
	int		ox,oy;
	int		r;
	BYTE	deg;
	BYTE	n;
} Circle3D;

// 雲管理用構造体 //
typedef struct tagCloud2D {
	int		x, y;		// ｘ６４座標
	int		vy;			// 速度のｙ成分(ｙしかないけど)
	BYTE	type;		// 雲の種類
} Cloud2D;

void InitTriangle(void);
void MoveTriangle(void);
void DrawTriangle(void);

void InitCircle(void);
void MoveCircle(void);
void DrawCircle(void);
/*
FVOID InitStg3Cloud(void);		// ３面の雲の初期化を行う
FVOID MoveStg3Cloud(void);		// ３面の雲を動作させる
FVOID DrawStg3Cloud(void);		// ３面の雲を描画する
*/
