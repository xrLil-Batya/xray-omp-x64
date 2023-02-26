#pragma once

#include "UIDialogWnd.h"
#include "../encyclopedia_article_defs.h"

class CInventoryOwner;
class CUIFrameLineWnd;
class CUI3tButton;
class CUITabControl;
class CUIStatic;
class CUITextWnd;
class CUIXml;
class CUIFrameWindow;
class UIHint;

class CUITaskWnd;
//-class CUIFactionWarWnd;
class CUIRankingWnd;
class CUILogsWnd;
class CUIAnimatedStatic;
class UIHint;
class CUIProgressBar;
class CMapSpot;

class CUIPdaWnd: public CUIDialogWnd
{
	typedef CUIDialogWnd	inherited;
protected:
	CUITabControl*			UITabControl;
	CUI3tButton*			m_btn_close;

	CUIStatic*				UIMainPdaFrame;
	CUIStatic*				UINoice;
	
	CUITextWnd*				m_caption;
	shared_str				m_caption_const;
//	CUIAnimatedStatic*		m_anim_static;
	CUITextWnd*				m_clock;

	// Текущий активный диалог
	CUIWindow*				m_pActiveDialog;
	shared_str				m_sActiveSection;

	UIHint*					m_hint_wnd;
	bool bButtonL, bButtonR;
	u32 dwPDAFrame;
public:
	CUITaskWnd*				pUITaskWnd;
//-	CUIFactionWarWnd*		pUIFactionWarWnd;
	CUIRankingWnd*			pUIRankingWnd;
	CUILogsWnd*				pUILogsWnd;

	virtual void			Reset				();

public:
							CUIPdaWnd			();
	virtual					~CUIPdaWnd			();

	virtual void 			Init				();

	virtual void 			SendMessage			(CUIWindow* pWnd, s16 msg, void* pData = NULL);
	Frect m_cursor_box;

	virtual void 			Draw				();
	virtual void 			Update				();
	virtual void 			Show				(bool status);
	virtual bool OnMouseAction(float x, float y, EUIMessages mouse_action);
	void MouseMovement(float x, float y);
	virtual void Enable(bool status);
	virtual bool			OnKeyboardAction			(int dik, EUIMessages keyboard_action);
		
			UIHint*			get_hint_wnd		() const { return m_hint_wnd; }
			void			DrawHint			();

			void			SetActiveCaption	();
			void			SetCaption			(LPCSTR text);
			void			Show_SecondTaskWnd	(bool status);
			void			Show_MapLegendWnd	(bool status);

			void			SetActiveSubdialog	(const shared_str& section);
	virtual bool			StopAnyMove			(){return false;}

			void			UpdatePda			();
			void			UpdateRankingWnd	();
	void ResetCursor();
	Fvector2 last_cursor_pos;

	Fvector target_joystickrot, joystickrot;
	float target_buttonpress, buttonpress;

	void ResetJoystick(bool bForce)
	{
		if (bForce)
		{
			joystickrot.set(0.f, 0.f, 0.f);
			buttonpress = 0.f;
		}
		target_joystickrot.set(0.f, 0.f, 0.f);
		target_buttonpress = 0.f;
	}
};
