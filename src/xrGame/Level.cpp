#include "stdafx.h"
#include "xrEngine/FDemoRecord.h"
#include "xrEngine/FDemoPlay.h"
#include "xrEngine/Environment.h"
#include "xrEngine/IGame_Persistent.h"
#include "ParticlesObject.h"
#include "Level.h"
#include "HUDManager.h"
#include "xrServer.h"
#include "NET_Queue.h"
#include "game_cl_base.h"
#include "entity_alive.h"
#include "ai_space.h"
#include "ai_debug.h"
#include "ShootingObject.h"
#include "GameTaskManager.h"
#include "Level_Bullet_Manager.h"
#include "script_process.h"
#include "script_engine.h"
#include "script_engine_space.h"
#include "team_base_zone.h"
#include "infoportion.h"
#include "patrol_path_storage.h"
#include "date_time.h"
#include "space_restriction_manager.h"
#include "seniority_hierarchy_holder.h"
#include "space_restrictor.h"
#include "client_spawn_manager.h"
#include "autosave_manager.h"
#include "ClimableObject.h"
#include "level_graph.h"
#include "mt_config.h"
#include "phcommander.h"
#include "map_manager.h"
#include "xrEngine/CameraManager.h"
#include "level_sounds.h"
#include "car.h"
#include "trade_parameters.h"
#include "game_cl_base_weapon_usage_statistic.h"
#include "MainMenu.h"
#include "xrEngine/XR_IOConsole.h"
#include "actor.h"
#include "player_hud.h"
#include "UI/UIGameTutorial.h"
#include "file_transfer.h"
#include "message_filter.h"
#include "demoplay_control.h"
#include "demoinfo.h"
#include "CustomDetector.h"
#include "xrPhysics/IPHWorld.h"
#include "xrPhysics/console_vars.h"

#include "UIGameCustom.h"
#include "ui/UIPdaWnd.h"
#include "UICursor.h"

#include "level_debug.h"
#include "ai/stalker/ai_stalker.h"
#include "debug_renderer.h"
#include "PhysicObject.h"
#include "PHDebug.h"
#include "debug_text_tree.h"
#include "string_table.h"
#include "embedded_editor/embedded_editor_main.h"
#include "inventory_upgrade_manager.h"
#include "PDA.h"

ENGINE_API bool g_dedicated_server;
extern CUISequencer* g_tutorial;
extern CUISequencer* g_tutorial2;
u32 g_game_flags[GAME_FLAGS_COUNT];

float g_cl_lvInterp = 0.1;
u32 lvInterpSteps = 0;

CLevel::CLevel() :
    NET_CLIENT_CLASS(Device.GetTimerGlobal())
#ifdef PROFILE_CRITICAL_SECTIONS
    , DemoCS(MUTEX_PROFILE_ID(DemoCS))
#endif
{
    g_bDebugEvents = strstr(Core.Params, "-debug_ge") != nullptr;
    game_events = xr_new<NET_Queue_Event>();
    eChangeRP = Engine.Event.Handler_Attach("LEVEL:ChangeRP", this);
    eDemoPlay = Engine.Event.Handler_Attach("LEVEL:PlayDEMO", this);
    eChangeTrack = Engine.Event.Handler_Attach("LEVEL:PlayMusic", this);
    eEnvironment = Engine.Event.Handler_Attach("LEVEL:Environment", this);
    eEntitySpawn = Engine.Event.Handler_Attach("LEVEL:spawn", this);
    m_pBulletManager = xr_new<CBulletManager>();
    m_map_manager = xr_new<CMapManager>();
    m_game_task_manager = xr_new<CGameTaskManager>();
	m_upgrade_manager = nullptr;
    m_dwDeltaUpdate = u32(fixed_step*1000);
    m_seniority_hierarchy_holder = xr_new<CSeniorityHierarchyHolder>();
    m_level_sound_manager = xr_new<CLevelSoundManager>();
    m_space_restriction_manager = xr_new<CSpaceRestrictionManager>();
    m_client_spawn_manager = xr_new<CClientSpawnManager>();
    m_autosave_manager = xr_new<CAutosaveManager>();
    m_debug_renderer = xr_new<CDebugRenderer>();
#ifdef DEBUG
    m_level_debug = xr_new<CLevelDebug>();
#endif
    m_ph_commander = xr_new<CPHCommander>();
    m_ph_commander_scripts = xr_new<CPHCommander>();
    pObjects4CrPr.clear();
    pActors4CrPr.clear();
    g_player_hud = xr_new<player_hud>();
    g_player_hud->load_default();
    Msg("%s", Core.Params);
    m_game_graph = 0;
	m_chunk = 0;
	spawn = 0;
	setup_game_flags();
}

extern CAI_Space *g_ai_space;

