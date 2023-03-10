// EngineAPI.cpp: implementation of the CEngineAPI class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "EngineAPI.h"
#include "../xrcdb/xrXRC.h"
#include "XR_IOConsole.h"

#define EXCLUDE_R2_AND_R3
extern xr_token* vid_quality_token;
bool extern ENGINE_API g_dedicated_server;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

void __cdecl dummy(void)
{
};
CEngineAPI::CEngineAPI()
{
    hGame = 0;
    hRender = 0;
    hTuner = 0;
    pCreate = 0;
    pDestroy = 0;
    tune_enabled = false;
    tune_pause = dummy;
    tune_resume = dummy;
}

CEngineAPI::~CEngineAPI()
{
    // destroy quality token here
    if (vid_quality_token)
    {
        for (int i = 0; vid_quality_token[i].name; i++)
        {
            xr_free(vid_quality_token[i].name);
        }
        xr_free(vid_quality_token);
        vid_quality_token = nullptr;
    }
}

extern u32 renderer_value; //con cmd
ENGINE_API int g_current_renderer = 0;

ENGINE_API bool is_enough_address_space_available()
{
    SYSTEM_INFO system_info;
    GetSystemInfo(&system_info);
    return (*(u32*)&system_info.lpMaximumApplicationAddress) > 0x90000000;
}

void CEngineAPI::InitializeNotDedicated()
{
#ifndef EXCLUDE_R2_AND_R3
    constexpr const char* r2_name = "xrRender_R2.dll";
    constexpr const char* r3_name = "xrRender_R3.dll";
#endif
    constexpr const char* r4_name = "xrRender_R4.dll";

    if (psDeviceFlags.test(rsR4))
    {
        psDeviceFlags.set(rsR3, false);
        psDeviceFlags.set(rsR2, false);
        // try to initialize R4
        Log("Loading DLL:", r4_name);
        hRender = LoadLibrary(r4_name);
        if (!hRender)
        {
            // try to load R1
            Msg("! ...Failed - incompatible hardware/pre-Vista OS.");
#ifndef EXCLUDE_R2_AND_R3
            psDeviceFlags.set(rsR3, true);
#endif
        }
		else
		{
			psDeviceFlags.set(rsR4, true);
			g_current_renderer = 3;
		}
    }

#ifndef EXCLUDE_R2_AND_R3
    if (psDeviceFlags.test(rsR3))
    {
        // try to initialize R3
        Log("Loading DLL:", r3_name);
        hRender = LoadLibrary(r3_name);
        if (!hRender)
        {
            // try to load R1
            Msg("! ...Failed - incompatible hardware/pre-Vista OS.");
            psDeviceFlags.set(rsR2, TRUE);
        }
        else
            g_current_renderer = 3;
    }

    if (psDeviceFlags.test(rsR2))
    {
        // try to initialize R2
        psDeviceFlags.set(rsR4, FALSE);
        psDeviceFlags.set(rsR3, FALSE);
        Log("Loading DLL:", r2_name);
        hRender = LoadLibrary(r2_name);
        if (!hRender)
        {
            // try to load R1
            Msg("! ...Failed - incompatible hardware.");
        }
        else
            g_current_renderer = 2;
    }
#endif
}


void CEngineAPI::Initialize(void)
{
    //////////////////////////////////////////////////////////////////////////
    // render
    constexpr const char* r1_name = "xrRender_R1.dll";
	if(!g_dedicated_server)
		InitializeNotDedicated();

    if (!hRender)
    {
		Console->Execute("renderer renderer_r1");
		
        // try to load R1
        psDeviceFlags.set(rsR4, FALSE);
        psDeviceFlags.set(rsR3, FALSE);
        psDeviceFlags.set(rsR2, FALSE);
        renderer_value = 0; //con cmd

        Log("Loading DLL:", r1_name);
        hRender = LoadLibrary(r1_name);
        if (!hRender) R_CHK(GetLastError());
        R_ASSERT(hRender);
        g_current_renderer = 1;
    }

    Device.ConnectToRender();

    // game
    {
        LPCSTR g_name = "xrGame.dll";
        Log("Loading DLL:", g_name);
        hGame = LoadLibrary(g_name);
        if (!hGame) R_CHK(GetLastError());
        R_ASSERT2(hGame, "Game DLL raised exception during loading or there is no game DLL at all");
        pCreate = (Factory_Create*)GetProcAddress(hGame, "xrFactory_Create");
        R_ASSERT(pCreate);
        pDestroy = (Factory_Destroy*)GetProcAddress(hGame, "xrFactory_Destroy");
        R_ASSERT(pDestroy);
    }

    //////////////////////////////////////////////////////////////////////////
    // vTune
    if (strstr(Core.Params, "-tune"))
    {
        LPCSTR g_name = "vTuneAPI.dll";
        Log("Loading DLL:", g_name);
        hTuner = LoadLibrary(g_name);
        if (0 == hTuner) R_CHK(GetLastError());
        R_ASSERT2(hTuner, "Intel vTune is not installed");
        tune_enabled = TRUE;
        tune_pause = (VTPause*)GetProcAddress(hTuner, "VTPause");
        R_ASSERT(tune_pause);
        tune_resume = (VTResume*)GetProcAddress(hTuner, "VTResume");
        R_ASSERT(tune_resume);
    }
}

void CEngineAPI::Destroy(void)
{
    if (hGame) { FreeLibrary(hGame); hGame = 0; }
    if (hRender) { FreeLibrary(hRender); hRender = 0; }
    pCreate = 0;
    pDestroy = 0;
    Engine.Event._destroy();
    XRC.r_clear_compact();
}

