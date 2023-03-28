#include "stdafx.h"
#include "../xrCDB/frustum.h"

#pragma warning(disable:4995)
// mmsystem.h
#define MMNOSOUND
#define MMNOMIDI
#define MMNOAUX
#define MMNOMIXER
#define MMNOJOY
//#define OPT_Z
#include <mmsystem.h>
// d3dx9.h
#include <d3dx9.h>
#pragma warning(default:4995)

#include "x_ray.h"
#include "render.h"

// must be defined before include of FS_impl.h
#define INCLUDE_FROM_ENGINE
#include "../xrCore/FS_impl.h"

#ifdef INGAME_EDITOR
# include "../include/editor/ide.hpp"
# include "engine_impl.hpp"
#endif // #ifdef INGAME_EDITOR

#include "xrSash.h"
#include "igame_persistent.h"

#include <imgui\imgui.h>
#include "backends\imgui_impl_dx9.h"
#include "backends\imgui_impl_dx11.h"
#include "backends\imgui_impl_win32.h"

ENGINE_API CRenderDevice Device;
ENGINE_API CLoadScreenRenderer load_screen_renderer;


ENGINE_API BOOL g_bRendering = FALSE;

extern BOOL debugSecondVP;

BOOL g_bLoaded = FALSE;
ref_light precache_light = 0;

// need for imgui
static INT64 g_Time = 0;
static INT64 g_TicksPerSecond = 0;

BOOL CRenderDevice::Begin()
{
	if(g_dedicated_server)
		return true;
    /*
    HW.Validate ();
    HRESULT _hr = HW.pDevice->TestCooperativeLevel();
    if (FAILED(_hr))
    {
    // If the device was lost, do not render until we get it back
    if (D3DERR_DEVICELOST==_hr) {
    Sleep (33);
    return FALSE;
    }

    // Check if the device is ready to be reset
    if (D3DERR_DEVICENOTRESET==_hr)
    {
    Reset ();
    }
    }
    */

    switch (m_pRender->GetDeviceState())
    {
    case IRenderDeviceRender::dsOK:
        break;

    case IRenderDeviceRender::dsLost:
        // If the device was lost, do not render until we get it back
        Sleep(33);
        return FALSE;
        break;

    case IRenderDeviceRender::dsNeedReset:
        // Check if the device is ready to be reset
        Reset();
        break;

    default:
        R_ASSERT(0);
    }

    m_pRender->Begin();

    /*
    CHK_DX (HW.pDevice->BeginScene());
    RCache.OnFrameBegin ();
    RCache.set_CullMode (CULL_CW);
    RCache.set_CullMode (CULL_CCW);
    if (HW.Caps.SceneMode) overdrawBegin ();
    */

    FPU::m24r();
    g_bRendering = TRUE;
    return TRUE;
}

void CRenderDevice::Clear()
{
    m_pRender->Clear();
}

extern void CheckPrivilegySlowdown();