CLevel::~CLevel()
{
    xr_delete(g_player_hud);
    delete_data(hud_zones_list);
    hud_zones_list = nullptr;
    Msg("- Destroying level");
    Engine.Event.Handler_Detach(eEntitySpawn, this);
    Engine.Event.Handler_Detach(eEnvironment, this);
    Engine.Event.Handler_Detach(eChangeTrack, this);
    Engine.Event.Handler_Detach(eDemoPlay, this);
    Engine.Event.Handler_Detach(eChangeRP, this);
    if (physics_world())
    {
        destroy_physics_world();
        xr_delete(m_ph_commander_physics_worldstep);
    }
    // destroy PSs
    for (POIt p_it = m_StaticParticles.begin(); m_StaticParticles.end() != p_it; ++p_it)
        CParticlesObject::Destroy(*p_it);
    m_StaticParticles.clear();
    // Unload sounds
    // unload prefetched sounds
    sound_registry.clear();
    // unload static sounds
    for (u32 i = 0; i < static_Sounds.size(); ++i)
    {
        static_Sounds[i]->destroy();
        xr_delete(static_Sounds[i]);
    }
    static_Sounds.clear();
    xr_delete(m_level_sound_manager);
    xr_delete(m_space_restriction_manager);
    xr_delete(m_seniority_hierarchy_holder);
    xr_delete(m_client_spawn_manager);
    xr_delete(m_autosave_manager);
    xr_delete(m_debug_renderer);
    ai().script_engine().remove_script_process(ScriptEngine::eScriptProcessorLevel);
    xr_delete(game);
    xr_delete(game_events);
    xr_delete(m_pBulletManager);
    xr_delete(pStatGraphR);
    xr_delete(pStatGraphS);
    xr_delete(m_ph_commander);
    xr_delete(m_ph_commander_scripts);
    pObjects4CrPr.clear();
    pActors4CrPr.clear();
    ai().unload();
#ifdef DEBUG	
    xr_delete(m_level_debug);
#endif
    xr_delete(m_map_manager);
    delete_data(m_game_task_manager);
	xr_delete(m_upgrade_manager);
    // here we clean default trade params
    // because they should be new for each saved/loaded game
    // and I didn't find better place to put this code in
    // XXX nitrocaster: find better place for this clean()
    CTradeParameters::clean();
    if (g_tutorial && g_tutorial->m_pStoredInputReceiver == this)
        g_tutorial->m_pStoredInputReceiver = nullptr;
    if (g_tutorial2 && g_tutorial2->m_pStoredInputReceiver == this)
        g_tutorial2->m_pStoredInputReceiver = nullptr;
    if (IsDemoPlay())
    {
        StopPlayDemo();
        if (m_reader)
        {
            FS.r_close(m_reader);
            m_reader = nullptr;
        }
    }
    xr_delete(m_msg_filter);
    xr_delete(m_demoplay_control);
    xr_delete(m_demo_info);
    if (IsDemoSave())
    {
        StopSaveDemo();
    }
    deinit_compression();
	xr_delete(m_game_graph);
	m_chunk->close();
	FS.r_close(spawn);
}

void CLevel::setup_game_flags()
{
	g_game_flags[F_DISABLE_WEAPON_FIRE_WHEN_LOOKOUT] = eGameIDDeathmatch | eGameIDTeamDeathmatch | eGameIDArtefactHunt | eGameIDCaptureTheArtefact;
	g_game_flags[F_DISABLE_RENDER_WEAPON_WHEN_LOOKOUT] = eGameIDDeathmatch | eGameIDTeamDeathmatch | eGameIDArtefactHunt | eGameIDCaptureTheArtefact;
	g_game_flags[F_DISABLE_RENDER_WEAPON_CROSSHAIR_WHEN_LOOKOUT] = eGameIDDeathmatch | eGameIDTeamDeathmatch | eGameIDArtefactHunt | eGameIDCaptureTheArtefact;
	g_game_flags[F_USE_BOLT] = eGameIDSingle | eGameIDFreeMp | eGameIDRolePlay;
	g_game_flags[F_USE_NO_ACTIVE_SLOT] = eGameIDSingle | eGameIDFreeMp | eGameIDRolePlay;
	g_game_flags[F_RENDER_ACTOR_HUD_INFO] = eGameIDFreeMp | eGameIDRolePlay;
	g_game_flags[F_SPECTATOR_MOVEMENT_DISABLED] = eGameIDFreeMp | eGameIDRolePlay;
	g_game_flags[F_SPECTATOR_ROTATION_DISABLED] = eGameIDFreeMp | eGameIDRolePlay;
	g_game_flags[F_MUTANTS_DEADBODY_SEARCH_ENABLED] = eGameIDFreeMp | eGameIDRolePlay;
	g_game_flags[F_ZONES_ALWAYS_FAST_MODE] = eGameIDFreeMp | eGameIDRolePlay;
}

shared_str CLevel::name() const
{
    return map_data.m_name;
}

void CLevel::GetLevelInfo(CServerInfo* si)
{
    if (Server && game)
    {
        Server->GetServerInfo(si);
    }
}

void CLevel::PrefetchSound(LPCSTR name)
{
    // preprocess sound name
    string_path tmp;
    xr_strcpy(tmp, name);
    xr_strlwr(tmp);
    if (strext(tmp))
        *strext(tmp) = 0;
    shared_str snd_name = tmp;
    // find in registry
    SoundRegistryMapIt it = sound_registry.find(snd_name);
    // if find failed - preload sound
    if (it == sound_registry.end())
        sound_registry[snd_name].create(snd_name.c_str(), st_Effect, sg_SourceType);
}

