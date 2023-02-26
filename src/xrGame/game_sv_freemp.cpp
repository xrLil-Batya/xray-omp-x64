#include "stdafx.h"
#include "game_sv_freemp.h"
#include "Level.h"
#include "alife_time_manager.h"
#include "alife_simulator.h"
#include "alife_spawn_registry.h"

game_sv_freemp::game_sv_freemp()
	:pure_relcase(&game_sv_freemp::net_Relcase)
{
	m_type = eGameIDFreeMp;

}

game_sv_freemp::~game_sv_freemp()
{
}

void game_sv_freemp::Create(shared_str & options)
{
	inherited::Create(options);
	R_ASSERT2(rpoints[0].size(), "rpoints for players not found");

	switch_Phase(GAME_PHASE_PENDING);

	::Random.seed(GetTickCount());
	m_CorpseList.clear();
}

// player connect #1
void game_sv_freemp::OnPlayerConnect(ClientID id_who)
{
	inherited::OnPlayerConnect(id_who);

	xrClientData* xrCData = m_server->ID_to_client(id_who);
	game_PlayerState*	ps_who = get_id(id_who);

	if (!xrCData->flags.bReconnect)
	{
		ps_who->clear();
		ps_who->team = 0;
		ps_who->skin = -1;
	};
	ps_who->setFlag(GAME_PLAYER_FLAG_SPECTATOR);

	ps_who->resetFlag(GAME_PLAYER_FLAG_SKIP);

	if (g_dedicated_server && (xrCData == m_server->GetServerClient()))
	{
		ps_who->setFlag(GAME_PLAYER_FLAG_SKIP);
		return;
	}
}

// player connect #2
void game_sv_freemp::OnPlayerConnectFinished(ClientID id_who)
{
	xrClientData* xrCData = m_server->ID_to_client(id_who);
	SpawnPlayer(id_who, "spectator");

	if (xrCData)
	{
		R_ASSERT2(xrCData->ps, "Player state not created yet");
		NET_Packet					P;
		GenerateGameMessage(P);
		P.w_u32(GAME_EVENT_PLAYER_CONNECTED);
		P.w_clientID(id_who);
		xrCData->ps->team = 0;
		xrCData->ps->setFlag(GAME_PLAYER_FLAG_SPECTATOR);
		xrCData->ps->setFlag(GAME_PLAYER_FLAG_READY);
		if (HasSaveFile(xrCData->ps))
		{
			xrCData->ps->resetFlag(GAME_PLAYER_MP_SAVE_LOADED);
		}
		else
		{
			xrCData->ps->setFlag(GAME_PLAYER_MP_SAVE_LOADED);
		}
		 

 		xrCData->ps->net_Export(P, TRUE);
		u_EventSend(P);
		xrCData->net_Ready = TRUE;
	};
}

void game_sv_freemp::AddMoneyToPlayer(game_PlayerState * ps, s32 amount)
{
	if (!ps) return;

	Msg("- Add money to player: [%u]%s, %d amount", ps->GameID, ps->getName(), amount);

	s64 total_money = ps->money_for_round;
	total_money += amount;

	if (total_money < 0)
		total_money = 0;

	if (total_money > std::numeric_limits<s32>().max())
	{
		Msg("! The limit of the maximum amount of money has been exceeded.");
		total_money = std::numeric_limits<s32>().max() - 1;
	}

	ps->money_for_round = s32(total_money);
	signal_Syncronize();
}

void game_sv_freemp::SpawnItemToActor(u16 actorId, LPCSTR name)
{
	if (!name) return;

	CSE_Abstract *E = spawn_begin(name);
	E->ID_Parent = actorId;
	E->s_flags.assign(M_SPAWN_OBJECT_LOCAL);	// flags

	CSE_ALifeItemWeapon		*pWeapon = smart_cast<CSE_ALifeItemWeapon*>(E);
	if (pWeapon)
	{
		u16 ammo_magsize = pWeapon->get_ammo_magsize();
		pWeapon->a_elapsed = ammo_magsize;
	}

	CSE_ALifeItemPDA *pPda = smart_cast<CSE_ALifeItemPDA*>(E);
	if (pPda)
	{
		pPda->m_original_owner = actorId;
	}

	spawn_end(E, m_server->GetServerClient()->ID);
}

void game_sv_freemp::OnTransferMoney(NET_Packet & P, ClientID const & clientID)
{
	ClientID to;
	s32 money;

	P.r_clientID(to);
	P.r_s32(money);
	Msg("* Try to transfer money from %u to %u. Amount: %d", clientID.value(), to.value(), money);

	game_PlayerState* ps_from = get_id(clientID);
	if (!ps_from)
	{
		Msg("! Can't find player state with id=%u", clientID.value());
		return;
	}

	game_PlayerState* ps_to = get_id(to);
	if (!ps_to)
	{
		Msg("! Can't find player state with id=%u", to.value());
		return;
	}
	if (money <= 0 || ps_from->money_for_round < money) return;

	AddMoneyToPlayer(ps_from, -money);
	AddMoneyToPlayer(ps_to, money);
}

void game_sv_freemp::RespawnPlayer(ClientID id_who, bool NoSpectator)
{
	inherited::RespawnPlayer(id_who, NoSpectator);

 	game_PlayerState* ps = get_id(id_who);

	if (Game().Type() == eGameIDFreeMp)
	if (ps && !ps->testFlag(GAME_PLAYER_MP_SAVE_LOADED))
	{
		string_path file_name;
		string32 filename;
		xr_strcpy(filename, ps->getName());
		xr_strcat(filename, ".ltx");

		FS.update_path(file_name, "$mp_saves$", filename);

		Msg("read file path = %s", file_name);

		CInifile* file = xr_new<CInifile>(file_name, true);
		LoadPlayer(ps, file);
				
 		ps->setFlag(GAME_PLAYER_MP_SAVE_LOADED);
 	}
}