void CRenderDevice::End(void)
{
	if(g_dedicated_server)
		return;
#ifdef INGAME_EDITOR
    bool load_finished = false;
#endif // #ifdef INGAME_EDITOR
    if (dwPrecacheFrame)
    {
        ::Sound->set_master_volume(0.f);
        dwPrecacheFrame--;
        //. pApp->load_draw_internal ();
        if (0 == dwPrecacheFrame)
        {

#ifdef INGAME_EDITOR
            load_finished = true;
#endif // #ifdef INGAME_EDITOR
            //Gamma.Update ();
            m_pRender->updateGamma();

            if (precache_light) precache_light->set_active(false);
            if (precache_light) precache_light.destroy();
            ::Sound->set_master_volume(1.f);
            // pApp->destroy_loading_shaders ();

            m_pRender->ResourcesDestroyNecessaryTextures();
            Memory.mem_compact();
			Msg("* MEMORY USAGE: %lld K", Memory.mem_usage() / 1024);
            Msg("* End of synchronization A[%d] R[%d]", b_is_Active, b_is_Ready);

#ifdef FIND_CHUNK_BENCHMARK_ENABLE
            g_find_chunk_counter.flush();
#endif // FIND_CHUNK_BENCHMARK_ENABLE

            CheckPrivilegySlowdown();

            if (g_pGamePersistent->GameType() == 1)//haCk
            {
                WINDOWINFO wi;
                GetWindowInfo(m_hWnd, &wi);
                if (wi.dwWindowStatus != WS_ACTIVECAPTION)
                    Pause(TRUE, TRUE, TRUE, "application start");
            }
        }
    }

    g_bRendering = FALSE;
    // end scene
    // Present goes here, so call OA Frame end.
    if (g_SASH.IsBenchmarkRunning())
        g_SASH.DisplayFrame(Device.fTimeGlobal);

	extern BOOL g_appLoaded;
	if (g_appLoaded)
	{
		ImGui::Render();
        extern ENGINE_API int g_current_renderer;
		if(g_current_renderer == 1 || g_current_renderer == 2)
			ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
		else
			ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	}

    m_pRender->End();
    //RCache.OnFrameEnd ();
    //Memory.dbg_check ();
    //CHK_DX (HW.pDevice->EndScene());

    //HRESULT _hr = HW.pDevice->Present( NULL, NULL, NULL, NULL );
    //if (D3DERR_DEVICELOST==_hr) return; // we will handle this later
    //R_ASSERT2 (SUCCEEDED(_hr), "Presentation failed. Driver upgrade needed?");
# ifdef INGAME_EDITOR
    if (load_finished && m_editor)
        m_editor->on_load_finished();
# endif // #ifdef INGAME_EDITOR
}


void CRenderDevice::SecondaryThreadProc(void* context)
{
    auto& device = *static_cast<CRenderDevice*>(context);
    while (true)
    {
        device.syncProcessFrame.Wait();
        if (device.mt_bMustExit)
        {
            device.mt_bMustExit = FALSE;
            device.syncThreadExit.Set();
            return;
        }
        for (u32 pit = 0; pit < device.seqParallel.size(); pit++)
            device.seqParallel[pit]();
        device.seqParallel.clear();
        device.seqFrameMT.Process(rp_Frame);
        device.syncFrameDone.Set();
    }
}

#include "igame_level.h"
void CRenderDevice::PreCache(u32 amount, bool b_draw_loadscreen, bool b_wait_user_input)
{
    if (m_pRender->GetForceGPU_REF()) amount = 0;
	if(g_dedicated_server)
		amount = 0;
    // Msg ("* PCACHE: start for %d...",amount);
    dwPrecacheFrame = dwPrecacheTotal = amount;
    if (amount && !precache_light && g_pGameLevel && g_loading_events.empty())
    {
        precache_light = ::Render->light_create();
        precache_light->set_shadow(false);
        precache_light->set_position(vCameraPosition);
        precache_light->set_color(255, 255, 255);
        precache_light->set_range(5.0f);
        precache_light->set_active(true);
    }

    if (amount && b_draw_loadscreen && load_screen_renderer.b_registered == false)
    {
        load_screen_renderer.start(b_wait_user_input);
    }
}


int g_svDedicateServerUpdateReate = 100;

ENGINE_API xr_list<fastdelegate::FastDelegate<bool()>> g_loading_events;

#include "render.h"
#include "IGame_Level.h"

//#define MOVE_CURRENT_FRAME_COUNTR // This is to determine, if the second vp bugs are happening because there were no frame step

bool CRenderDevice::bMainMenuActive()
{
    return  g_pGamePersistent && g_pGamePersistent->m_pMainMenu && g_pGamePersistent->m_pMainMenu->IsActive();
}

#define OPT_Z

