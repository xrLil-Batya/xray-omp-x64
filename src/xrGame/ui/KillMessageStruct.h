// File:		KillMessageStruct.h
// Description:	storage for HUD message about player death
// Created:		10.03.2005
// Mail:		narrator@gsc-game.kiev.ua
//
// Copyright 2005 GSC Game World

#pragma once

#include "ui_defs.h"

struct KilledPlayerInfo{
    shared_str	m_name;
	u32			m_color;
};

struct IconInfo{
	Frect		m_rect;
	ui_shader	m_shader;
};

struct KillMessageStruct{
	KilledPlayerInfo m_victim;
	IconInfo	m_initiator;
	KilledPlayerInfo m_killer;
	IconInfo	m_ext_info;
};