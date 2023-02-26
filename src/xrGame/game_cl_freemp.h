#pragma once
#include "game_cl_mp.h"

class CUIGameFMP;
class CVoiceChat;
class game_cl_freemp :public game_cl_mp
{
private:
	using inherited = game_cl_mp;
	CUIGameFMP *m_game_ui;

public:
			game_cl_freemp();
	virtual	~game_cl_freemp();


	virtual CUIGameCustom* createGameUI();
	virtual void SetGameUI(CUIGameCustom*);

	virtual void SetEnvironmentGameTimeFactor(ALife::_TIME_ID GameTime, const float fTimeFactor) override;

	virtual	void net_import_state(NET_Packet& P);
	virtual	void net_import_update(NET_Packet& P);
	virtual void shedule_Update(u32 dt);
	virtual void OnRender();

	virtual	bool OnKeyboardPress(int key);
	virtual	bool OnKeyboardRelease(int key);

	virtual void TranslateGameMessage(u32 msg, NET_Packet& P);
	virtual LPCSTR GetGameScore(string32&	score_dest);
	virtual bool Is_Rewarding_Allowed()  const { return false; };

	virtual void OnConnected();
	virtual void OnScreenResolutionChanged();

private:
	void OnVoiceMessage(NET_Packet* P);

private:
	CVoiceChat* m_pVoiceChat = nullptr;
};