void ImGui_NewFrame()
{
	ImGuiIO& io = ImGui::GetIO();

	// Setup display size (every frame to accommodate for window resizing)
	RECT rect;
	GetClientRect(Device.m_hWnd, &rect);
	io.DisplaySize = ImVec2((float)(rect.right - rect.left), (float)(rect.bottom - rect.top));

	if (g_TicksPerSecond == 0) {
		QueryPerformanceFrequency((LARGE_INTEGER *)&g_TicksPerSecond);
		QueryPerformanceCounter((LARGE_INTEGER *)&g_Time);
	}
	// Setup time step
	INT64 current_time;
	QueryPerformanceCounter((LARGE_INTEGER *)&current_time);
	io.DeltaTime = (float)(current_time - g_Time) / g_TicksPerSecond;
	g_Time = current_time;

	// Read keyboard modifiers inputs
	//io.KeyCtrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
	//io.KeyShift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
	//io.KeyAlt =  (GetKeyState(VK_MENU) & 0x8000) != 0;
	//io.KeySuper = false;
	// io.KeysDown : filled by WM_KEYDOWN/WM_KEYUP events
	// io.MousePos : filled by WM_MOUSEMOVE events
	// io.MouseDown : filled by WM_*BUTTON* events
	// io.MouseWheel : filled by WM_MOUSEWHEEL events

	// Hide OS mouse cursor if ImGui is drawing it
	//if (io.MouseDrawCursor)
	//	SetCursor(NULL);

	// Start the frame
    extern ENGINE_API int g_current_renderer;
	if(g_current_renderer == 1 || g_current_renderer == 2)
		ImGui_ImplDX9_NewFrame();
	else
		ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void CRenderDevice::on_idle()
{
    if (!b_is_Ready)
    {
        Sleep(100);
        return;
    }

    const u64 frameStartTime = TimerGlobal.GetElapsed_ms();

    if (psDeviceFlags.test(rsStatistic)) g_bEnableStatGather = TRUE;
    else g_bEnableStatGather = FALSE;
    if (g_loading_events.size())
    {
        if (g_loading_events.front()())
            g_loading_events.pop_front();
        pApp->LoadDraw();
        return;
    }

	if ((!Device.dwPrecacheFrame) && (!g_SASH.IsBenchmarkRunning()) && g_bLoaded)
		g_SASH.StartBenchmark();

	if(!g_dedicated_server)
		ImGui_NewFrame();

	FrameMove();

    // Precache
    if (dwPrecacheFrame)
    {
        float factor = float(dwPrecacheFrame) / float(dwPrecacheTotal);
        float angle = PI_MUL_2 * factor;
        vCameraDirection.set(_sin(angle), 0, _cos(angle));
        vCameraDirection.normalize();
        vCameraTop.set(0, 1, 0);
        vCameraRight.crossproduct(vCameraTop, vCameraDirection);

        mView.build_camera_dir(vCameraPosition, vCameraDirection, vCameraTop);
    }

	//if (m_bMakeLevelMap)
	//{
	//	// build camera matrix
	//	Msg("DDDDFF");
	//	Fbox bb = curr_lm_fbox;
	//	bb.getcenter(vCameraPosition);

	//	vCameraDirection.set(0.f, -1.f, EPS_S);
	//	vCameraTop.set(0.f, 0.f, 1.f);
	//	vCameraRight.set(1.f, 0.f, 0.f);
	//	mView.build_camera_dir(vCameraPosition, vCameraDirection, vCameraTop);

	//	bb.xform(mView);
	//	// build project matrix
	//	mProject.build_projection_ortho(bb.max.x - bb.min.x, bb.max.y - bb.min.y, bb.min.z, bb.max.z);

	//	//Device.mFullTransform.mul(Device.mProject, Device.mView);
	//	//D3DXMatrixInverse((D3DXMATRIX*)&Device.mInvFullTransform, 0, (D3DXMATRIX*)&Device.mFullTransform);
	//	//m_bMakeLevelMap = false;
	//}

	// Render Viewports
	if (Device.m_SecondViewport.IsSVPActive() && Device.m_SecondViewport.IsSVPFrame())
	{
        Render->firstViewPort = MAIN_VIEWPORT;
        Render->lastViewPort = SECONDARY_WEAPON_SCOPE;
        Render->viewPortsThisFrame.push_back(MAIN_VIEWPORT);
        Render->viewPortsThisFrame.push_back(SECONDARY_WEAPON_SCOPE);

	}
	else
	{
		Render->viewPortsThisFrame.push_back(MAIN_VIEWPORT);
		Render->firstViewPort = MAIN_VIEWPORT;
		Render->lastViewPort = MAIN_VIEWPORT;
	}

	Statistic->RenderTOTAL_Real.FrameStart();
	Statistic->RenderTOTAL_Real.Begin();

    u32 t_width = Device.dwWidth, t_height = Device.dwHeight;

#ifdef OPT_Z
	for (size_t i = 0; i < Render->viewPortsThisFrame.size(); i++)
	{
		Render->currentViewPort = Render->viewPortsThisFrame[i];
		Render->needPresenting = (Render->currentViewPort == MAIN_VIEWPORT) ? true : false;

		if (Render->currentViewPort == SECONDARY_WEAPON_SCOPE && (psDeviceFlags.test(rsR3) || psDeviceFlags.test(rsR4)))
		{
			Device.dwWidth = m_SecondViewport.screenWidth;
			Device.dwHeight = m_SecondViewport.screenHeight;
		}

		if (g_pGameLevel)
			g_pGameLevel->ApplyCamera(); // Apply camera params of vp, so that we create a correct full transform matrix

		m_pRender->SetCacheXform(mView, mProject);

		if (Render->currentViewPort == MAIN_VIEWPORT && Device.m_SecondViewport.IsSVPActive()) // need to save main vp stuff for next frame
		{
			mainVPCamPosSaved = vCameraPosition;
			mainVPFullTrans = mFullTransform;
			mainVPViewSaved = mView;
			mainVPProjectSaved = mProject;
		}

		// Set "_saved" for this frame each vport
		vCameraPosition_saved = vCameraPosition;
		mFullTransform_saved = mFullTransform;
		mView_saved = mView;
		mProject_saved = mProject;

		// *** Resume threads
		// Capture end point - thread must run only ONE cycle
		// Release start point - allow thread to run
		if (Render->currentViewPort == MAIN_VIEWPORT)
		{
			syncProcessFrame.Set();
			Sleep(0);
		}

		if (b_is_Active)
		{
			if (Begin())
			{
				seqRender.Process(rp_Render);
				if ((psDeviceFlags.test(rsCameraPos) || psDeviceFlags.test(rsStatistic) || psDeviceFlags.test(rsFPS) || Statistic->errors.size()) && (Render->currentViewPort == MAIN_VIEWPORT || debugSecondVP))
					Statistic->Show();
				End();
			}
		}

		if ((psDeviceFlags.test(rsR3) || psDeviceFlags.test(rsR4)))
		{
			Device.dwWidth = t_width;
			Device.dwHeight = t_height;
		}
	}
#endif // 0

	
	// Restore main vp saved stuff for the needs of new frame
    if (Device.m_SecondViewport.IsSVPActive())
    {
        vCameraPosition_saved = mainVPCamPosSaved;
        mFullTransform_saved = mainVPFullTrans;
        mView_saved = mainVPViewSaved;
        mProject_saved = mainVPProjectSaved;
    }

	Statistic->RenderTOTAL_Real.End();
	Statistic->RenderTOTAL_Real.FrameEnd();
	Statistic->RenderTOTAL.accum = Statistic->RenderTOTAL_Real.accum;
	if(!g_dedicated_server)
		ImGui::EndFrame();

	Render->viewPortsThisFrame.clear();
	
	if (g_pGameLevel) // reapply camera params as for the main vp, for next frame stuff(we dont want to use last vp camera in next frame possible usages)
		g_pGameLevel->ApplyCamera();
    
    const u64 frameEndTime = TimerGlobal.GetElapsed_ms();
    const u64 frameTime = frameEndTime - frameStartTime;

    float fps_to_rate = fps_limit == 900? 0 : 1000.f / fps_limit;

    u32 updateDelta = 1; // 1 ms

    IMainMenu* pMainMenu = g_pGamePersistent ? g_pGamePersistent->m_pMainMenu : 0;

    if (Device.Paused() || bMainMenuActive())
        updateDelta = 16; // 16 ms, ~60 FPS max while paused
    else
        updateDelta = static_cast<u32>(fps_to_rate);
        
    if (fps_to_rate != 0)
    {
        if (frameTime < updateDelta)
            Sleep(updateDelta - frameTime);
    }

    syncFrameDone.Wait(); // wait until secondary thread finish its job
    if (!b_is_Active)
        Sleep(1);
}

void CRenderDevice::message_loop()
{

    MSG msg;
    PeekMessage(&msg, NULL, 0U, 0U, PM_NOREMOVE);
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }

        on_idle();
    }
}

