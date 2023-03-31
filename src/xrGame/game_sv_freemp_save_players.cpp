#include "stdafx.h"
#include "game_sv_freemp.h"
#include "Level.h"
#include "Actor.h"
#include "Inventory.h"
#include "Weapon.h"
#include "CustomDetector.h"

void game_sv_freemp::SavePlayer(game_PlayerState* ps, CInifile* file)
{
	CObject* obj = Level().Objects.net_Find(ps->GameID);
	CActor* actor = smart_cast<CActor*>(obj);
  
	if (actor && actor->g_Alive())
	{
		TIItemContainer items;
		actor->inventory().AddAvailableItems(items, false);
		u32 id = 0;
		
		for (auto item : items)
		{
			if (!xr_strcmp("mp_players_rukzak", item->m_section_id.c_str()))
				continue;

			id += 1;
			string32 itemID;
			xr_sprintf(itemID, "item_%d", id);
 
			file->w_string(itemID, "section", item->m_section_id.c_str());
			file->w_u16(itemID, "slot", item->CurrValue());
			
			if (item->GetCondition() < 1)
				file->w_float(itemID, "condition", item->GetCondition());
			
			file->w_u32(itemID, "upgrades_count", item->upgardes().size());
			u32 iterator = 0;
			for (const auto upgrade : item->upgardes())
			{
				string64 upgrade_line{};
				strconcat(sizeof(upgrade_line), upgrade_line, "installed_upgrade", std::to_string(iterator).c_str());
				file->w_string(itemID, upgrade_line, upgrade.c_str());
				iterator++;
			}
			
			if (item->cast_weapon_ammo())
			{
				CWeaponAmmo* ammo = smart_cast<CWeaponAmmo*>(item);
				file->w_u16(itemID, "ammo_count", ammo->m_boxCurr);
			}

			if (item->cast_weapon())
			{
				CWeapon* wpn = smart_cast<CWeapon*>(item);
				file->w_u16(itemID, "ammo_count", u16(wpn->GetAmmoElapsed()));
				file->w_u8(itemID, "ammo_type", wpn->m_ammoType);
				file->w_u8(itemID, "addon_State", wpn->GetAddonsState());
				file->w_u8(itemID, "cur_scope", wpn->m_cur_scope);
			}
		}

		CCustomDetector* detector = smart_cast<CCustomDetector*>(actor->inventory().ItemFromSlot(DETECTOR_SLOT));

		if (detector)
		{
			file->w_string("detector", "section", detector->m_section_id.c_str());
		}

		file->w_u32("actor", "items_count", id);
		file->w_u32("actor", "money", ps->money_for_round);
		file->w_u8("actor", "team", ps->team);
	}
}

bool game_sv_freemp::LoadPlayer(game_PlayerState* ps, CInifile* file)
{
	if (file->section_exist("actor"))
	{
		u32 count = file->r_u32("actor", "items_count");
		ps->money_for_round = file->r_u32("actor", "money");
		SpawnItemToActor(ps->GameID, "device_torch");
		SpawnItemToActor(ps->GameID, "wpn_knife");
		SpawnItemToActor(ps->GameID, "device_pda");
		SpawnItemToActor(ps->GameID, "wpn_binoc");

		Msg("[game_sv_freemp] LoadPlayer [%s] items[%d]", ps->getName(), count);

		if (file->section_exist("detector"))
		{
			LPCSTR name = file->r_string("detector", "section");

			CSE_Abstract* E = spawn_begin(name);
			E->ID_Parent = ps->GameID;
			CSE_ALifeInventoryItem* item = smart_cast<CSE_ALifeInventoryItem*>(E);
			spawn_end(E, m_server->GetServerClient()->ID);
		}

		for (u32 id = 1; id != count + 1; id++)
		{
			string32 itemID;
			xr_sprintf(itemID, "item_%d", id);


			if (file->section_exist(itemID))
			{
				LPCSTR name = file->r_string(itemID, "section");

				CSE_Abstract* E = spawn_begin(name);

				E->ID_Parent = ps->GameID;

				CSE_ALifeItem* item = smart_cast<CSE_ALifeItem*>(E);
				CSE_ALifeItemWeapon* wpn = smart_cast<CSE_ALifeItemWeapon*>(item);
				CSE_ALifeItemAmmo* ammo = smart_cast<CSE_ALifeItemAmmo*>(item);

				if (file->line_exist(itemID, "condition"))
				{
					float cond = file->r_float(itemID, "condition");
					item->m_fCondition = cond;
				}

				if(file->line_exist(itemID, "upgrades_count"))
				{
					const auto pSE_InventoryItem = smart_cast<CSE_ALifeInventoryItem*>(E);
					if(pSE_InventoryItem)
					{
						u32 size = file->r_u32(itemID, "upgrades_count");
						for(u32 iterator = 0; iterator < size; iterator++)
						{
							string64 upgrade_line{};
							strconcat(sizeof(upgrade_line), upgrade_line, "installed_upgrade", std::to_string(iterator).c_str());
							pSE_InventoryItem->m_upgrades.push_back(file->r_string(itemID, upgrade_line));
						}
					}
				}

				if (wpn)
				{
					u16 ammo_count = file->r_u16(itemID, "ammo_count");
					u8 ammo_type = file->r_u8(itemID, "ammo_type");
					u8 addon_state = file->r_u8(itemID, "addon_State");
					u8 cur_scope = file->r_u8(itemID, "cur_scope");
					wpn->a_elapsed = ammo_count;
					wpn->ammo_type = ammo_type;
					wpn->m_addon_flags.flags = addon_state;
					wpn->cur_scope = cur_scope;
				}

				if (ammo)
				{
					u16 ammo_current = file->r_u16(itemID, "ammo_count");
					ammo->a_elapsed = ammo_current;
				}

				if (file->line_exist(itemID, "slot"))
				{
					u16 slot = file->r_u16(itemID, "slot");
					item->slot = slot;
				}

				spawn_end(E, m_server->GetServerClient()->ID);
			}
		}
		 
		return true;
	}
	else
		return false;
}