void game_sv_freemp::OnPlayerReady(ClientID id_who)
{
	switch (Phase())
	{
	case GAME_PHASE_INPROGRESS:
	{
		xrClientData*	xrCData = (xrClientData*)m_server->ID_to_client(id_who);
		game_PlayerState*	ps = get_id(id_who);
		if (ps->IsSkip())
			break;

		if (!(ps->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD)))
			break;

		RespawnPlayer(id_who, true);

	} break;

	default:
		break;
	};
}

// player disconnect
void game_sv_freemp::OnPlayerDisconnect(ClientID id_who, LPSTR Name, u16 GameID)
{
	inherited::OnPlayerDisconnect(id_who, Name, GameID);
}

void game_sv_freemp::OnPlayerKillPlayer(game_PlayerState * ps_killer, game_PlayerState * ps_killed, KILL_TYPE KillType, SPECIAL_KILL_TYPE SpecialKillType, CSE_Abstract * pWeaponA)
{
	if (ps_killed)
	{
		ps_killed->setFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD);
		ps_killed->DeathTime = Device.dwTimeGlobal;
	}
	signal_Syncronize();
}

void game_sv_freemp::OnEvent(NET_Packet &P, u16 type, u32 time, ClientID sender)
{
	switch (type)
	{
	case GAME_EVENT_PLAYER_KILL: // (g_kill)
		{
			u16 ID = P.r_u16();
			xrClientData *l_pC = (xrClientData*)get_client(ID);
			if (!l_pC) break;
			KillPlayer(l_pC->ID, l_pC->ps->GameID);
			l_pC->ps->resetFlag(GAME_PLAYER_MP_SAVE_LOADED);
		}
		break;
	case GAME_EVENT_MP_TRADE:
		{
			OnPlayerTrade(P, sender);
		}
		break;
	case GAME_EVENT_MP_REPAIR:
		{
			OnPlayerRepairItem(P, sender);
		}
		break;
	case GAME_EVENT_MP_INSTALL_UPGRADE:
		{
			OnPlayerInstallUpgrade(P, sender);
		}
		break;
	case GAME_EVENT_TRANSFER_MONEY:
		{
			OnTransferMoney(P, sender);
		}
		break;
	default:
		inherited::OnEvent(P, type, time, sender);
	};
}

#include "Actor.h"
void game_sv_freemp::Update()
{
	inherited::Update();
	if (Phase() != GAME_PHASE_INPROGRESS)
	{
		OnRoundStart();
	}

	if (!g_pGameLevel)
		return;

	if (Level().game && Device.dwFrame % 60 == 0)
	{
		for (auto player : Level().game->players)
		{
			if (player.second->testFlag(GAME_PLAYER_MP_SAVE_LOADED))
			{
				if (player.first == server().GetServerClient()->ID)
					continue;

				CObject* obj = Level().Objects.net_Find(player.second->GameID);
				CActor* actor = smart_cast<CActor*>(obj);
				if (!actor)
					return;
				if (!actor->g_Alive())
					return;

				string_path file_name;
				string32 filename;
				xr_strcpy(filename, player.second->getName());
				xr_strcat(filename, ".ltx");

				FS.update_path(file_name, "$mp_saves$", filename);

				CInifile* file = xr_new<CInifile>(file_name, false, false);
				SavePlayer(player.second, file);
				file->save_as(file_name);
			}
		}

		for (int i = 0; i != server().GetEntitiesNum(); i++)
		{
			CSE_Abstract* abs = server().GetEntity(i);
			CSE_ALifeInventoryBox* box = smart_cast<CSE_ALifeInventoryBox*>(abs);
			if (box)
			{
				string_path path_name;
				string64 invbox_name;
				xr_strcpy(invbox_name, box->name_replace());
				xr_strcat(invbox_name, ".ltx");
				FS.update_path(path_name, "$mp_saves_invbox$", invbox_name);

				bool need_load = true;

				for (auto box_id : inventory_boxes)
				{
					if (box->ID == box_id)
					{
						need_load = false;
					}
				}

				if (need_load)
				{
					inventory_boxes.push_back(box->ID);
					CInifile* boxFile = xr_new<CInifile>(path_name, true);
					LoadInvBox(box, boxFile);
				}
				else
				{
					CInifile* boxFile = xr_new<CInifile>(path_name, false, false);
					SaveInvBox(box, boxFile);
					boxFile->save_as(path_name);
				}
			}
		}
	}
}

BOOL game_sv_freemp::OnTouch(u16 eid_who, u16 eid_what, BOOL bForced)
{
	CSE_ActorMP *e_who = smart_cast<CSE_ActorMP*>(m_server->ID_to_entity(eid_who));
	if (!e_who)
		return TRUE;

	CSE_Abstract *e_entity = m_server->ID_to_entity(eid_what);
	if (!e_entity)
		return FALSE;

	return TRUE;
}

void game_sv_freemp::SetGameTimeFactor(ALife::_TIME_ID GameTime, const float fTimeFactor)
{
	if (ai().get_alife() && ai().alife().initialized())
		return(alife().time_manager().set_game_time_factor(GameTime, fTimeFactor));
	else
		return(inherited::SetGameTimeFactor(fTimeFactor));
}