extern "C" {
#ifndef EXCLUDE_R2_AND_R3
    typedef bool __cdecl SupportsAdvancedRendering(void);
    typedef bool _declspec(dllexport) SupportsDX10Rendering();
#endif
    typedef bool _declspec(dllexport) SupportsDX11Rendering();
};

void CEngineAPI::CreateRendererList()
{
	if(g_dedicated_server)
	{
		vid_quality_token = xr_alloc<xr_token>(2);

		vid_quality_token[0].id = 0;
		vid_quality_token[0].name = xr_strdup("renderer_r1");

		vid_quality_token[1].id = -1;
		vid_quality_token[1].name = nullptr;
		return;
	}
    // TODO: ask renderers if they are supported!
    if (vid_quality_token) return;
#ifndef EXCLUDE_R2_AND_R3
    bool bSupports_r2 = false;
    bool bSupports_r2_5 = false;
    bool bSupports_r3 = false;
#endif
    bool bSupports_r4 = false;

#ifndef EXCLUDE_R2_AND_R3
    constexpr const char* r2_name = "xrRender_R2.dll";
    constexpr const char* r3_name = "xrRender_R3.dll";
#endif
    constexpr const char* r4_name = "xrRender_R4.dll";

    if (strstr(Core.Params, "-perfhud_hack"))
    {
        R_ASSERT(0); // ?????? ???? ???????????? ???????? ??????????????
    }
    else
    {
#ifndef EXCLUDE_R2_AND_R3
        // try to initialize R2
        Log("Loading DLL:", r2_name);
        hRender = LoadLibrary(r2_name);
        if (hRender)
        {
            bSupports_r2 = true;
            SupportsAdvancedRendering* test_rendering = (SupportsAdvancedRendering*)GetProcAddress(hRender, "SupportsAdvancedRendering");
            R_ASSERT(test_rendering);
            bSupports_r2_5 = test_rendering();
            FreeLibrary(hRender);
        }

        // try to initialize R3
        Log("Loading DLL:", r3_name);
        // Hide "d3d10.dll not found" message box for XP
        SetErrorMode(SEM_FAILCRITICALERRORS);
        hRender = LoadLibrary(r3_name);
        // Restore error handling
        SetErrorMode(0);
        if (hRender)
        {
            SupportsDX10Rendering* test_dx10_rendering = (SupportsDX10Rendering*)GetProcAddress(hRender, "SupportsDX10Rendering");
            R_ASSERT(test_dx10_rendering);
            bSupports_r3 = test_dx10_rendering();
            FreeLibrary(hRender);
        }
#endif

        // try to initialize R4
        Log("Loading DLL:", r4_name);
        // Hide "d3d10.dll not found" message box for XP
        SetErrorMode(SEM_FAILCRITICALERRORS);
        hRender = LoadLibrary(r4_name);
        // Restore error handling
        SetErrorMode(0);
        if (hRender)
        {
            SupportsDX11Rendering* test_dx11_rendering = (SupportsDX11Rendering*)GetProcAddress(hRender, "SupportsDX11Rendering");
            R_ASSERT(test_dx11_rendering);
            bSupports_r4 = test_dx11_rendering();
            FreeLibrary(hRender);
        }
    }

    hRender = 0;
    vid_quality_token = xr_alloc<xr_token>(3);
    vid_quality_token[0].id = 0;
    vid_quality_token[0].name = xr_strdup("renderer_r1");
    vid_quality_token[1].id = 1;
    vid_quality_token[1].name = xr_strdup("renderer_r4");
    vid_quality_token[2].id = -1;
    vid_quality_token[2].name = nullptr;

//#ifdef DEBUG
    Msg("Available render modes[%d]:", 2);
//#endif // DEBUG
    for (u8 i = 0; i < 2; ++i)
    {
//#ifdef DEBUG
        Msg("[%s]", vid_quality_token[i].name);
//#endif // DEBUG
    }

    /*
    if(vid_quality_token != NULL) return;

    D3DCAPS9 caps;
    CHW _HW;
    _HW.CreateD3D ();
    _HW.pD3D->GetDeviceCaps (D3DADAPTER_DEFAULT,D3DDEVTYPE_HAL,&caps);
    _HW.DestroyD3D ();
    u16 ps_ver_major = u16 ( u32(u32(caps.PixelShaderVersion)&u32(0xf << 8ul))>>8 );

    xr_vector<LPCSTR> _tmp;
    u32 i = 0;
    for(; i<5; ++i)
    {
    bool bBreakLoop = false;
    switch (i)
    {
    case 3: //"renderer_r2.5"
    if (ps_ver_major < 3)
    bBreakLoop = true;
    break;
    case 4: //"renderer_r_dx10"
    bBreakLoop = true;
    break;
    default: ;
    }

    if (bBreakLoop) break;

    _tmp.push_back (NULL);
    LPCSTR val = NULL;
    switch (i)
    {
    case 0: val ="renderer_r1"; break;
    case 1: val ="renderer_r2a"; break;
    case 2: val ="renderer_r2"; break;
    case 3: val ="renderer_r2.5"; break;
    case 4: val ="renderer_r_dx10"; break; // -)
    }
    _tmp.back() = xr_strdup(val);
    }
    u32 _cnt = _tmp.size()+1;
    vid_quality_token = xr_alloc<xr_token>(_cnt);

    vid_quality_token[_cnt-1].id = -1;
    vid_quality_token[_cnt-1].name = NULL;

    #ifdef DEBUG
    Msg("Available render modes[%d]:",_tmp.size());
    #endif // DEBUG
    for(u32 i=0; i<_tmp.size();++i)
    {
    vid_quality_token[i].id = i;
    vid_quality_token[i].name = _tmp[i];
    #ifdef DEBUG
    Msg ("[%s]",_tmp[i]);
    #endif // DEBUG
    }
    */
}