#include <Windows.h>
#include "LZ_UTY.h"

bool sub_421160(BIT_DEVICE *a1);
PBG_FILEHEAD FileHead;
PBG_FILEINFO *FileInfo;
char aScreenDBmp[0x100] = "Screen%d.BMP";
extern BOOL CaptureFlag;

void WriteHead(BIT_DEVICE *a1) {
	int v1; // edi

	if (a1->m5 == BD_FILE_WRITE)
	{
		v1 = ftell(a1->p.file);
		fseek(a1->p.file, 0, 0);
		fwrite(&FileHead.name, 0xC, 1, a1->p.file);
		fseek(a1->p.file, v1, 0);
	}
}

BIT_DEVICE *BitFilCreate(const char *a1, int a2)
{
	BIT_DEVICE *v2; // esi
	char v3; // cl
	FILE *v4; // eax
	const char *v6; // [esp+Ch] [ebp-4h]

	v2 = (BIT_DEVICE *)malloc(0x20u);
	if (!v2)
		return NULL;

	switch (a2)
	{
	case BD_FILE_READ:
		v6 = "rb";
		break;
	case BD_FILE_WRITE:
		v6 = "wb";
		break;
	case 2:
		v6 = "rb";
		break;
	case 3:
		v6 = "wb";
		break;
	default:
		free(v2);
		return 0;
	}
	v2->p.file = fopen(a1, v6);
	if (!v2->p.file) {
		free(v2);
		return 0;
	}
	v2->m1 = 0;
	v2->mask = 0x80;
	v2->rack = 0;
	v2->m4 = 0;
	v2->m5 = a2;
	v2->m6 = 0;
	v2->m7 = 0;
	return v2;
}

BIT_DEVICE *BitMemCreate(BYTE *a1, int a2, size_t a3) {
	BIT_DEVICE *result; // eax
	result = (BIT_DEVICE *)malloc(0x20u);
	if (result)
	{
		if (a2 >= 4 && a2 <= 7)
		{
			result->p.m0 = a1;
			result->m1 = a1;
			result->mask = 0x80;
			result->rack = 0;
			result->m4 = 0;
			result->m5 = a2;
			result->m6 = a3;
			result->m7 = 0;
			return result;
		}
		free(result);
	}
	return 0;
}

BIT_DEVICE *FilStartR(const char *a1, int a2) {
	if (a2)
		return NULL;
	BIT_DEVICE *v3 = BitFilCreate(a1, BD_FILE_READ);
	if (sub_421160(v3))
		return v3;
	BitDevRelease(v3);
	return NULL;
}

BIT_DEVICE *FilStartW(const char *a1, int a2, size_t a3) {
	BIT_DEVICE *v4;
	if (a2 != 1)
		return 0;
	v4 = BitFilCreate(a1, 1);
	FileInfo = (PBG_FILEINFO *)LocalAlloc(0x40u, 12 * a3);
	return v4;
}

bool sub_421160(BIT_DEVICE *a1) {
	// TODO
	return 0;
}

void BitDevRelease(BIT_DEVICE *a1) {
	if (a1) {
		switch (a1->m5)
		{
		case 0:
		case 1:
		case 2:
		case 3:
			fclose(a1->p.file);
		case 4:
		case 5:
		case 6:
		case 7:
			free(a1);
		}
	}
}

void FilEnd(BIT_DEVICE *a1) {
	BitDevRelease(a1);
	LocalFree(FileInfo);
}

BYTE *MemExpand(BIT_DEVICE *a1, int a2) {
	BYTE *result; // eax
	BYTE *v3; // edi
	BIT_DEVICE *v4; // esi

	result = (BYTE *)LocalAlloc(0x40u, FileInfo[a2].m0);
	v3 = result;
	if (result)
	{
		v4 = BitMemCreate(result, 5, FileInfo[a2].m0);
		if (!sub_421060(a1, v4, a2))
		{
			LocalFree(v3);
			v3 = 0;
		}
		BitDevRelease(v4);
		result = v3;
	}
	return result;
}

void Compress(BIT_DEVICE *a1, BIT_DEVICE *a2, int a3) {
	// TODo
	return;
}

int sub_421060(BIT_DEVICE *a1, BIT_DEVICE *a2, int a3) {
	// TODO
	return 0;
}

void PutBit(BIT_DEVICE *a1, BYTE a2) {
	char v2; // al
	unsigned __int8 *v3; // edx
	int v4; // eax
	int v5; // eax
	int v6; // ecx

	if (a2)
		a1->rack |= a1->mask;
	v2 = a1->mask >> 1;
	a1->mask = v2;
	if (!v2)
	{
		if (a1->m5 == BD_FILE_WRITE)
		{
			putc(a1->rack, a1->p.file);
		}
		else
		{
			if (a1->m5 != 5)
				return;
			*(a1->p.m0)++ = (a1->rack);
		}
		v5 = a1->rack;
		v6 = a1->m4;
		a1->rack = 0;
		a1->m4 = v5 + v6;
		a1->mask = 0x80;
	}
}

