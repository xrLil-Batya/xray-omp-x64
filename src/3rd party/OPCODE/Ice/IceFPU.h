///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
*	Contains FPU related code.
*	\file		IceFPU.h
*	\author		Pierre Terdiman
*	\date		April, 4, 2000
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Include Guard
#pragma once

#define	SIGN_BITMASK			0x80000000

//! Integer representation of a floating-point value.
#define IR(x)					((uqword&)(x))

//! Signed integer representation of a floating-point value.
#define SIR(x)					((sdword&)(x))

//! Absolute integer representation of a floating-point value
#define AIR(x)					(IR(x)&0x7fffffff)

//! Floating-point representation of an integer value.
#define FR(x)					((float&)(x))

inline_	bool IsValidFloat(float value)
{
	if (isnan(value))			return false;
	if (value != value)			return false;
	if (isinf(value))			return false;

	return true;
}

#define CHECK_VALID_FLOAT(x)	ASSERT(IsValidFloat(x));

/*
//! FPU precision setting function.
inline_ void SetFPU()
{
// This function evaluates whether the floating-point
// control word is set to single precision/round to nearest/
// exceptions disabled. If these conditions don't hold, the
// function changes the control word to set them and returns
// TRUE, putting the old control word value in the passback
// location pointed to by pwOldCW.
{
uword wTemp, wSave;

__asm fstcw wSave
if (wSave & 0x300 ||            // Not single mode
0x3f != (wSave & 0x3f) ||   // Exceptions enabled
wSave & 0xC00)              // Not round to nearest mode
{
__asm
{
mov ax, wSave
and ax, not 300h    ;; single mode
or  ax, 3fh         ;; disable all exceptions
and ax, not 0xC00   ;; round to nearest mode
mov wTemp, ax
fldcw   wTemp
}
}
}
}
*/
//! This function computes the slowest possible floating-point value (you can also directly use FLT_EPSILON)
inline_ float ComputeFloatEpsilon()
{
	float f = 1.0f;
	((uqword&)f) ^= 1;
	return f - 1.0f;	// You can check it's the same as FLT_EPSILON
}

inline_ bool IsFloatZero(float x, float epsilon = 1e-6f)
{
	return x * x < epsilon;
}

inline_ int ConvertToSortable(float f)
{
	int& Fi = (int&)f;
	int Fmask = (Fi >> 31);
	Fi ^= Fmask;
	Fmask &= ~(1 << 31);
	Fi -= Fmask;
	return Fi;
}

enum FPUMode
{
	FPU_FLOOR = 0,
	FPU_CEIL = 1,
	FPU_BEST = 2,

	FPU_FORCE_DWORD = 0x7fffffff
};

FUNCTION ICECORE_API FPUMode	GetFPUMode();
FUNCTION ICECORE_API void		SaveFPU();
FUNCTION ICECORE_API void		RestoreFPU();
FUNCTION ICECORE_API void		SetFPUFloorMode();
FUNCTION ICECORE_API void		SetFPUCeilMode();
FUNCTION ICECORE_API void		SetFPUBestMode();

FUNCTION ICECORE_API void		SetFPUPrecision24();
FUNCTION ICECORE_API void		SetFPUPrecision53();
FUNCTION ICECORE_API void		SetFPUPrecision64();
FUNCTION ICECORE_API void		SetFPURoundingChop();
FUNCTION ICECORE_API void		SetFPURoundingUp();
FUNCTION ICECORE_API void		SetFPURoundingDown();
FUNCTION ICECORE_API void		SetFPURoundingNear();

FUNCTION ICECORE_API int		intChop(const float& f);
FUNCTION ICECORE_API int		intFloor(const float& f);
FUNCTION ICECORE_API int		intCeil(const float& f);
