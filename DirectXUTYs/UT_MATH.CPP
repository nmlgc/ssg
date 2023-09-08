/*                                                                           */
/*   UT_MATH.c   整数限定の数学関数群                                        */
/*                                                                           */
/*                                                                           */

#include "UT_MATH.H"
#include <type_traits>
#pragma message(PBGWIN_UT_MATH_H)


#define RANDH	0x015a
#define RANDL	0x4e35

static uint32_t random_seed; // 乱数のたね //
//uint32_t random_ref;


////ｓｉｎテーブル(ｃｏｓを含む)////
extern const signed int SIN256[256+64] = {
	0,6,12,18,25,31,37,43,49,56,62,68,74,80,86,92,97,103,109,115,120,126,131,136,
	142,147,152,157,162,167,171,176,181,185,189,193,197,201,205,209,212,216,219,
	222,225,228,231,234,236,238,241,243,244,246,248,249,251,252,253,254,254,255,
	255,255,256,255,255,255,254,254,253,252,251,249,248,246,244,243,241,238,236,
	234,231,228,225,222,219,216,212,209,205,201,197,193,189,185,181,176,171,167,
	162,157,152,147,142,136,131,126,120,115,109,103,97,92,86,80,74,68,62,56,49,43,
	37,31,25,18,12,6,0,-6,-12,-18,-25,-31,-37,-43,-49,-56,-62,-68,-74,-80,-86,-92,
	-97,-103,-109,-115,-120,-126,-131,-136,-142,-147,-152,-157,-162,-167,-171,
	-176,-181,-185,-189,-193,-197,-201,-205,-209,-212,-216,-219,-222,-225,-228,
	-231,-234,-236,-238,-241,-243,-244,-246,-248,-249,-251,-252,-253,-254,-254,
	-255,-255,-255,-256,-255,-255,-255,-254,-254,-253,-252,-251,-249,-248,-246,
	-244,-243,-241,-238,-236,-234,-231,-228,-225,-222,-219,-216,-212,-209,-205,
	-201,-197,-193,-189,-185,-181,-176,-171,-167,-162,-157,-152,-147,-142,-136,
	-131,-126,-120,-115,-109,-103,-97,-92,-86,-80,-74,-68,-62,-56,-49,-43,-37,-31,
	-25,-18,-12,-6,0,6	,12,18,25,31,37,43,49,56,62,68,74,80,86,92,97,103,109,115,
	120,126,131,136,142,147,152,157,162,167,171,176,181,185,189,193,197,201,205,
	209,212,216,219,222,225,228,231,234,236,238,241,243,244,246,248,249,251,252,
	253,254,254,255,255,255
};

////ｃｏｓテーブル、というのかな？////
extern const signed int *COS256 = &SIN256[64];


////ａｔａｎテーブル////
static const char ATAN256[]={
	0, 0, 0, 0, 1, 1, 1, 1,  1, 1, 2, 2, 2, 2, 2, 2,
	3, 3, 3, 3, 3, 3, 3, 4,  4, 4, 4, 4, 4, 5, 5, 5,
	5, 5, 5, 6, 6, 6, 6, 6,  6, 6, 7, 7, 7, 7, 7, 7,
	8, 8, 8, 8, 8, 8, 8, 9,  9, 9, 9, 9, 9,10,10,10,
	10,10,10,10,11,11,11,11, 11,11,11,12,12,12,12,12,
	12,12,13,13,13,13,13,13, 13,14,14,14,14,14,14,14,
	15,15,15,15,15,15,15,16, 16,16,16,16,16,16,17,17,
	17,17,17,17,17,17,18,18, 18,18,18,18,18,19,19,19,
	19,19,19,19,19,20,20,20, 20,20,20,20,20,21,21,21,
	21,21,21,21,21,21,22,22, 22,22,22,22,22,22,23,23,
	23,23,23,23,23,23,23,24, 24,24,24,24,24,24,24,24,
	25,25,25,25,25,25,25,25, 25,25,26,26,26,26,26,26,
	26,26,26,27,27,27,27,27, 27,27,27,27,27,28,28,28,
	28,28,28,28,28,28,28,28, 29,29,29,29,29,29,29,29,
	29,29,29,30,30,30,30,30, 30,30,30,30,30,30,31,31,
	31,31,31,31,31,31,31,31, 31,31,32,32,32,32,32,32
};


long __fastcall sinl(uint8_t deg,int length)
{
	//return ((long)sinm(deg)*length)/256;
	return ((static_cast<long>(sinm(deg)) * length) >> 8);
}

long __fastcall cosl(uint8_t deg,int length)
{
	//return ((long)cosm(deg)*length)/256;
	return ((static_cast<long>(cosm(deg)) * length) >> 8);
}

// length*256/(SIN(deg)>0 ? SIN(deg) : 256) //
long __fastcall sinDiv(uint8_t deg, int length)
{
	const int sind = sinm(deg);
	return (length<<8) / (sind>0 ? sind : 256);
}