void CRenderDevice::Run()
{
    // DUMP_PHASE;
    g_bLoaded = FALSE;
    Log("Starting engine...");
    thread_name("X-RAY Primary thread");

    // Startup timers and calculate timer delta
    dwTimeGlobal = 0;
    Timer_MM_Delta = 0;
    {
        u32 time_mm = timeGetTime();
        while (timeGetTime() == time_mm); // wait for next tick
        u32 time_system = timeGetTime();
        u32 time_local = TimerAsync();
        Timer_MM_Delta = time_system - time_local;
    }

    // Start all threads
    mt_bMustExit = FALSE;
    thread_spawn(SecondaryThreadProc, "X-RAY Secondary thread", 0, this);

    // Message cycle
    seqAppStart.Process(rp_AppStart);

    //CHK_DX(HW.pDevice->Clear(0,0,D3DCLEAR_TARGET,D3DCOLOR_XRGB(0,0,0),1,0));
    m_pRender->ClearTarget();

    message_loop();

    seqAppEnd.Process(rp_AppEnd);

    // Stop Balance-Thread
    mt_bMustExit = TRUE;
    syncProcessFrame.Set();
    syncThreadExit.Wait();
    while (mt_bMustExit) Sleep(0);
    // DeleteCriticalSection (&mt_csEnter);
    // DeleteCriticalSection (&mt_csLeave);
}

