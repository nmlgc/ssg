/*
 *   Unused snippets from 3D effect code
 *
 */

typedef struct tagTriangle3D{
	Point3D	Current;
	Point3D	Speed;

	WINDOW_POINT	Grp[3];

	uint8_t	deg_x,deg_y,deg_z;
	char	roll_x,roll_y,roll_z;
} Triangle3D;

void InitTriangle(void);
void MoveTriangle(void);
void DrawTriangle(void);

void InitCircle(void);
void MoveCircle(void);
void DrawCircle(void);