void PutBits(BIT_DEVICE *a1, DWORD a2, int a3) {
	unsigned int v3; // edi
	char v4; // cl
	DWORD v5; // ecx
	DWORD v6; // edx
	char v7; // al
	unsigned __int8 *v8; // edx
	int v9; // eax
	DWORD v10; // ecx
	DWORD v11; // eax

	v3 = 1 << (a3 - 1);
	if (a1->m5 == 1)
	{
		for (; v3; v3 >>= 1)
		{
			if (v3 & a2)
				a1->rack |= a1->mask;
			v7 = a1->mask >> 1;
			a1->mask = v7;
			if (!v7)
			{
				putc(a1->rack, a1->p.file);
				v10 = a1->rack;
				v11 = a1->m4;
				a1->rack = 0;
				a1->m4 = v10 + v11;
				a1->mask = 0x80;
			}
		}
	}
	else if (a1->m5 == 5 && v3)
	{
		do
		{
			if (v3 & a2)
				a1->rack |= a1->mask;
			v4 = a1->mask >> 1;
			a1->mask = v4;
			if (!v4)
			{
				*(a1->p.m0) = a1->rack;
				v5 = a1->m4;
				++(a1->p.m0);
				v6 = a1->rack;
				a1->rack = 0;
				a1->m4 = v6 + v5;
				a1->mask = -128;
			}
			v3 >>= 1;
		} while (v3);
	}
}

void PutChar(uint32_t a1, BIT_DEVICE *a2) {
	switch (a2->m5)
	{
	case BD_FILE_WRITE:
	case 3:
		fputc(a1, a2->p.file);
		break;
	case 5:
	case 7:
		*(a2->p.m0) = a1;
		(a2->p.m0)++;
		break;
	default:
		return;
	}
}

BYTE GetBit(BIT_DEVICE *a1) {
	int v1; // eax
	unsigned int v3; // ecx
	int v4; // edx
	DWORD *v5; // ecx
	int v6; // eax
	int v7; // eax
	char v8; // cl

	v1 = a1->m5;
	if (v1)
	{
		if (v1 != BD_MEMORY_READ)
			return 0;
		if (a1->mask == 0x80)
		{
			v3 = a1->m7;
			if (v3 >= a1->m6)
			{
				return 0;
			}
			v4 = *(a1->p.m0)++;
			a1->rack= v4;
			a1->m4 += v4;
			a1->m7 = v3 + 1;
		}
	}
	else if (a1->mask == 0x80)
	{
		v6 = getc(a1->p.file);
		a1->rack = v6;
		if (v6 == -1)
		{
			return 0;
		}
		a1->m4 += v6;
	}
	v7 = a1->mask & a1->rack;
	v8 = a1->mask >> 1;
	a1->mask = v8;
	if (!v8)
		a1->mask = 0x80;
	return v7 != 0;
}

WORD GetBits(BIT_DEVICE *a1, int a2) {
	DWORD v2; // eax
	WORD v3; // bx
	unsigned int v4; // edi
	unsigned __int8 v6; // al
	DWORD v7; // ebp
	DWORD v8; // edx
	char v9; // al
	DWORD *v10; // ecx
	int v11; // eax
	unsigned __int8 v12; // al
	char v13; // al

	v2 = a1->m5;
	v3 = 0;
	v4 = 1 << (a2 - 1);
	if (v2)
	{
		if (v2 != BD_MEMORY_READ)
			return 0;
		while (v4)
		{
			v6 = a1->mask;
			if (v6 == 0x80)
			{
				v7 = a1->m7;
				if (v7 >= a1->m6)
				{
					return 0;
				}
				v8 = *(a1->p.m0)++;
				a1->rack = v8;
				a1->m4 += v8;
				a1->m7 = v7 + 1;
			}
			if (v6 & (uint8_t)(a1->rack))
				v3 |= v4;
			v4 >>= 1;
			v9 = v6 >> 1;
			a1->mask = v9;
			if (!v9)
				a1->mask = 0x80;
		}
		return v3;
	}
	if (!v4)
		return v3;
	for (;;)
	{
		v12 = a1->mask;
		if ((uint8_t)(a1->rack) & v12)
			v3 |= v4;
		v4 >>= 1;
		v13 = v12 >> 1;
		a1->mask = v13;
		if (!v13) {
			a1->mask = 0x80;
			v11 = getc(a1->p.file);
			a1->rack = v11;
			if (v11 != -1)
				a1->m4 += v11;
			else
				return 0;
		}
		if (!v4)
			return v3;
	}
}

void GrpSetCaptureFilename(const char *a1) {
	sprintf(aScreenDBmp, "%.4s%%04d.BMP", a1);
	CaptureFlag = 1;
}