// Game interface ////////////////////////////////////////////////////
int	CLevel::get_RPID(LPCSTR /**name/**/)
{
    /*
    // Gain access to string
    LPCSTR	params = pLevel->r_string("respawn_point",name);
    if (0==params)	return -1;

    // Read data
    Fvector4	pos;
    int			team;
    sscanf		(params,"%f,%f,%f,%d,%f",&pos.x,&pos.y,&pos.z,&team,&pos.w); pos.y += 0.1f;

    // Search respawn point
    svector<Fvector4,maxRP>	&rp = Level().get_team(team).RespawnPoints;
    for (int i=0; i<(int)(rp.size()); ++i)
    if (pos.similar(rp[i],EPS_L))	return i;
    */
    return -1;
}

bool g_bDebugEvents = false;

void CLevel::cl_Process_Event(u16 dest, u16 type, NET_Packet& P)
{
    // Msg("--- event[%d] for [%d]",type,dest);
    CObject* O = Objects.net_Find(dest);
    if (0 == O)
    {
#ifdef DEBUG
        Msg("* WARNING: c_EVENT[%d] to [%d]: unknown dest", type, dest);
#endif
        return;
    }
    CGameObject* GO = smart_cast<CGameObject*>(O);
    if (!GO)
    {
#ifndef MASTER_GOLD
        Msg("! ERROR: c_EVENT[%d] : non-game-object", dest);
#endif
        return;
    }
    if (type != GE_DESTROY_REJECT)
    {
        if (type == GE_DESTROY)
        {
            Game().OnDestroy(GO);
        }
        GO->OnEvent(P, type);
    }
    else
    {
        // handle GE_DESTROY_REJECT here
        u32 pos = P.r_tell();
        u16 id = P.r_u16();
        P.r_seek(pos);
        bool ok = true;
        CObject* D = Objects.net_Find(id);
        if (0 == D)
        {
#ifndef MASTER_GOLD
            Msg("! ERROR: c_EVENT[%d] : unknown dest", id);
#endif
            ok = false;
        }
        CGameObject *GD = smart_cast<CGameObject*>(D);
        if (!GD)
        {
#ifndef MASTER_GOLD
            Msg("! ERROR: c_EVENT[%d] : non-game-object", id);
#endif
            ok = false;
        }
        GO->OnEvent(P, GE_OWNERSHIP_REJECT);
        if (ok)
        {
            Game().OnDestroy(GD);
            GD->OnEvent(P, GE_DESTROY);
        }
    }
}

void CLevel::ProcessGameEvents()
{
    // Game events
    {
        NET_Packet P;
        u32 svT = timeServer() - NET_Latency;
        while (game_events->available(svT))
        {
            u16 ID, dest, type;
            game_events->get(ID, dest, type, P);
            switch (ID)
            {
            case M_SPAWN:
            {
                u16 dummy16;
                P.r_begin(dummy16);
                cl_Process_Spawn(P);
                break;
            }
            case M_EVENT:
            {
                cl_Process_Event(dest, type, P);
                break;
            }
            case M_MOVE_PLAYERS:
            {
                u8 Count = P.r_u8();
                for (u8 i = 0; i < Count; i++)
                {
                    u16 ID = P.r_u16();
                    Fvector NewPos, NewDir;
                    P.r_vec3(NewPos);
                    P.r_vec3(NewDir);
                    CActor*	OActor = smart_cast<CActor*>(Objects.net_Find(ID));
                    if (0 == OActor)
                        break;
                    OActor->MoveActor(NewPos, NewDir);
                }
                NET_Packet PRespond;
                PRespond.w_begin(M_MOVE_PLAYERS_RESPOND);
                Send(PRespond, net_flags(TRUE, TRUE));
                break;
            }
            case M_STATISTIC_UPDATE:
            {
                if (GameID() != eGameIDSingle)
                    Game().m_WeaponUsageStatistic->OnUpdateRequest(&P);
                break;
            }
            case M_FILE_TRANSFER:
            {
                if (m_file_transfer) // in case of net_Stop
                    m_file_transfer->on_message(&P);
                break;
            }
            case M_GAMEMESSAGE:
            {
                Game().OnGameMessage(P);
                break;
            }
            default:
            {
                VERIFY(0);
                break;
            }
            }
        }
    }
    if (OnServer() && GameID() != eGameIDSingle)
        Game().m_WeaponUsageStatistic->Send_Check_Respond();
}

#ifdef DEBUG_MEMORY_MANAGER
extern Flags32 psAI_Flags;
extern float debug_on_frame_gather_stats_frequency;

struct debug_memory_guard
{
    inline debug_memory_guard()
    {
        mem_alloc_gather_stats(!!psAI_Flags.test(aiDebugOnFrameAllocs));
        mem_alloc_gather_stats_frequency(debug_on_frame_gather_stats_frequency);
    }
};
#endif

