#include "StdAfx.h"

#include "WeaponMagazined.h"
#include "WeaponMagazinedWGrenade.h"
#include "../Include/xrRender/Kinematics.h"

bool CWeaponMagazined::CheckAnimationExist(LPCSTR anim_name)
{
	auto ptr = psAnimationList.count(anim_name);

	if (ptr > 0)
	{
		return psAnimationList[anim_name];
	}
	else
	{
		bool res = pSettings->line_exist(hud_sect, anim_name);
		psAnimationList[anim_name] = res;
		return res;
	}
}
void CWeaponMagazined::ReloadAnimationsExist()
{
	for (auto &it : psAnimationList)
	{
		it.second = pSettings->line_exist(hud_sect, it.first);
	}
}

Bas_Addon* Bas_Addon::CreateAddon(shared_str parent, xr_vector<Bas_Addon*>& m_attaches)
{
	Bas_Addon* parent_addon = xr_new<Bas_Addon>();
	parent_addon->Load(parent);

	if (parent_addon->m_sectionParent != NULL)
	{
		if (!parent_addon->FindParentAndAttach(m_attaches))
		{
			Bas_Addon* parent = CreateAddon(parent_addon->m_sectionParent, m_attaches);
			parent->m_childs.push_back(parent_addon);
		}
	}
	else
		m_attaches.push_back(parent_addon);

	return parent_addon;
}

bool Bas_Addon::FindParentAndAttach(xr_vector<Bas_Addon*>& m_attaches)
{
	for (auto& it : m_attaches)
	{
		if (it->m_sectionId == m_sectionParent)
		{
			it->m_childs.push_back(this);
			return true;
		}

		if (!it->m_childs.empty())
		{
			bool res = FindParentAndAttach(it->m_childs);
			if (res) return true;
		}
	}

	return false;
}

Bas_Addon::Bas_Addon()
{
	hud_model = NULL;
	world_model = NULL;
	m_boneNameHUD = NULL;
	m_sectionId = NULL;
	m_meshName = NULL;
	m_meshHUDName = NULL;
	m_renderPosHud.identity();
	isRoot = false;
}

void Bas_Addon::UpdateRenderPos(IRenderVisual* model, bool hud, Fmatrix parent)
{
	if ((hud && !hud_model) || (!hud && !world_model && m_UseWorldModel))
	{
		PrepareRender(hud);
	}

	u16 bone_id = hud ? model->dcast_PKinematics()->LL_BoneID(m_boneNameHUD) : model->dcast_PKinematics()->LL_BoneID(m_boneNameWORLD);
	Fmatrix bone_trans = model->dcast_PKinematics()->LL_GetTransform(bone_id);

	if (!hud)
	{
		m_renderPosWorld.identity();
		m_renderPosWorld.mul(parent, bone_trans);
	}
	else
	{
		m_renderPosHud.identity();
		m_renderPosHud.mul(parent, bone_trans);
	}

	for (auto& child : m_childs)
		child->UpdateRenderPos(hud ? hud_model : world_model, hud, hud ? m_renderPosHud : m_renderPosWorld);
}

void Bas_Addon::PrepareRender(bool hud)
{
	if (hud && !hud_model)
		hud_model = ::Render->model_Create(m_meshHUDName.c_str());
	else if (!hud && !world_model && m_UseWorldModel)
		world_model = ::Render->model_Create(m_meshName.c_str());

	for (auto& child : m_childs)
		child->PrepareRender(hud);
}

void Bas_Addon::Update_And_Render_World(IRenderVisual* model, Fmatrix parent)
{
	for (auto& child : m_childs)
	{
		child->UpdateRenderPos(model, false, parent);
		child->Render(false);
	}
}

void Bas_Addon::Render(bool hud)
{
	if ((hud && !hud_model) || (!hud && !world_model && m_UseWorldModel))
		PrepareRender(hud);

	if ((hud && hud_model) || (!hud && world_model))
	{
		::Render->set_Transform(hud ? &m_renderPosHud : &m_renderPosWorld);
		::Render->add_Visual(hud ? hud_model : world_model);
	}

	for (auto& child : m_childs)
		child->Render(hud);
}

void Bas_Addon::Load(shared_str section)
{
	m_sectionId = section;
	m_boneNameHUD = READ_IF_EXISTS(pSettings, r_string, section, "bone_slot_hud", "wpn_body");
	m_boneNameWORLD = READ_IF_EXISTS(pSettings, r_string, section, "bone_slot_w", "wpn_body");

	m_sectionParent = READ_IF_EXISTS(pSettings, r_string, section, "addon_parent_section", NULL);
	isRoot = READ_IF_EXISTS(pSettings, r_bool, section, "is_root", false);

	m_AttachWorldToMainBody = READ_IF_EXISTS(pSettings, r_bool, section, "attach_w_to_main", false);
	m_UseWorldModel = READ_IF_EXISTS(pSettings, r_bool, section, "use_world", true);

	m_meshName = READ_IF_EXISTS(pSettings, r_string, section, "addon_visual", NULL);
	m_meshHUDName = READ_IF_EXISTS(pSettings, r_string, section, "addon_hud_visual", NULL);

	Msg("Addon Load: %s", m_sectionId.c_str());
}
