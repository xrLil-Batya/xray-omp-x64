#pragma once

#include "HUDCrosshair.h"
#include "../xrcdb/xr_collide_defs.h"


class CHUDManager;
class CLAItem;

struct SPickParam
{
	collide::rq_result RQ;
	float				power;
	u32					pass;
};

class CHUDTarget 
{
private:
	ui_shader				hShader;
	float					fuzzyShowInfo;
	SPickParam				PP;

	Fvector					m_crosshairCastedFromPos;
	Fvector					m_crosshairCastedToDir;

	bool					m_bShowCrosshair;
	CHUDCrosshair			HUDCrosshair;

private:
	collide::rq_results		RQR;

public:
							CHUDTarget	();
							~CHUDTarget	();
	void					CursorOnFrame ();
	void					Render		();
	void					Load		();
	collide::rq_result&		GetRQ		() {return PP.RQ;};
	float					GetRQVis	() {return PP.power;};
	CHUDCrosshair&			GetHUDCrosshair	() {return HUDCrosshair;}
	void					DefineCrosshairCastingPoint(const Fvector& point, const Fvector& direction);
	void					ShowCrosshair	(bool b);
	void					net_Relcase		(CObject* O);
};
