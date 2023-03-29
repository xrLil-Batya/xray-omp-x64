#pragma once

const		u32					occq_size			= 2048; //256	;	// queue for occlusion queries

// must conform to following order of allocation/free
// a(A), a(B), a(C), a(D), ....
// f(A), f(B), f(C), f(D), ....
// a(A), a(B), a(C), a(D), ....
//	this mean:
//		use as litle of queries as possible
//		first try to use queries allocated first
//	assumption:
//		used queries number is much smaller than total count

class R_occlusion
{
private:
	struct	_Q	{
		_Q() :status(0) {}
		u32					order;
		u32                 status;
		ID3DQuery*	Q;
		u32 ttl;
	};

	static const u32		iInvalidHandle = u32(-1);

	BOOL					enabled;	// 
	xr_vector<_Q>			pool;		// sorted (max ... min), insertions are usually at the end
	xr_vector<_Q>			used;		// id's are generated from this and it is cleared from back only
	xr_vector<u32>			fids;		// free id's

	u32 last_frame;
	void cleanup_lost();
public:
#if defined(USE_DX10) || defined(USE_DX11)
	typedef	u64		occq_result;
#else	//	USE_DX10
	typedef	u32		occq_result;
#endif	//	USE_DX10
public:
	R_occlusion		();
	~R_occlusion	();

	void			occq_create		(u32	limit	);
	void			occq_destroy	(				);
	u32				occq_begin		(u32&	ID		);	// returns 'order'
	void			occq_end		(u32&	ID		);
	occq_result		occq_get		(u32&	ID		);
	void occq_free( u32 ID );
};
