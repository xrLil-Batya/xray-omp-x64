#include "stdafx.h"
#include "Weapon.h"
#include "player_hud.h"

shared_str wpn_scope = "wpn_scope";
shared_str wpn_silencer = "wpn_silencer";
shared_str wpn_grenade_launcher = "wpn_launcher";

void CWeapon::UpdateHUDAddonsVisibility()
{//actor only
	if (!GetHUDmode())										return;

	//.	return;

	u16 bone_id = HudItemData()->m_model->LL_BoneID(wpn_scope);

	if (bone_id != BI_NONE)
	{
		if (ScopeAttachable())
		{
			HudItemData()->set_bone_visible(wpn_scope, IsScopeAttached());
		}

		if (m_eScopeStatus == ALife::eAddonDisabled)
		{
			HudItemData()->set_bone_visible(wpn_scope, FALSE, TRUE);
		}
		else
			if (m_eScopeStatus == ALife::eAddonPermanent)
				HudItemData()->set_bone_visible(wpn_scope, TRUE, TRUE);
	}
	if (SilencerAttachable())
	{
		HudItemData()->set_bone_visible(wpn_silencer, IsSilencerAttached());
	}
	if (m_eSilencerStatus == ALife::eAddonDisabled)
	{
		HudItemData()->set_bone_visible(wpn_silencer, FALSE, TRUE);
	}
	else
		if (m_eSilencerStatus == ALife::eAddonPermanent)
			HudItemData()->set_bone_visible(wpn_silencer, TRUE, TRUE);

	if (GrenadeLauncherAttachable())
	{
		HudItemData()->set_bone_visible(wpn_grenade_launcher, IsGrenadeLauncherAttached());
	}
	if (m_eGrenadeLauncherStatus == ALife::eAddonDisabled)
	{
		HudItemData()->set_bone_visible(wpn_grenade_launcher, FALSE, TRUE);
	}
	else
		if (m_eGrenadeLauncherStatus == ALife::eAddonPermanent)
			HudItemData()->set_bone_visible(wpn_grenade_launcher, TRUE, TRUE);

}

void CWeapon::UpdateAddonsVisibility()
{
	IKinematics* pWeaponVisual = smart_cast<IKinematics*>(Visual()); R_ASSERT(pWeaponVisual);

	u16  bone_id;
	UpdateHUDAddonsVisibility();

	pWeaponVisual->CalculateBones_Invalidate();

	bone_id = pWeaponVisual->LL_BoneID(wpn_scope);
	if (ScopeAttachable())
	{
		if (IsScopeAttached())
		{
			if (!pWeaponVisual->LL_GetBoneVisible(bone_id) && bone_id != BI_NONE)
				pWeaponVisual->LL_SetBoneVisible(bone_id, TRUE, TRUE);
		}
		else {
			if (pWeaponVisual->LL_GetBoneVisible(bone_id) && bone_id != BI_NONE)
				pWeaponVisual->LL_SetBoneVisible(bone_id, FALSE, TRUE);
		}
	}
	if (m_eScopeStatus == ALife::eAddonDisabled && bone_id != BI_NONE &&
		pWeaponVisual->LL_GetBoneVisible(bone_id))
	{
		pWeaponVisual->LL_SetBoneVisible(bone_id, FALSE, TRUE);
		//		Log("scope", pWeaponVisual->LL_GetBoneVisible		(bone_id));
	}
	bone_id = pWeaponVisual->LL_BoneID(wpn_silencer);
	if (SilencerAttachable())
	{
		if (IsSilencerAttached()) {
			if (!pWeaponVisual->LL_GetBoneVisible(bone_id))
				pWeaponVisual->LL_SetBoneVisible(bone_id, TRUE, TRUE);
		}
		else {
			if (pWeaponVisual->LL_GetBoneVisible(bone_id))
				pWeaponVisual->LL_SetBoneVisible(bone_id, FALSE, TRUE);
		}
	}
	if (m_eSilencerStatus == ALife::eAddonDisabled && bone_id != BI_NONE &&
		pWeaponVisual->LL_GetBoneVisible(bone_id))
	{
		pWeaponVisual->LL_SetBoneVisible(bone_id, FALSE, TRUE);
		//		Log("silencer", pWeaponVisual->LL_GetBoneVisible	(bone_id));
	}

	bone_id = pWeaponVisual->LL_BoneID(wpn_grenade_launcher);
	if (GrenadeLauncherAttachable())
	{
		if (IsGrenadeLauncherAttached())
		{
			if (!pWeaponVisual->LL_GetBoneVisible(bone_id))
				pWeaponVisual->LL_SetBoneVisible(bone_id, TRUE, TRUE);
		}
		else {
			if (pWeaponVisual->LL_GetBoneVisible(bone_id))
				pWeaponVisual->LL_SetBoneVisible(bone_id, FALSE, TRUE);
		}
	}
	if (m_eGrenadeLauncherStatus == ALife::eAddonDisabled && bone_id != BI_NONE &&
		pWeaponVisual->LL_GetBoneVisible(bone_id))
	{
		pWeaponVisual->LL_SetBoneVisible(bone_id, FALSE, TRUE);
		//		Log("gl", pWeaponVisual->LL_GetBoneVisible			(bone_id));
	}


	pWeaponVisual->CalculateBones_Invalidate();
	pWeaponVisual->CalculateBones(TRUE);
}

