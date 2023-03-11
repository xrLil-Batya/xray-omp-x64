///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 *	OPCODE - Optimized Collision Detection
 *	Copyright (C) 2001 Pierre Terdiman
 *	Homepage: http://www.codercorner.com/Opcode.htm
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Contains an implementation of the sweep-and-prune algorithm (moved from Z-Collide)
 *	\file		OPC_SweepAndPrune.h
 *	\author		Pierre Terdiman
 *	\date		January, 29, 2000
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Include Guard
#ifndef __OPC_SWEEPANDPRUNE_H__
#define __OPC_SWEEPANDPRUNE_H__

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/**
	 *	User-callback, called by OPCODE for each colliding pairs.
	 *	\param		id0			[in] id of colliding object
	 *	\param		id1			[in] id of colliding object
	 *	\param		user_data	[in] user-defined data
	 *	\return		TRUE to continue enumeration
	 */
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	typedef BOOL	(*PairCallback)	(uqword id0, uqword id1, void* user_data);

	class SAP_Element;
	class SAP_EndPoint;
	class SAP_Box;

	class OPCODE_API SAP_PairData
	{
		public:
								SAP_PairData();
								~SAP_PairData();

				bool			Init(uqword nb_objects);

				void			AddPair(uqword id1, uqword id2);
				void			RemovePair(uqword id1, uqword id2);

				void			DumpPairs(Pairs& pairs)								const;
				void			DumpPairs(PairCallback callback, void* user_data)	const;
		private:
				uqword			mNbElements;		//!< Total number of elements in the pool
				uqword			mNbUsedElements;	//!< Number of used elements
				SAP_Element*	mElementPool;		//!< Array of mNbElements elements
				SAP_Element*	mFirstFree;			//!< First free element in the pool

				uqword			mNbObjects;			//!< Max number of objects we can handle
				SAP_Element**	mArray;				//!< Pointers to pool
		// Internal methods
				SAP_Element*	GetFreeElem(uqword id, SAP_Element* next, uqword* remap=null);
		inline_	void			FreeElem(SAP_Element* elem);
				void			Release();
	};

	class OPCODE_API SweepAndPrune
	{
		public:
								SweepAndPrune();
								~SweepAndPrune();

				bool			Init(uqword nb_objects, const AABB** boxes);
				bool			UpdateObject(uqword i, const AABB& box);

				void			GetPairs(Pairs& pairs)								const;
				void			GetPairs(PairCallback callback, void* user_data)	const;
		private:
				SAP_PairData	mPairs;

				uqword			mNbObjects;
				SAP_Box*		mBoxes;
				SAP_EndPoint*	mList[3];
		// Internal methods
				bool			CheckListsIntegrity();
	};

#endif //__OPC_SWEEPANDPRUNE_H__