u32 app_inactive_time = 0;
u32 app_inactive_time_start = 0;

void ProcessLoading(RP_FUNC* f);
void CRenderDevice::FrameMove()
{
    dwFrame++;

    Core.dwFrame = dwFrame;

    dwTimeContinual = TimerMM.GetElapsed_ms() - app_inactive_time;

    if (psDeviceFlags.test(rsConstantFPS))
    {
        // 20ms = 50fps
        //fTimeDelta = 0.020f;
        //fTimeGlobal += 0.020f;
        //dwTimeDelta = 20;
        //dwTimeGlobal += 20;
        // 33ms = 30fps
        fTimeDelta = 0.033f;
        fTimeGlobal += 0.033f;
        dwTimeDelta = 33;
        dwTimeGlobal += 33;
    }
    else
    {
        // Timer
        float fPreviousFrameTime = Timer.GetElapsed_sec();
        Timer.Start(); // previous frame
        fTimeDelta = 0.1f * fTimeDelta + 0.9f*fPreviousFrameTime; // smooth random system activity - worst case ~7% error
        //fTimeDelta = 0.7f * fTimeDelta + 0.3f*fPreviousFrameTime; // smooth random system activity
        if (fTimeDelta > .1f)
            fTimeDelta = .1f; // limit to 15fps minimum

        if (fTimeDelta <= 0.f)
            fTimeDelta = EPS_S + EPS_S; // limit to 15fps minimum

        if (Paused())
            fTimeDelta = 0.0f;

        // u64 qTime = TimerGlobal.GetElapsed_clk();
        fTimeGlobal = TimerGlobal.GetElapsed_sec(); //float(qTime)*CPU::cycles2seconds;
        u32 _old_global = dwTimeGlobal;
        dwTimeGlobal = TimerGlobal.GetElapsed_ms();
        dwTimeDelta = dwTimeGlobal - _old_global;
    }

    // Frame move
    Statistic->EngineTOTAL.Begin();

    // TODO: HACK to test loading screen.
    //if(!g_bLoaded)
    ProcessLoading(rp_Frame);
    //else
    // seqFrame.Process (rp_Frame);
    Statistic->EngineTOTAL.End();
}

void ProcessLoading(RP_FUNC* f)
{
    Device.seqFrame.Process(rp_Frame);
    g_bLoaded = TRUE;
}

ENGINE_API BOOL bShowPauseString = TRUE;
#include "IGame_Persistent.h"