void CLevel::MakeReconnect()
{
    if (!Engine.Event.Peek("KERNEL:disconnect"))
    {
        Engine.Event.Defer("KERNEL:disconnect");
        char const* server_options = nullptr;
        char const* client_options = nullptr;
        if (m_caServerOptions.c_str())
        {
            server_options = xr_strdup(*m_caServerOptions);
        }
        else
        {
            server_options = xr_strdup("");
        }
        if (m_caClientOptions.c_str())
        {
            client_options = xr_strdup(*m_caClientOptions);
        }
        else
        {
            client_options = xr_strdup("");
        }
        Engine.Event.Defer("KERNEL:start", size_t(server_options), size_t(client_options));
    }
}

void CLevel::OnFrame()
{
#ifdef DEBUG_MEMORY_MANAGER
    debug_memory_guard __guard__;
#endif
#ifdef DEBUG
    DBG_RenderUpdate();
#endif
    Fvector	temp_vector;
    m_feel_deny.feel_touch_update(temp_vector, 0.f);
    if (GameID() != eGameIDSingle)
        psDeviceFlags.set(rsDisableObjectsAsCrows, true);
    else
        psDeviceFlags.set(rsDisableObjectsAsCrows, false);
    // commit events from bullet manager from prev-frame
    Device.Statistic->TEST0.Begin();
    BulletManager().CommitEvents();
    Device.Statistic->TEST0.End();
    // Client receive
    if (net_isDisconnected())
    {
        if (OnClient() && GameID() != eGameIDSingle)
        {
#ifdef DEBUG
            Msg("--- I'm disconnected, so clear all objects...");
#endif
            ClearAllObjects();
        }
        Engine.Event.Defer("kernel:disconnect");
        return;
    }
    else
    {
        Device.Statistic->netClient1.Begin();
        ClientReceive();
        Device.Statistic->netClient1.End();
    }
    ProcessGameEvents();
    if (m_bNeed_CrPr)
        make_NetCorrectionPrediction();
    if (!g_dedicated_server)
    {
        if (g_mt_config.test(mtMap))
            Device.seqParallel.emplace_back(fastdelegate::MakeDelegate(m_map_manager, &CMapManager::Update));
        else
            MapManager().Update();
        if (IsGameTypeSingle() && Device.dwPrecacheFrame == 0)
        {
            // XXX nitrocaster: was enabled in x-ray 1.5; to be restored or removed
            if (g_mt_config.test(mtMap)) 
            {
                Device.seqParallel.emplace_back(fastdelegate::MakeDelegate(m_game_task_manager,&CGameTaskManager::UpdateTasks));
            }
            else								
				GameTaskManager().UpdateTasks();
        }
    }
    // Inherited update
    inherited::OnFrame();
    // Draw client/server stats
    if (!g_dedicated_server && psDeviceFlags.test(rsStatistic))
    {
        CGameFont* F = UI().Font().pFontDI;
        if (!psNET_direct_connect)
        {
            if (IsServer())
            {
                const IServerStatistic* S = Server->GetStatistic();
                F->SetHeightI(0.015f);
                F->OutSetI(0.0f, 0.5f);
                F->SetColor(D3DCOLOR_XRGB(0, 255, 0));
                F->OutNext("IN:  %4d/%4d (%2.1f%%)", S->bytes_in_real, S->bytes_in,
                    100.f*float(S->bytes_in_real) / float(S->bytes_in));
                F->OutNext("OUT: %4d/%4d (%2.1f%%)", S->bytes_out_real, S->bytes_out,
                    100.f*float(S->bytes_out_real) / float(S->bytes_out));
                F->OutNext("client_2_sever ping: %d", net_Statistic.getPing());
                F->OutNext("SPS/Sended : %4d/%4d", S->dwBytesPerSec, S->dwBytesSended);
                F->OutNext("sv_urate/cl_urate : %4d/%4d", psNET_ServerUpdate, psNET_ClientUpdate);
                F->SetColor(D3DCOLOR_XRGB(255, 255, 255));
                struct net_stats_functor
                {
                    xrServer* m_server;
                    CGameFont* F;
                    void operator()(IClient* C)
                    {
                        m_server->UpdateClientStatistic(C);
                        F->OutNext("0x%08x: P(%d), BPS(%2.1fK), MRR(%2d), MSR(%2d), Retried(%2d), Blocked(%2d)",
                            //Server->game->get_option_s(*C->Name,"name",*C->Name),
                            C->ID.value(),
                            C->stats.getPing(),
                            float(C->stats.getBPS()),// /1024,
                            C->stats.getMPS_Receive(),
                            C->stats.getMPS_Send(),
                            C->stats.getRetriedCount(),
                            C->stats.dwTimesBlocked);
                    }
                };
                net_stats_functor tmp_functor;
                tmp_functor.m_server = Server;
                tmp_functor.F = F;
                Server->ForEachClientDo(tmp_functor);
            }
            if (IsClient())
            {
                NET_CLIENT_CLASS::UpdateStatistic();
                F->SetHeightI(0.015f);
                F->OutSetI(0.0f, 0.5f);
                F->SetColor(D3DCOLOR_XRGB(0, 255, 0));
                F->OutNext("client_2_sever ping: %d", net_Statistic.getPing());
                F->OutNext("sv_urate/cl_urate : %4d/%4d", psNET_ServerUpdate, psNET_ClientUpdate);
                F->SetColor(D3DCOLOR_XRGB(255, 255, 255));
                F->OutNext("BReceivedPs(%2d), BSendedPs(%2d), Retried(%2d), Blocked(%2d)",
                    net_Statistic.getReceivedPerSec(),
                    net_Statistic.getSendedPerSec(),
                    net_Statistic.getRetriedCount(),
                    net_Statistic.dwTimesBlocked);
#ifdef DEBUG
                if (!pStatGraphR)
                {
                    pStatGraphR = xr_new<CStatGraph>();
                    pStatGraphR->SetRect(50, 700, 300, 68, 0xff000000, 0xff000000);
                    //m_stat_graph->SetGrid(0, 0.0f, 10, 1.0f, 0xff808080, 0xffffffff);
                    pStatGraphR->SetMinMax(0.0f, 65536.0f, 1000);
                    pStatGraphR->SetStyle(CStatGraph::stBarLine);
                    pStatGraphR->AppendSubGraph(CStatGraph::stBarLine);
                }
                pStatGraphR->AppendItem(float(net_Statistic.getBPS()), 0xff00ff00, 0);
                F->OutSet(20.f, 700.f);
                F->OutNext("64 KBS");
#endif
            }
        }
    }
    else
    {
#ifdef DEBUG
        if (pStatGraphR)
            xr_delete(pStatGraphR);
#endif
    }
#ifdef DEBUG
    g_pGamePersistent->Environment().m_paused = m_bEnvPaused;
#endif
	if (OnServer())
		g_pGamePersistent->Environment().SetGameTime(GetEnvironmentGameDayTimeSec(), game->GetEnvironmentGameTimeFactor());
	else
		g_pGamePersistent->Environment().SetGameTimeClient(GetEnvironmentGameDayTimeSec(), game->GetEnvironmentGameTimeFactor());
    ai().script_engine().script_process(ScriptEngine::eScriptProcessorLevel)->update();
    m_ph_commander->update();
    m_ph_commander_scripts->update();
    Device.Statistic->TEST0.Begin();
    BulletManager().CommitRenderSet();
    Device.Statistic->TEST0.End();
    // update static sounds
    if (g_mt_config.test(mtLevelSounds))
    {
        Device.seqParallel.emplace_back(fastdelegate::MakeDelegate(m_level_sound_manager, &CLevelSoundManager::Update));
    }
    else
        m_level_sound_manager->Update();
    // defer LUA-GC-STEP
    if (g_mt_config.test(mtLUA_GC))
        Device.seqParallel.emplace_back(fastdelegate::MakeDelegate(this, &CLevel::script_gc));
    else
        script_gc();
    if (pStatGraphR)
    {
        static float fRPC_Mult = 10.0f;
        static float fRPS_Mult = 1.0f;
        pStatGraphR->AppendItem(float(m_dwRPC)*fRPC_Mult, 0xffff0000, 1);
        pStatGraphR->AppendItem(float(m_dwRPS)*fRPS_Mult, 0xff00ff00, 0);
    }
	ShowEditor();
}

