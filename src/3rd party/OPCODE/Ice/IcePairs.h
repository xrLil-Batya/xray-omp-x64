///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Contains a simple pair class.
 *	\file		IcePairs.h
 *	\author		Pierre Terdiman
 *	\date		January, 13, 2003
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Include Guard
#ifndef __ICEPAIRS_H__
#define __ICEPAIRS_H__

	//! A generic couple structure
	struct ICECORE_API Pair
	{
		inline_	Pair()	{}
		inline_	Pair(uqword i0, uqword i1) : id0(i0), id1(i1)	{}

		uqword	id0;	//!< First index of the pair
        uqword	id1;	//!< Second index of the pair
	};

	class ICECORE_API Pairs : private Container
	{
		public:
		// Constructor / Destructor
								Pairs()							{}
								~Pairs()						{}

		inline_	uqword			GetNbPairs()		const		{ return GetNbEntries()>>1;					}
		inline_	const Pair*		GetPairs()			const		{ return (const Pair*)GetEntries();			}
		inline_	const Pair*		GetPair(uqword i)	const		{ return (const Pair*)&GetEntries()[i+i];	}

		inline_	BOOL			HasPairs()			const		{ return !!IsNotEmpty();					}

		inline_	void			ResetPairs()					{ Reset();									}
		inline_	void			DeleteLastPair()				{ DeleteLastEntry();	DeleteLastEntry();	}

		inline_	void			AddPair(const Pair& p)			{ Add(p.id0).Add(p.id1);					}
		inline_	void			AddPair(uqword id0, uqword id1)	{ Add(id0).Add(id1);						}
	};

#endif // __ICEPAIRS_H__
