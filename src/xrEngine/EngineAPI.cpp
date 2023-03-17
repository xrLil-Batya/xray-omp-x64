// EngineAPI.cpp: implementation of the CEngineAPI class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "EngineAPI.h"
#include "../xrcdb/xrXRC.h"
#include "XR_IOConsole.h"

extern xr_vector<xr_token> vid_quality_token;

constexpr const char* r1_name = "xrRender_R1";
constexpr const char* r2_name = "xrRender_R2";
constexpr const char* r4_name = "xrRender_R4";

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
	vid_quality_token.clear();
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
	// If we failed to load render,
	// then try to fallback to lower one.
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
            psDeviceFlags.set(rsR3, true);
        }
		else
			g_current_renderer = 4;
    }

    if (psDeviceFlags.test(rsR2))
    {
        // try to initialize R2
        psDeviceFlags.set(rsR4, false);
        psDeviceFlags.set(rsR3, false);
        Log("Loading DLL:", r2_name);
        hRender = LoadLibrary(r2_name);
        if (!hRender)
        {
            // try to load R1
            Msg("! ...Failed - incompatible hardware.");
			psDeviceFlags.set(rsR1, true);
        }
        else
            g_current_renderer = 2;
    }

	if (psDeviceFlags.test(rsR1))
	{
		// try to load R1
		renderer_value = 0; //con cmd

		Log("Loading DLL:", r1_name);
		hRender = LoadLibrary(r1_name);
		if (0 == hRender)
		{
			// try to load R1
			Msg("! ...Failed - incompatible hardware.");
		}
		else
			g_current_renderer = 1;
	}
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
		if(g_dedicated_server)
		{
			Console->Execute("renderer renderer_r1");
			
			// try to load R1
			psDeviceFlags.set(rsR4, false);
			psDeviceFlags.set(rsR3, false);
			psDeviceFlags.set(rsR2, false);
			psDeviceFlags.set(rsR1, true);
			renderer_value = 0; //con cmd

			Log("Loading DLL:", r1_name);
			hRender = LoadLibrary(r1_name);
			if (!hRender) R_CHK(GetLastError());
			R_ASSERT(hRender);
			g_current_renderer = 1;
		}
		else
		{
			// if engine failed to load renderer
			// but there is at least one available
			// then try again
			string32 buf;
			xr_sprintf(buf, "renderer %s", vid_quality_token[0].name);
			Console->Execute(buf);

			// Second attempt
			InitializeNotDedicated();
		}
    }
	if (0 == hRender)
		R_CHK(GetLastError());

	R_ASSERT2(hRender, "Can't load renderer");
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
    typedef bool __cdecl SupportsAdvancedRendering(void);
    typedef bool _declspec(dllexport) SupportsDX10Rendering();
    typedef bool _declspec(dllexport) SupportsDX11Rendering();
};

void CEngineAPI::CreateRendererList()
{
	if (!vid_quality_token.empty())
		return;

	xr_vector<xr_token> modes;

	// try to initialize R1
	Log("Loading DLL:", r1_name);
	hRender = LoadLibrary(r1_name);
	if (hRender)
	{
		modes.emplace_back(xr_token("renderer_r1", 0));
		FreeLibrary(hRender);
	}

	// try to initialize R2
	Log("Loading DLL:", r2_name);
	hRender = LoadLibrary(r2_name);
	if (hRender)
	{
		modes.push_back(xr_token("renderer_r2a", 1));
		modes.emplace_back(xr_token("renderer_r2", 2));
		SupportsAdvancedRendering *test_rendering = (SupportsAdvancedRendering*)GetProcAddress(hRender, "SupportsAdvancedRendering");
		if (test_rendering && test_rendering())
			modes.emplace_back(xr_token("renderer_r2.5", 3));
		FreeLibrary(hRender);
	}

	// try to initialize R4
	Log("Loading DLL:", r4_name);
	//	Hide "d3d10 not found" message box for XP
	SetErrorMode(SEM_FAILCRITICALERRORS);
	hRender = LoadLibrary(r4_name);
	//	Restore error handling
	SetErrorMode(0);
	if (hRender)
	{
		SupportsDX11Rendering *test_dx11_rendering = (SupportsDX11Rendering*)GetProcAddress(hRender, "SupportsDX11Rendering");
		if (test_dx11_rendering && test_dx11_rendering())
			modes.emplace_back(xr_token("renderer_r4", 5));
		FreeLibrary(hRender);
	}

	modes.emplace_back(xr_token(nullptr, -1));

	hRender = nullptr;

	Msg("Available render modes[%d]:", modes.size());
	for (auto& mode : modes)
		if (mode.name)
			Log(mode.name);

#ifdef DEBUG
	Msg("Available render modes[%d]:",_tmp.size());
#endif // DEBUG

	vid_quality_token = std::move(modes);
}