int psLUA_GCSTEP = 10;
void CLevel::script_gc()
{
    lua_gc(ai().script_engine().lua(), LUA_GCSTEP, psLUA_GCSTEP);
}

#ifdef DEBUG_PRECISE_PATH
void test_precise_path();
#endif

#ifdef DEBUG
extern Flags32 dbg_net_Draw_Flags;
#endif

extern void draw_wnds_rects();

void CLevel::OnRender()
{
	// PDA
	if (game && CurrentGameUI())
	{
		const auto pda = &CurrentGameUI()->GetPdaMenu();
		const auto pda_actor = Actor() ? Actor()->GetPDA() : nullptr;
		if (pda && pda->IsShown())
		{
			pda->Draw();
			CUICursor* cursor = &UI().GetUICursor();

			if (cursor)
			{
				static bool need_reset{};
				if (pda_actor && pda_actor->m_bZoomed && CurrentGameUI()->TopInputReceiver() != pda)
					CurrentGameUI()->SetMainInputReceiver(pda, false);

				const bool is_top = CurrentGameUI()->TopInputReceiver() == pda;
				if (pda->IsEnabled() && is_top && !Console->bVisible)
				{
					if (need_reset)
					{
						need_reset = false;
						pda->ResetCursor();
					}

					Frect& pda_border = pda->m_cursor_box;
					Fvector2 cursor_pos = cursor->GetCursorPosition();

					if (!pda_border.in(cursor_pos))
					{
						clamp(cursor_pos.x, pda_border.left, pda_border.right);
						clamp(cursor_pos.y, pda_border.top, pda_border.bottom);
						cursor->SetUICursorPosition(cursor_pos);
					}

					Fvector2 cursor_pos_dif;
					cursor_pos_dif.set(cursor_pos);
					cursor_pos_dif.sub(pda->last_cursor_pos);
					pda->last_cursor_pos.set(cursor_pos);
					pda->MouseMovement(cursor_pos_dif.x, cursor_pos_dif.y);
				}
				else
					need_reset = true;

				cursor->OnRender();
			}
			Render->PdaRenderToTarget();
		}
	}
	::Render->BeforeWorldRender();	//--#SM+#-- +SecondVP+

	//Level().rend
	//debug_renderer().render();
    inherited::OnRender();
    if (!game)
        return;
    Game().OnRender();
    //Device.Statistic->TEST1.Begin();
    BulletManager().Render();
    //Device.Statistic->TEST1.End();

	::Render->AfterWorldRender(); //--#SM+#-- +SecondVP+

    HUD().RenderUI();
	debug_renderer().render();
#ifdef DEBUG
    draw_wnds_rects();
    physics_world()->OnRender();
#endif
#ifdef DEBUG
    if (ai().get_level_graph())
        ai().level_graph().render();
#ifdef DEBUG_PRECISE_PATH
    test_precise_path();
#endif
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(Level().CurrentEntity());
    if (stalker)
        stalker->OnRender();
    if (bDebug)
    {
        for (u32 I = 0; I < Level().Objects.o_count(); I++)
        {
            CObject* _O = Level().Objects.o_get_by_iterator(I);
            CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(_O);
            if (stalker)
                stalker->OnRender();
            CCustomMonster* monster = smart_cast<CCustomMonster*>(_O);
            if (monster)
                monster->OnRender();
            CPhysicObject* physic_object = smart_cast<CPhysicObject*>(_O);
            if (physic_object)
                physic_object->OnRender();
            CSpaceRestrictor* space_restrictor = smart_cast<CSpaceRestrictor*>(_O);
            if (space_restrictor)
                space_restrictor->OnRender();
            CClimableObject* climable = smart_cast<CClimableObject*>(_O);
            if (climable)
                climable->OnRender();
            CTeamBaseZone* team_base_zone = smart_cast<CTeamBaseZone*>(_O);
            if (team_base_zone)
                team_base_zone->OnRender();
            if (GameID() != eGameIDSingle)
            {
                CInventoryItem* pIItem = smart_cast<CInventoryItem*>(_O);
                if (pIItem)
                    pIItem->OnRender();
            }
            if (dbg_net_Draw_Flags.test(dbg_draw_skeleton)) //draw skeleton
            {
                CGameObject* pGO = smart_cast<CGameObject*>	(_O);
                if (pGO && pGO != Level().CurrentViewEntity() && !pGO->H_Parent())
                {
                    if (pGO->Position().distance_to_sqr(Device.vCameraPosition) < 400.0f)
                    {
                        pGO->dbg_DrawSkeleton();
                    }
                }
            }
        }
        //  [7/5/2005]
        if (Server && Server->game) Server->game->OnRender();
        //  [7/5/2005]
        ObjectSpace.dbgRender();
        UI().Font().pFontStat->OutSet(170, 630);
        UI().Font().pFontStat->SetHeight(16.0f);
        UI().Font().pFontStat->SetColor(0xffff0000);
        if (Server)
            UI().Font().pFontStat->OutNext("Client Objects:      [%d]", Server->GetEntitiesNum());
        UI().Font().pFontStat->OutNext("Server Objects:      [%d]", Objects.o_count());
        UI().Font().pFontStat->OutNext("Interpolation Steps: [%d]", Level().GetInterpolationSteps());
        if (Server)
        {
            UI().Font().pFontStat->OutNext("Server updates size: [%d]", Server->GetLastUpdatesSize());
        }
        UI().Font().pFontStat->SetHeight(8.0f);
    }
#endif

#ifdef DEBUG
    if (bDebug)
    {
        DBG().draw_object_info();
        DBG().draw_text();
        DBG().draw_level_info();
    }
    debug_renderer().render();
    DBG().draw_debug_text();
    if (psAI_Flags.is(aiVision))
    {
        for (u32 I = 0; I < Level().Objects.o_count(); I++)
        {
            CObject* object = Objects.o_get_by_iterator(I);
            CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(object);
            if (!stalker)
                continue;
            stalker->dbg_draw_vision();
        }
    }


    if (psAI_Flags.test(aiDrawVisibilityRays))
    {
        for (u32 I = 0; I < Level().Objects.o_count(); I++)
        {
            CObject* object = Objects.o_get_by_iterator(I);
            CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(object);
            if (!stalker)
                continue;
            stalker->dbg_draw_visibility_rays();
        }
    }
#endif
}

