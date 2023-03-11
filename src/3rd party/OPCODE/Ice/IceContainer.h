///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Contains a simple container class.
 *	\file		IceContainer.h
 *	\author		Pierre Terdiman
 *	\date		February, 5, 2000
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Include Guard
#ifndef __ICECONTAINER_H__
#define __ICECONTAINER_H__

	#define CONTAINER_STATS

	enum FindMode
	{
		FIND_CLAMP,
		FIND_WRAP,

		FIND_FORCE_DWORD = 0x7fffffff
	};

	class ICECORE_API Container
	{
		public:
		// Constructor / Destructor
								Container();
								Container(const Container& object);
								Container(uqword size, float growth_factor);
								~Container();
		// Management
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	A O(1) method to add a value in the container. The container is automatically resized if needed.
		 *	The method is inline, not the resize. The call overhead happens on resizes only, which is not a problem since the resizing operation
		 *	costs a lot more than the call overhead...
		 *
		 *	\param		entry		[in] a uqword to store in the container
		 *	\see		Add(float entry)
		 *	\see		Empty()
		 *	\see		Contains(uqword entry)
		 *	\return		Self-Reference
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		inline_	Container&		Add(uqword entry)
								{
									// Resize if needed
									if(mCurNbEntries==mMaxNbEntries)	Resize();

									// Add new entry
									mEntries[mCurNbEntries++]	= entry;
									return *this;
								}

        inline_	Container&		Add(udword entry)
								{
									// Resize if needed
									if(mCurNbEntries==mMaxNbEntries)	Resize();

									// Add new entry
									mEntries[mCurNbEntries++]	= entry;
									return *this;
								}

		inline_	Container&		Add(const uqword* entries, uqword nb)
								{
									// Resize if needed
									if(mCurNbEntries+nb>mMaxNbEntries)	Resize(nb);

									// Add new entry
									CopyMemory(&mEntries[mCurNbEntries], entries, nb*sizeof(uqword));
									mCurNbEntries+=nb;
									return *this;
								}

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	A O(1) method to add a value in the container. The container is automatically resized if needed.
		 *	The method is inline, not the resize. The call overhead happens on resizes only, which is not a problem since the resizing operation
		 *	costs a lot more than the call overhead...
		 *
		 *	\param		entry		[in] a float to store in the container
		 *	\see		Add(uqword entry)
		 *	\see		Empty()
		 *	\see		Contains(uqword entry)
		 *	\return		Self-Reference
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		inline_	Container&		Add(float entry)
								{
									// Resize if needed
									if(mCurNbEntries==mMaxNbEntries)	Resize();

									// Add new entry
									mEntries[mCurNbEntries++]	= IR(entry);
									return *this;
								}

		inline_	Container&		Add(const float* entries, uqword nb)
								{
									// Resize if needed
									if(mCurNbEntries+nb>mMaxNbEntries)	Resize(nb);

									// Add new entry
									CopyMemory(&mEntries[mCurNbEntries], entries, nb*sizeof(float));
									mCurNbEntries+=nb;
									return *this;
								}

		//! Add unique [slow]
		inline_	Container&		AddUnique(uqword entry)
								{
									if(!Contains(entry))	Add(entry);
									return *this;
								}

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Clears the container. All stored values are deleted, and it frees used ram.
		 *	\see		Reset()
		 *	\return		Self-Reference
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				Container&		Empty();

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Resets the container. Stored values are discarded but the buffer is kept so that further calls don't need resizing again.
		 *	That's a kind of temporal coherence.
		 *	\see		Empty()
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		inline_	void			Reset()
								{
									// Avoid the write if possible
									// ### CMOV
									if(mCurNbEntries)	mCurNbEntries = 0;
								}

		// HANDLE WITH CARE
		inline_	void			ForceSize(uqword size)
								{
									mCurNbEntries = size;
								}

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Sets the initial size of the container. If it already contains something, it's discarded.
		 *	\param		nb		[in] Number of entries
		 *	\return		true if success
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				bool			SetSize(uqword nb);

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 *	Refits the container and get rid of unused bytes.
		 *	\return		true if success
		 */
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				bool			Refit();

		// Checks whether the container already contains a given value.
				bool			Contains(uqword entry, uqword* location=null) const;
		// Deletes an entry - doesn't preserve insertion order.
				bool			Delete(uqword entry);
		// Deletes an entry - does preserve insertion order.
				bool			DeleteKeepingOrder(uqword entry);
		//! Deletes the very last entry.
		inline_	void			DeleteLastEntry()						{ if(mCurNbEntries)	mCurNbEntries--;			}
		//! Deletes the entry whose index is given
		inline_	void			DeleteIndex(uqword index)				{ mEntries[index] = mEntries[--mCurNbEntries];	}

		// Helpers
				Container&		FindNext(uqword& entry, FindMode find_mode=FIND_CLAMP);
				Container&		FindPrev(uqword& entry, FindMode find_mode=FIND_CLAMP);
		// Data access.
		inline_	uqword			GetNbEntries()					const	{ return mCurNbEntries;					}	//!< Returns the current number of entries.
		inline_	uqword			GetEntry(uqword i)				const	{ return mEntries[i];					}	//!< Returns ith entry
		inline_	uqword*			GetEntries()					const	{ return mEntries;						}	//!< Returns the list of entries.

		inline_	uqword			GetFirst()						const	{ return mEntries[0];					}
		inline_	uqword			GetLast()						const	{ return mEntries[mCurNbEntries-1];		}

		// Growth control
		inline_	float			GetGrowthFactor()				const	{ return mGrowthFactor;					}	//!< Returns the growth factor
		inline_	void			SetGrowthFactor(float growth)			{ mGrowthFactor = growth;				}	//!< Sets the growth factor
		inline_	bool			IsFull()						const	{ return mCurNbEntries==mMaxNbEntries;	}	//!< Checks the container is full
		inline_	uqword			IsNotEmpty()					const	{ return mCurNbEntries;					}	//!< Checks the container is empty

		//! Read-access as an array
		inline_	uqword			operator[](uqword i)			const	{ ASSERT(i>=0 && i<mCurNbEntries); return mEntries[i];	}
		//! Write-access as an array
		inline_	uqword&			operator[](uqword i)					{ ASSERT(i>=0 && i<mCurNbEntries); return mEntries[i];	}

		// Stats
				uqword			GetUsedRam()					const;

		//! Operator for "Container A = Container B"
				void			operator = (const Container& object);

#ifdef CONTAINER_STATS
		inline_	uqword			GetNbContainers()				const	{ return mNbContainers;		}
		inline_	uqword			GetTotalBytes()					const	{ return mUsedRam;			}
		private:

		static	uqword			mNbContainers;		//!< Number of containers around
		static	uqword			mUsedRam;			//!< Amount of bytes used by containers in the system
#endif
		private:
		// Resizing
				bool			Resize(uqword needed=1);
		// Data
				uqword			mMaxNbEntries;		//!< Maximum possible number of entries
				uqword			mCurNbEntries;		//!< Current number of entries
				uqword*			mEntries;			//!< List of entries
				float			mGrowthFactor;		//!< Resize: new number of entries = old number * mGrowthFactor
	};

#endif // __ICECONTAINER_H__
