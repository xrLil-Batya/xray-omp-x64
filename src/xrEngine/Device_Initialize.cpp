#include "stdafx.h"
#include "resource.h"

#ifdef INGAME_EDITOR
# include "../include/editor/ide.hpp"
# include "engine_impl.hpp"
#endif // #ifdef INGAME_EDITOR

extern LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
bool extern ENGINE_API g_dedicated_server;

#ifdef INGAME_EDITOR
void CRenderDevice::initialize_editor()
{
    m_editor_module = LoadLibrary("editor.dll");
    if (!m_editor_module)
    {
        Msg("! cannot load library \"editor.dll\"");
        return;
    }

    m_editor_initialize = (initialize_function_ptr)GetProcAddress(m_editor_module, "initialize");
    VERIFY(m_editor_initialize);

    m_editor_finalize = (finalize_function_ptr)GetProcAddress(m_editor_module, "finalize");
    VERIFY(m_editor_finalize);

    m_engine = xr_new<engine_impl>();
    m_editor_initialize(m_editor, m_engine);
    VERIFY(m_editor);

    m_hWnd = m_editor->view_handle();
    VERIFY(m_hWnd != INVALID_HANDLE_VALUE);
}
#endif // #ifdef INGAME_EDITOR

void CRenderDevice::Initialize()
{
    Log("Initializing Engine...");
    TimerGlobal.Start();
    TimerMM.Start();

    // Unless a substitute hWnd has been specified, create a window to render into
    if (m_hWnd == NULL)
    {
        const char* wndclass = "_XRAY_1.5";

        // Register the windows class
        HINSTANCE hInstance = (HINSTANCE)GetModuleHandle(0);
        WNDCLASS wndClass = { 0, WndProc, 0, 0, hInstance,
                             LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1)),
                             LoadCursor(NULL, IDC_ARROW),
                             (HBRUSH)GetStockObject(BLACK_BRUSH),
                             NULL, wndclass
        };
        RegisterClass(&wndClass);


        // Set the window's initial style
        m_dwWindowStyle = WS_BORDER | WS_DLGFRAME;

        // Set the window's initial width
        u32 screen_width = GetSystemMetrics(SM_CXSCREEN);
        u32 screen_height = GetSystemMetrics(SM_CYSCREEN);

        DEVMODE screen_settings;
        memset(&screen_settings, 0, sizeof(screen_settings));
        screen_settings.dmSize = sizeof(screen_settings);
        screen_settings.dmPelsWidth = (unsigned long)screen_width;
        screen_settings.dmPelsHeight = (unsigned long)screen_height;
        screen_settings.dmBitsPerPel = 32;
        screen_settings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

        ChangeDisplaySettings(&screen_settings, CDS_FULLSCREEN);

        // Create the render window
		if(g_dedicated_server)
		{
			m_hWnd = CreateWindow(wndclass, "S.T.A.L.K.E.R. | X-Ray OMP x64", m_dwWindowStyle,
				/*rc.left, rc.top, */0, 0,
				640, 480, 0L,
				0, hInstance, 0L);
		}
		else
		{
			m_hWnd = CreateWindow(wndclass, "S.T.A.L.K.E.R. | X-Ray OMP x64", m_dwWindowStyle,
				/*rc.left, rc.top, */0, 0,
				screen_width, screen_height, 0L,
				0, hInstance, 0L);
		}
    }

    // Save window properties
    m_dwWindowStyle = GetWindowLongPtr(m_hWnd, GWL_STYLE);
    GetWindowRect(m_hWnd, &m_rcWindowBounds);
    GetClientRect(m_hWnd, &m_rcWindowClient);

    /*
    if (strstr(lpCmdLine,"-gpu_sw")!=NULL) HW.Caps.bForceGPU_SW = TRUE;
    else HW.Caps.bForceGPU_SW = FALSE;
    if (strstr(lpCmdLine,"-gpu_nopure")!=NULL) HW.Caps.bForceGPU_NonPure = TRUE;
    else HW.Caps.bForceGPU_NonPure = FALSE;
    if (strstr(lpCmdLine,"-gpu_ref")!=NULL) HW.Caps.bForceGPU_REF = TRUE;
    else HW.Caps.bForceGPU_REF = FALSE;
    */
}