void CLevel::OnEvent(EVENT E, u64 P1, u64 /**P2/**/)
{
    if (E == eEntitySpawn)
    {
        char	Name[128];	Name[0] = 0;
        sscanf(LPCSTR(P1), "%s", Name);
        Level().g_cl_Spawn(Name, 0xff, M_SPAWN_OBJECT_LOCAL, Fvector().set(0, 0, 0));
    }
    else if (E == eChangeRP && P1)
    {
    }
    else if (E == eDemoPlay && P1)
    {
        char* name = (char*)P1;
        string_path RealName;
        xr_strcpy(RealName, name);
        xr_strcat(RealName, ".xrdemo");
        Cameras().AddCamEffector(xr_new<CDemoPlay>(RealName, 1.3f, 0));
    }
    else if (E == eChangeTrack && P1)
    {
        // int id = atoi((char*)P1);
        // Environment->Music_Play(id);
    }
    else if (E == eEnvironment)
    {
        // int id=0; float s=1;
        // sscanf((char*)P1,"%d,%f",&id,&s);
        // Environment->set_EnvMode(id,s);
    }
}

void CLevel::AddObject_To_Objects4CrPr(CGameObject* pObj)
{
    if (!pObj)
        return;
    for (CGameObject* obj : pObjects4CrPr)
    {
        if (obj == pObj)
            return;
    }
    pObjects4CrPr.push_back(pObj);

}
void CLevel::AddActor_To_Actors4CrPr(CGameObject* pActor)
{
    if (!pActor)
        return;
    if (!smart_cast<CActor*>(pActor)) return;
    for (CGameObject* act : pActors4CrPr)
    {
        if (act == pActor)
            return;
    }
    pActors4CrPr.push_back(pActor);
}

