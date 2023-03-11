///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Contains source code from the article "Radix Sort Revisited".
 *	\file		IceRevisitedRadix.h
 *	\author		Pierre Terdiman
 *	\date		April, 4, 2000
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Include Guard
#ifndef __ICERADIXSORT_H__
#define __ICERADIXSORT_H__

	//! Allocate histograms & offsets locally
	#define RADIX_LOCAL_RAM

	enum RadixHint
	{
		RADIX_SIGNED,		//!< Input values are signed
		RADIX_UNSIGNED,		//!< Input values are unsigned

		RADIX_FORCE_DWORD = 0x7fffffff
	};

	class ICECORE_API RadixSort
	{
		public:
		// Constructor/Destructor
								RadixSort();
								~RadixSort();
		// Sorting methods
				RadixSort&		Sort(const uqword* input, uqword nb, RadixHint hint=RADIX_SIGNED);
				RadixSort&		Sort(const float* input, uqword nb);

		//! Access to results. mRanks is a list of indices in sorted order, i.e. in the order you may further process your data
		inline_	const uqword*	GetRanks()			const	{ return mRanks;		}

		//! mIndices2 gets trashed on calling the sort routine, but otherwise you can recycle it the way you want.
		inline_	uqword*			GetRecyclable()		const	{ return mRanks2;		}

		// Stats
				uqword			GetUsedRam()		const;
		//! Returns the total number of calls to the radix sorter.
		inline_	uqword			GetNbTotalCalls()	const	{ return mTotalCalls;	}
		//! Returns the number of eraly exits due to temporal coherence.
		inline_	uqword			GetNbHits()			const	{ return mNbHits;		}

		private:
#ifndef RADIX_LOCAL_RAM
				uqword*			mHistogram;			//!< Counters for each byte
				uqword*			mOffset;			//!< Offsets (nearly a cumulative distribution function)
#endif
				uqword			mCurrentSize;		//!< Current size of the indices list
				uqword*			mRanks;				//!< Two lists, swapped each pass
				uqword*			mRanks2;
		// Stats
				uqword			mTotalCalls;		//!< Total number of calls to the sort routine
				uqword			mNbHits;			//!< Number of early exits due to coherence
		// Internal methods
				void			CheckResize(uqword nb);
				bool			Resize(uqword nb);
	};

#endif // __ICERADIXSORT_H__
