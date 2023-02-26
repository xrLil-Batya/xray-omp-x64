#include "stdafx.h"

#include "../Include/xrRender/DrawUtils.h"
#include "render.h"
#include "IGame_Persistent.h"
#include "xr_IOConsole.h"

extern ENGINE_API float psSVPImageSizeK;

void CRenderDevice::_Destroy(BOOL bKeepTextures)
{
    DU->OnDeviceDestroy();

    // before destroy
    b_is_Ready = FALSE;
    Statistic->OnDeviceDestroy();
    ::Render->destroy();
    m_pRender->OnDeviceDestroy(bKeepTextures);
    //Resources->OnDeviceDestroy (bKeepTextures);
    //RCache.OnDeviceDestroy ();

    Memory.mem_compact();
}

void CRenderDevice::Destroy(void)
{
    if (!b_is_Ready) return;

    Log("Destroying Direct3D...");

    ShowCursor(TRUE);
	//ClipCursor(NULL);
    m_pRender->ValidateHW();

    _Destroy(FALSE);

    // real destroy
    m_pRender->DestroyHW();

    //xr_delete (Resources);
    //HW.DestroyDevice ();

    seqRender.R.clear();
    seqAppActivate.R.clear();
    seqAppDeactivate.R.clear();
    seqAppStart.R.clear();
    seqAppEnd.R.clear();
    seqFrame.R.clear();
    seqFrameMT.R.clear();
    seqDeviceReset.R.clear();
    seqParallel.clear();

    RenderFactory->DestroyRenderDeviceRender(m_pRender);
    m_pRender = 0;
    xr_delete(Statistic);
}

#include "IGame_Level.h"
#include "CustomHUD.h"
extern BOOL bNeed_re_create_env;
void CRenderDevice::Reset(bool precache)
{
    u32 dwWidth_before = dwWidth;
    u32 dwHeight_before = dwHeight;

    ShowCursor(TRUE);
    u32 tm_start = TimerAsync();
    if (g_pGamePersistent)
    {

        //. g_pGamePersistent->Environment().OnDeviceDestroy();
    }

    m_pRender->Reset(m_hWnd, dwWidth, dwHeight, fWidth_2, fHeight_2);

    if (psDeviceFlags.test(rsR3) || psDeviceFlags.test(rsR4))
    {
        m_SecondViewport.screenWidth = u32((dwWidth / 32) * psSVPImageSizeK) * 32;
        m_SecondViewport.screenHeight = u32((dwHeight / 32) * psSVPImageSizeK) * 32;
    }
    else
    {
        m_SecondViewport.screenWidth = dwWidth;
        m_SecondViewport.screenHeight = dwHeight;
    }

    if (g_pGamePersistent)
    {
        //. g_pGamePersistent->Environment().OnDeviceCreate();
        //bNeed_re_create_env = TRUE;
        g_pGamePersistent->Environment().bNeed_re_create_env = TRUE;
    }
    _SetupStates();
    if (precache)
        PreCache(20, true, false);
    u32 tm_end = TimerAsync();
    Msg("*** RESET [%d ms]", tm_end - tm_start);

    // TODO: Remove this! It may hide crash
    Memory.mem_compact();

	if(!g_dedicated_server)
	{
		ShowCursor(false);
		//RECT winRect;
		//GetWindowRect(m_hWnd, &winRect);
		//ClipCursor(&winRect);
	}

    seqDeviceReset.Process(rp_DeviceReset);

    if (dwWidth_before != dwWidth || dwHeight_before != dwHeight)
    {
        seqResolutionChanged.Process(rp_ScreenResolutionChanged);
    }
}