void CLevel::RemoveObject_From_4CrPr(CGameObject* pObj)
{
    if (!pObj)
        return;
    auto objIt = std::find(pObjects4CrPr.begin(), pObjects4CrPr.end(), pObj);
    if (objIt != pObjects4CrPr.end())
    {
        pObjects4CrPr.erase(objIt);
    }
    auto aIt = std::find(pActors4CrPr.begin(), pActors4CrPr.end(), pObj);
    if (aIt != pActors4CrPr.end())
    {
        pActors4CrPr.erase(aIt);
    }
}

void CLevel::make_NetCorrectionPrediction()
{
    m_bNeed_CrPr = false;
    m_bIn_CrPr = true;
    u64 NumPhSteps = physics_world()->StepsNum();
    physics_world()->StepsNum() -= m_dwNumSteps;
    if (ph_console::g_bDebugDumpPhysicsStep&&m_dwNumSteps > 10)
    {
        Msg("!!!TOO MANY PHYSICS STEPS FOR CORRECTION PREDICTION = %d !!!", m_dwNumSteps);
        m_dwNumSteps = 10;
    }
    physics_world()->Freeze();
    //setting UpdateData and determining number of PH steps from last received update
    for (CGameObject* obj : pObjects4CrPr)
    {
        if (!obj)
            continue;
        obj->PH_B_CrPr();
    }
    //first prediction from "delivered" to "real current" position
    //making enought PH steps to calculate current objects position based on their updated state
    for (u32 i = 0; i < m_dwNumSteps; i++)
    {
        physics_world()->Step();

        for (CGameObject* act : pActors4CrPr)
        {
            if (!act || act->CrPr_IsActivated())
                continue;
            act->PH_B_CrPr();
        }
    }
    for (CGameObject* obj : pObjects4CrPr)
    {
        if (!obj)
            continue;
        obj->PH_I_CrPr();
    }
    if (!InterpolationDisabled())
    {
        for (u32 i = 0; i < lvInterpSteps; i++)	//second prediction "real current" to "future" position
        {
            physics_world()->Step();
        }
        for (CGameObject* obj : pObjects4CrPr)
        {
            if (!obj)
                continue;
            obj->PH_A_CrPr();
        }
    }
    physics_world()->UnFreeze();
    physics_world()->StepsNum() = NumPhSteps;
    m_dwNumSteps = 0;
    m_bIn_CrPr = false;
    pObjects4CrPr.clear();
    pActors4CrPr.clear();
}

u32 CLevel::GetInterpolationSteps()
{
    return lvInterpSteps;
}

void CLevel::UpdateDeltaUpd(u32 LastTime)
{
    u32 CurrentDelta = LastTime - m_dwLastNetUpdateTime;
    if (CurrentDelta < m_dwDeltaUpdate)
        CurrentDelta = iFloor(float(m_dwDeltaUpdate * 10 + CurrentDelta) / 11);
    m_dwLastNetUpdateTime = LastTime;
    m_dwDeltaUpdate = CurrentDelta;
    if (0 == g_cl_lvInterp)
        ReculcInterpolationSteps();
    else
    if (g_cl_lvInterp > 0)
    {
        lvInterpSteps = iCeil(g_cl_lvInterp / fixed_step);
    }
}

void CLevel::ReculcInterpolationSteps()
{
    lvInterpSteps = iFloor(float(m_dwDeltaUpdate) / (fixed_step * 1000));
    if (lvInterpSteps > 60)
        lvInterpSteps = 60;
    if (lvInterpSteps < 3)
        lvInterpSteps = 3;
}

bool CLevel::InterpolationDisabled()
{
    return g_cl_lvInterp < 0;
}

void CLevel::PhisStepsCallback(u32 Time0, u32 Time1)
{
    if (!Level().game)
        return;
    if (GameID() == eGameIDSingle)
        return;
    //#pragma todo("Oles to all: highly inefficient and slow!!!")
    //fixed (Andy)
    /*
    for (xr_vector<CObject*>::iterator O=Level().Objects.objects.begin(); O!=Level().Objects.objects.end(); ++O)
    {
    if( smart_cast<CActor*>((*O)){
    CActor* pActor = smart_cast<CActor*>(*O);
    if (!pActor || pActor->Remote()) continue;
    pActor->UpdatePosStack(Time0, Time1);
    }
    };
    */
}