void CRenderDevice::Pause(BOOL bOn, BOOL bTimer, BOOL bSound, LPCSTR reason)
{
    static int snd_emitters_ = -1;

    if (g_bBenchmark) return;


#ifdef DEBUG
    // Msg("pause [%s] timer=[%s] sound=[%s] reason=%s",bOn?"ON":"OFF", bTimer?"ON":"OFF", bSound?"ON":"OFF", reason);
#endif // DEBUG

	if(!g_dedicated_server)
	{
		if (bOn)
		{
			if (!Paused())
				bShowPauseString =
	#ifdef DEBUG
					!xr_strcmp(reason, "li_pause_key_no_clip") ? FALSE :
	#endif // DEBUG
					TRUE;

			if (bTimer && (!g_pGamePersistent || g_pGamePersistent->CanBePaused()))
			{
				g_pauseMngr.Pause(TRUE);
	#ifdef DEBUG
				if (!xr_strcmp(reason, "li_pause_key_no_clip"))
					TimerGlobal.Pause(FALSE);
	#endif // DEBUG
			}

			if (bSound && ::Sound)
			{
				snd_emitters_ = ::Sound->pause_emitters(true);
	#ifdef DEBUG
				// Log("snd_emitters_[true]",snd_emitters_);
	#endif // DEBUG
			}
		}
		else
		{
			if (bTimer && /*g_pGamePersistent->CanBePaused() &&*/ g_pauseMngr.Paused())
			{
				fTimeDelta = EPS_S + EPS_S;
				g_pauseMngr.Pause(FALSE);
			}

			if (bSound)
			{
				if (snd_emitters_ > 0) //avoid crash
				{
					snd_emitters_ = ::Sound->pause_emitters(false);
	#ifdef DEBUG
					// Log("snd_emitters_[false]",snd_emitters_);
	#endif // DEBUG
				}
				else
				{
	#ifdef DEBUG
					Log("Sound->pause_emitters underflow");
	#endif // DEBUG
				}
			}
		}
	}
}

BOOL CRenderDevice::Paused()
{
    return g_pauseMngr.Paused();
};

void CRenderDevice::OnWM_Activate(WPARAM wParam, LPARAM lParam)
{
    u16 fActive = LOWORD(wParam);
    BOOL fMinimized = (BOOL)HIWORD(wParam);
    BOOL bActive = ((fActive != WA_INACTIVE) && (!fMinimized)) ? TRUE : FALSE;

    if (bActive != Device.b_is_Active)
    {
        Device.b_is_Active = bActive;

        if (Device.b_is_Active)
        {
            Device.seqAppActivate.Process(rp_AppActivate);
            app_inactive_time += TimerMM.GetElapsed_ms() - app_inactive_time_start;

			if(!g_dedicated_server)
			{
# ifdef INGAME_EDITOR
            if (!editor())
# endif // #ifdef INGAME_EDITOR
                ShowCursor(FALSE);
				/*if (m_hWnd)
				{
					RECT winRect;
					GetWindowRect(m_hWnd, &winRect);
					ClipCursor(&winRect);
				}*/
			}
        }
        else
        {
            app_inactive_time_start = TimerMM.GetElapsed_ms();
            Device.seqAppDeactivate.Process(rp_AppDeactivate);
            ShowCursor(TRUE);
			//ClipCursor(NULL);
        }
    }
}

void CRenderDevice::AddSeqFrame(pureFrame* f, bool mt)
{
    if (mt)
        seqFrameMT.Add(f, REG_PRIORITY_HIGH);
    else
        seqFrame.Add(f, REG_PRIORITY_LOW);

}

void CRenderDevice::RemoveSeqFrame(pureFrame* f)
{
    seqFrameMT.Remove(f);
    seqFrame.Remove(f);
}

CLoadScreenRenderer::CLoadScreenRenderer()
    :b_registered(false)
{}

void CLoadScreenRenderer::start(bool b_user_input)
{
    Device.seqRender.Add(this, 0);
    b_registered = true;
    b_need_user_input = b_user_input;
}

void CLoadScreenRenderer::stop()
{
    if (!b_registered) return;
    Device.seqRender.Remove(this);
    pApp->DestroyLoadingScreen();
    b_registered = false;
    b_need_user_input = false;
}

void CLoadScreenRenderer::OnRender()
{
    pApp->load_draw_internal();
}

void CSecondVPParams::SetSVPActive(bool bState) //--#SM+#-- +SecondVP+
{
	isActive = bState;
	if (g_pGamePersistent != NULL)
		g_pGamePersistent->m_pGShaderConstants->m_blender_mode.z = (isActive ? 1.0f : 0.0f);
}

bool CSecondVPParams::IsSVPFrame() //--#SM+#-- +SecondVP+
{
	return (Device.dwFrame % GetSVPFrameDelay() == 0);
}