// length*256/(COS(deg)>0 ? COS(deg) : 256) //
long __fastcall cosDiv(uint8_t deg, int length)
{
	const int cosd = cosm(deg);
	return (length<<8) / (cosd>0 ? cosd : 256);
}

uint8_t __stdcall atan8(int x,int y)
{
	_asm{
			PUSH	ESI

			MOV		ECX,y			; ECX := y
			MOV		ESI,x			; ESI := x
			MOV		EAX,ECX
			OR		EAX,ESI
			JZ		FINISH

			MOV		EAX,ECX
			CDQ
			XOR		EAX,EDX
			SUB		EAX,EDX
			MOV		EBX,EAX			; EBX := ABS(y)

			MOV		EAX,ESI
			CDQ
			XOR		EAX,EDX
			SUB		EAX,EDX
			MOV		EDX,EAX			; EDX := ABS(x)

			CMP		EDX,EBX
			JE		JP_E
			JL		JP_L

			XCHG	EBX,EDX			; EBX := ABS(x) , EDX := ABS(y)
			MOV		EAX,EDX
			SHR		EDX,24
			SHL		EAX,8
			DIV		EBX
			MOV		EBX,offset ATAN256
			XLATB
			JMP		LAST_C0
			EVEN

	JP_E:
			MOV		AL,32
			JMP		LAST_C0
			EVEN

	JP_L:
			MOV		EAX,EDX
			SHR		EDX,24
			SHL		EAX,8
			DIV		EBX
			MOV		EBX,offset ATAN256
			XLATB
			NEG		AL
			ADD		AL,64

			// EBP = x , ECX = y
	LAST_C0:
			XOR		AH,AH
			OR		ESI,ESI
			JGE		LAST_C1
			NEG		AX
			ADD		AX,128

	LAST_C1:
			OR		ECX,ECX
			JGE		LAST_C2
			NEG		AL

	LAST_C2:
			XOR		AH,AH			; RETURN が BYTE だから必要ないのだが...

	FINISH:
			POP		ESI
	}
}

void __fastcall rnd_seed_set(uint32_t val)
{
	random_seed = val;
}

int32_t isqrt(int32_t s)
{
	auto error = s;

	// Continuously subtract integer squares (1², 2², 3³, …) from [s] until [s]
	// is smaller.
	// This can be done without multiplications by considering the difference
	// between two consecutive integer squares (𝓃 and 𝓃+1):
	//
	// 	d(𝓃) = ((𝓃 + 1)² - 𝓃²) = (2𝓃 + 1)
	//
	// Defined recursively:
	//
	// 	d(0) = 1
	// 	d(𝓃) = (2𝓃 + 1)
	// 	d(𝓃 + 1) = (2(𝓃 + 1) + 1),	multiplied out:
	// 	d(𝓃 + 1) = (2𝓃 + 2 + 1),  	and with d(𝓃) = (2𝓃 + 1) extracted:
	// 	d(𝓃 + 1) = (d(𝓃) + 2)
	//
	// That is, start with 1, and add 2 on every iteration.
	//
	// This is a descending variant of the addition-based linear search
	// algorithm found on Wikipedia:
	//
	// 	https://en.wikipedia.org/w/index.php?title=Integer_square_root&oldid=1174298171#Linear_search_using_addition
	//
	// By deriving the loop count from the accumulator, this variant saves one
	// variable. It also adds 1 to the result we would otherwise get from the
	// Wikipedia algorithm, which always rounds up the result to the next
	// higher integer root.
	std::make_unsigned_t<decltype(s)> acc = 1;
	while(error >= 0) {
		error -= acc;
		acc += 2;
	}
	auto root_plus_1 = (acc >> 1); // = number of iterations

	// If [s] is closer to ([root_plus_1] - 1)² than it is to [root_plus_1]²,
	// we need to round down. By once again taking the difference between the
	// two squares (2𝓃 + 1), and halving it to arrive at the arithmetic mean
	// between the two, we get (𝓃 + 0.5), which we can round up to (𝓃 + 1).
	// The remaining [error] already tells us the negated difference between
	// [root_plus_1]² and [s].
	if((-error >= root_plus_1) && (s > 0)) {
		return (root_plus_1 - 1);
	}
	return root_plus_1;
}

uint16_t __fastcall rnd(void)
{
	_asm{
		MOV	AX,RANDL
		MUL	word ptr random_seed+2
		MOV	CX,AX
		MOV	AX,RANDH
		MUL	word ptr random_seed
		ADD	CX,AX
		MOV	AX,RANDL
		MUL	word ptr random_seed
		ADD	AX,1
		ADC	DX,CX
		MOV	word ptr random_seed,AX
		MOV	AX,DX
		MOV	word ptr random_seed+2,AX
		AND	AH,7fh

		//MOV	result,AX
	}
}