void CLevel::SetNumCrSteps(u32 NumSteps)
{
    m_bNeed_CrPr = true;
    if (m_dwNumSteps > NumSteps)
        return;
    m_dwNumSteps = NumSteps;
    if (m_dwNumSteps > 1000000)
    {
        VERIFY(0);
    }
}

ALife::_TIME_ID CLevel::GetStartGameTime()
{
    return (game->GetStartGameTime());
}

ALife::_TIME_ID CLevel::GetGameTime()
{
    return (game->GetGameTime());
}

ALife::_TIME_ID CLevel::GetEnvironmentGameTime()
{
    return (game->GetEnvironmentGameTime());
}

u8 CLevel::GetDayTime()
{
    u32 dummy32, hours;
    GetGameDateTime(dummy32, dummy32, dummy32, hours, dummy32, dummy32, dummy32);
    VERIFY(hours < 256);
    return u8(hours);
}

float CLevel::GetGameDayTimeSec()
{
    return (float(s64(GetGameTime() % (24 * 60 * 60 * 1000))) / 1000.f);
}

u32 CLevel::GetGameDayTimeMS()
{
    return (u32(s64(GetGameTime() % (24 * 60 * 60 * 1000))));
}

float CLevel::GetEnvironmentGameDayTimeSec()
{
    return (float(s64(GetEnvironmentGameTime() % (24 * 60 * 60 * 1000))) / 1000.f);
}

void CLevel::GetGameDateTime(u32& year, u32& month, u32& day, u32& hours, u32& mins, u32& secs, u32& milisecs)
{
    split_time(GetGameTime(), year, month, day, hours, mins, secs, milisecs);
}

float CLevel::GetEnvironmentTimeFactor() const
{
	if (!game)
		return 0.0f;
	return game->GetEnvironmentGameTimeFactor();
}

void CLevel::SetEnvironmentTimeFactor(const float fTimeFactor)
{
	if (!game)
		return;
	game->SetEnvironmentGameTimeFactor(fTimeFactor);
}

u64 CLevel::GetEnvironmentGameTime() const
{
	if (!game)
		return 0;
	return game->GetEnvironmentGameTime();
}

float CLevel::GetGameTimeFactor()
{
    return (game->GetGameTimeFactor());
}

void CLevel::SetGameTimeFactor(const float fTimeFactor)
{
    game->SetGameTimeFactor(fTimeFactor);
}

void CLevel::SetGameTimeFactor(ALife::_TIME_ID GameTime, const float fTimeFactor)
{
    game->SetGameTimeFactor(GameTime, fTimeFactor);
}

void CLevel::SetEnvironmentGameTimeFactor(u64 const& GameTime, float const& fTimeFactor)
{
    if (!game)
        return;
    game->SetEnvironmentGameTimeFactor(GameTime, fTimeFactor);
}

bool CLevel::IsServer()
{
    if (!Server || IsDemoPlayStarted())
        return false;
    return true;
}

bool CLevel::IsClient()
{
    if (IsDemoPlayStarted())
        return true;
    if (Server)
        return false;
    return true;
}

void CLevel::OnAlifeSimulatorUnLoaded()
{
    MapManager().ResetStorage();
    GameTaskManager().ResetStorage();
	xr_delete(m_upgrade_manager);
}

void CLevel::OnAlifeSimulatorLoaded()
{
    MapManager().ResetStorage();
    GameTaskManager().ResetStorage();
	// moved from alife simulator for supporting in MP
	// only for single and server
	// for client mangager creates in Load_GameSpecific_Before()
	R_ASSERT(m_upgrade_manager == nullptr);
	m_upgrade_manager = xr_new<inventory::upgrade::Manager>();
}

void CLevel::OnSessionTerminate(LPCSTR reason)
{
	MainMenu()->OnSessionTerminate(CStringTable().translate(reason).c_str());
}

#include "../xrEngine/CameraManager.h"
#include "ActorEffector.h"

void CLevel::ApplyCamera()
{
	inherited::ApplyCamera();

	/*if (g_actor)
	{
		Actor()->Cameras().ApplyDevice(VIEWPORT_NEAR);		
	}*/

	if (lastApplyCameraVPNear > -1.f)
		lastApplyCamera(lastApplyCameraVPNear);

}

u32	GameID()
{
    return Game().Type();
}

CZoneList* CLevel::create_hud_zones_list()
{
    hud_zones_list = xr_new<CZoneList>();
    hud_zones_list->clear();
    return hud_zones_list;
}

bool CZoneList::feel_touch_contact(CObject* O)
{
    TypesMapIt it = m_TypesMap.find(O->cNameSect());
    bool res = (it != m_TypesMap.end());
    CCustomZone *pZone = smart_cast<CCustomZone*>(O);
    if (pZone && !pZone->IsEnabled())
    {
        res = false;
    }
    return res;
}

CZoneList::CZoneList()
{
}

CZoneList::~CZoneList()
{
    clear();
    destroy();
}