bool game_sv_freemp::HasSaveFile(game_PlayerState* ps)
{
	if (ps->GameID == get_id(server().GetServerClient()->ID)->GameID)
		return false;

	string_path path;
 	string32 filename;
	xr_strcpy(filename, ps->getName());
	xr_strcat(filename, ".ltx");
	FS.update_path(path, "$mp_saves$", filename);
	bool exist = false;
	FS.rescan_path(path, true);


	if (FS.exist(path))
	{
		CInifile* file = xr_new<CInifile>(path, true);
		if (file)  
		if (file->section_exist("actor"))
		if (file->line_exist("actor", "team"))
			ps->team = file->r_u8("actor", "team");

		exist = file->section_exist("actor");
		file->Destroy(file);
	}

	return exist;
}

void game_sv_freemp::SaveInvBox(CSE_ALifeInventoryBox* box, CInifile* file)
{
	u32 ids = 0;
	for (auto id : box->children)
	{
		ids += 1;
		string32 itemID;
		xr_sprintf(itemID, "item_%d", ids);

		CInventoryItem* item = smart_cast<CInventoryItem*>( Level().Objects.net_Find(id));
		if (!item)
			return;

		file->w_string(itemID, "section", item->m_section_id.c_str());
 
		if (item->GetCondition() < 1)
			file->w_float(itemID, "condition", item->GetCondition());

		if (item->cast_weapon_ammo())
		{
			CWeaponAmmo* ammo = smart_cast<CWeaponAmmo*>(item);
			file->w_u16(itemID, "ammo_count", ammo->m_boxCurr);
		}

		if (item->cast_weapon())
		{
			CWeapon* wpn = smart_cast<CWeapon*>(item);
			file->w_u16(itemID, "ammo_count", u16(wpn->GetAmmoElapsed()));
			file->w_u8(itemID, "ammo_type", wpn->m_ammoType);
			file->w_u8(itemID, "addon_State", wpn->GetAddonsState());
			file->w_u8(itemID, "cur_scope", wpn->m_cur_scope);
		}
	}

	file->w_u32("box", "items_count", ids);

}

void game_sv_freemp::LoadInvBox(CSE_ALifeInventoryBox* box, CInifile* file)
{
	if (!file->section_exist("box"))
		return;
	if (!box)
		return;

	u32 count = file->r_u32("box", "items_count");

	//Msg("[game_sv_freemp] LoadInvBox [%d] items[%d]", box->ID, count);
    
	for (u32 id = 1; id != count + 1; id++)
	{
		string32 itemID;
		xr_sprintf(itemID, "item_%d", id);
 
		if (file->section_exist(itemID))
		{
			LPCSTR name = file->r_string(itemID, "section");

			CSE_Abstract* E = spawn_begin(name);

			E->ID_Parent = box->ID;

			CSE_ALifeItem* item = smart_cast<CSE_ALifeItem*>(E);
			CSE_ALifeItemWeapon* wpn = smart_cast<CSE_ALifeItemWeapon*>(item);
			CSE_ALifeItemAmmo* ammo = smart_cast<CSE_ALifeItemAmmo*>(item);

			if (file->line_exist(itemID, "condition"))
			{
				float cond = file->r_float(itemID, "condition");
				item->m_fCondition = cond;
			}

			if (wpn)
			{
				u16 ammo_count = file->r_u16(itemID, "ammo_count");
				u8 ammo_type = file->r_u8(itemID, "ammo_type");
				u8 addon_state = file->r_u8(itemID, "addon_State");
				u8 cur_scope = file->r_u8(itemID, "cur_scope");
				wpn->a_elapsed = ammo_count;
				wpn->ammo_type = ammo_type;
				wpn->m_addon_flags.flags = addon_state;
				wpn->cur_scope = cur_scope;
			}

			if (ammo)
			{
				u16 ammo_current = file->r_u16(itemID, "ammo_count");
				ammo->a_elapsed = ammo_current;
			}

			spawn_end(E, m_server->GetServerClient()->ID);
		}
	}
}