#include "StdAfx.h"
#include ".\r__occlusion.h"

#include "QueryHelper.h"

R_occlusion::R_occlusion(void)
{
	enabled = strstr(Core.Params, "-no_occq") ? FALSE : TRUE;
	last_frame = Device.dwFrame;
}
R_occlusion::~R_occlusion(void)
{
	occq_destroy();
}
void	R_occlusion::occq_create(u32	limit)
{
	pool.reserve(limit);
	used.reserve(limit);
	fids.reserve(limit);
	for (u32 it = 0; it < limit; it++) {
		_Q	q;	q.order = it;
		if (FAILED(CreateQuery(&q.Q, D3DQUERYTYPE_OCCLUSION)))	break;
		pool.push_back(q);
	}
	std::reverse(pool.begin(), pool.end());
}
void	R_occlusion::occq_destroy()
{
	Msg( "* [%s]: fids[%u] used[%u] pool[%u]", __FUNCTION__, fids.size(), used.size(), pool.size() );
	u32 p_cnt = 0;
	u32 u_cnt = 0;
	while (!used.empty()) {
		if ( used.back().Q ) {
			_RELEASE( used.back().Q );
			u_cnt++;
		}
		used.pop_back();
	}
	while (!pool.empty()) {
		_RELEASE(pool.back().Q);
		pool.pop_back();
		p_cnt++;
	}
	used.clear();
	pool.clear();
	fids.clear();
	Msg( "* [%s]: released %u used and %u pool queries", __FUNCTION__, u_cnt, p_cnt );
}

void R_occlusion::cleanup_lost() {
	u32 cnt = 0;
	for ( u32 ID = 0; ID < used.size(); ID++ ) {
	if ( used[ ID ].Q && used[ ID ].ttl && used[ ID ].ttl < Device.dwFrame ) {
		occq_free( ID );
		cnt++;
		}
	}
	if ( cnt > 0 )
		Msg( "! [%s]: cleanup %u lost queries", __FUNCTION__, cnt );
}

u32		R_occlusion::occq_begin(u32& ID)
{
	if (!enabled)		return 0;

	//	Igor: prevent release crash if we issue too many queries
	if (pool.empty())
	{
		if ((Device.dwFrame % 40) == 0)
			Msg(" RENDER [Warning]: Too many occlusion queries were issued(>1536)!!!");
		ID = iInvalidHandle;
		return 0;
	}
	if ( last_frame != Device.dwFrame ) {
		cleanup_lost();
		last_frame = Device.dwFrame;
	}

	RImplementation.stats.o_queries++;
	if (!fids.empty()) {
		ID = fids.back();
		fids.pop_back();
		VERIFY(pool.size());
		used[ID] = pool.back();
	}
	else {
		ID = used.size();
		VERIFY(pool.size());
		used.push_back(pool.back());
	}

	if (used[ID].status == 2)
	{
		occq_result	fragments = 0;
		GetData(used[ID].Q, &fragments, sizeof(fragments));
		used[ID].status = 0;
	}
	R_ASSERT(used[ID].status == 0);

	pool.pop_back();
	//CHK_DX					(used[ID].Q->Issue	(D3DISSUE_BEGIN));
	CHK_DX(BeginQuery(used[ID].Q));

	// Msg				("begin: [%2d] - %d", used[ID].order, ID);
	used[ID].status = 1;
	used[ID].ttl = Device.dwFrame + 1;
	return			used[ID].order;
}
void	R_occlusion::occq_end(u32& ID)
{
	if (!enabled)		return;

	//	Igor: prevent release crash if we issue too many queries
	if (ID == iInvalidHandle || !used[ID].Q) return;

	R_ASSERT(used[ID].status == 1);

	// Msg				("end  : [%2d] - %d", used[ID].order, ID);
	//CHK_DX			(used[ID].Q->Issue	(D3DISSUE_END));
	CHK_DX(EndQuery(used[ID].Q));
	used[ID].ttl = Device.dwFrame + 1;
	used[ID].status = 2;
}
R_occlusion::occq_result R_occlusion::occq_get(u32& ID)
{
	if (!enabled)		return 0xffffffff;

	//	Igor: prevent release crash if we issue too many queries
	if (ID == iInvalidHandle) return 0xFFFFFFFF;

	if (ID >= used.size())
	{
		Msg("!Occlusion: Wrong data sequence");
		return 0xffffffff;
	}

	if (!used[ID].Q)
	{
		Msg("!Occlusion: Null queue skipped");
		return 0xffffffff;
	}


	occq_result	fragments = 0;
	HRESULT hr;

	R_ASSERT(used[ID].status == 2);

	CTimer	T;
	T.Start();
	Device.Statistic->RenderDUMP_Wait.Begin();
	//while	((hr=used[ID].Q->GetData(&fragments,sizeof(fragments),D3DGETDATA_FLUSH))==S_FALSE) {
	VERIFY2(ID < used.size(), make_string("_Pos = %d, size() = %d ", ID, used.size()));

	while ((hr = GetData(used[ID].Q, &fragments, sizeof(fragments))) == S_FALSE)
	{
		if (!SwitchToThread())
			Sleep(ps_r2_wait_sleep);

		if (T.GetElapsed_ms() > 500)
		{
			fragments = (occq_result)-1;//0xffffffff;
			break;
		}
	}
	Device.Statistic->RenderDUMP_Wait.End();
	if (hr == D3DERR_DEVICELOST)	fragments = 0xffffffff;

	if (0 == fragments)	RImplementation.stats.o_culled++;

	// insert into pool (sorting in decreasing order)
	_Q& Q = used[ID];

	if (pool.empty())
		pool.push_back(Q);
	else
	{
		int		it = int(pool.size()) - 1;
		while ((it >= 0) && (pool[it].order < Q.order))
			it--;
		pool.insert(pool.begin() + it + 1, Q);
	}

	// remove from used and shrink as nesessary
	if ( used[ ID ].Q ) {
		used[ID].Q = 0;
		used[ID].status = 0;
		fids.push_back(ID);
	}
	ID = 0;
	return	fragments;
}

void R_occlusion::occq_free( u32 ID ) {
	if ( used[ ID ].Q ) {
		pool.push_back( used[ ID ] );
		used[ ID ].Q = nullptr;
		fids.push_back( ID );
	}
}