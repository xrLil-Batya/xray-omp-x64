#pragma once

#include "inventory_item.h"

class CPhysicItem;
class CEntityAlive;

class CEatableItem : public CInventoryItem {
private:
	typedef CInventoryItem	inherited;

protected:
	CPhysicItem		*m_physic_item;

public:
							CEatableItem				();
	virtual					~CEatableItem				();
	virtual	DLL_Pure*		_construct					();
	virtual CEatableItem	*cast_eatable_item			()	{return this;}

			void			UpdateInRuck				();

	virtual void OnEvent(NET_Packet& P, u16 type);

	virtual void UpdateCL();
			void			UpdateUseAnim				();
			void			StartAnimation				();
			void			HideWeapon					();

	virtual void			Load						(LPCSTR section);
	virtual bool			Useful						() const;

	virtual BOOL			net_Spawn					(CSE_Abstract* DC);

	virtual void			OnH_B_Independent			(bool just_before_destroy);
	virtual void			OnH_A_Independent			();
	virtual	bool			UseBy						(CEntityAlive* npc);
	virtual	bool			Empty						()						{return PortionsNum()==0;};
			int				PortionsNum					()	const				{return m_iPortionsNum;}
	int						m_iPortionsNum;
			bool			m_bHasAnimation;
			float			m_fEffectorIntensity;
			bool			m_bActivated;
			bool			m_bItmStartAnim;
			int				m_iAnimHandsCnt;
			int				m_iAnimLength;
			LPCSTR			anim_sect;
			shared_str		use_cam_effector;
			ref_sound		m_using_sound;
};