bool CWeapon::IsGrenadeLauncherAttached() const
{
	return (ALife::eAddonAttachable == m_eGrenadeLauncherStatus &&
		0 != (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher)) ||
		ALife::eAddonPermanent == m_eGrenadeLauncherStatus;
}

bool CWeapon::IsScopeAttached() const
{
	return (ALife::eAddonAttachable == m_eScopeStatus &&
		0 != (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonScope)) ||
		ALife::eAddonPermanent == m_eScopeStatus;

}

bool CWeapon::IsSilencerAttached() const
{
	return (ALife::eAddonAttachable == m_eSilencerStatus &&
		0 != (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonSilencer)) ||
		ALife::eAddonPermanent == m_eSilencerStatus;
}

bool CWeapon::GrenadeLauncherAttachable()
{
	return (ALife::eAddonAttachable == m_eGrenadeLauncherStatus);
}
bool CWeapon::ScopeAttachable()
{
	return (ALife::eAddonAttachable == m_eScopeStatus);
}
bool CWeapon::SilencerAttachable()
{
	return (ALife::eAddonAttachable == m_eSilencerStatus);
}

void CWeapon::InitAddons()
{
}

shared_str CWeapon::GetNameWithAttachment()
{
	string64 str;
	if (pSettings->line_exist(m_section_id.c_str(), "parent_section"))
	{
		shared_str parent = pSettings->r_string(m_section_id.c_str(), "parent_section");
		xr_sprintf(str, "%s_%s", parent.c_str(), GetScopeName().c_str());
	}
	else
	{
		xr_sprintf(str, "%s_%s", m_section_id.c_str(), GetScopeName().c_str());
	}
	return (shared_str)str;
}

int CWeapon::GetScopeX()
{
	if (bUseAltScope)
	{
		if (m_eScopeStatus != ALife::eAddonPermanent && IsScopeAttached())
		{
			return pSettings->r_s32(GetNameWithAttachment(), "scope_x");
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return pSettings->r_s32(m_scopes[m_cur_scope], "scope_x");
	}
}

int CWeapon::GetScopeY()
{
	if (bUseAltScope)
	{
		if (m_eScopeStatus != ALife::eAddonPermanent && IsScopeAttached())
		{
			return pSettings->r_s32(GetNameWithAttachment(), "scope_y");
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return pSettings->r_s32(m_scopes[m_cur_scope], "scope_y");
	}
}
