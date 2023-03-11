///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Contains code for random generators.
 *	\file		IceRandom.h
 *	\author		Pierre Terdiman
 *	\date		August, 9, 2001
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Include Guard
#ifndef __ICERANDOM_H__
#define __ICERANDOM_H__

	FUNCTION ICECORE_API	void	SRand(udword seed);
	FUNCTION ICECORE_API	udword	Rand();

	//! Returns a unit random floating-point value
	inline_ float UnitRandomFloat()	{ return float(Rand()) * ONE_OVER_RAND_MAX;	}

	//! Returns a random index so that 0<= index < max_index
	ICECORE_API	uqword GetRandomIndex(uqword max_index);

	class ICECORE_API BasicRandom
	{
		public:

		//! Constructor
		inline_				BasicRandom(uqword seed=0)	: mRnd(seed)	{}
		//! Destructor
		inline_				~BasicRandom()								{}

		inline_	void		SetSeed(uqword seed)		{ mRnd = seed;											}
		inline_	uqword		GetCurrentValue()	const	{ return mRnd;											}
		inline_	uqword		Randomize()					{ mRnd = mRnd * 2147001325 + 715136305; return mRnd;	}

		private:
				uqword		mRnd;
	};

#endif // __ICERANDOM_H__

