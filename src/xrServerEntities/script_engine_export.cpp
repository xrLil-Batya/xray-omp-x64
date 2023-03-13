////////////////////////////////////////////////////////////////////////////
//	Module 		: script_engine_export.cpp
//	Created 	: 01.04.2004
//  Modified 	: 22.06.2004
//	Author		: Dmitriy Iassenev
//	Description : XRay Script Engine export
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#define SCRIPT_REGISTRATOR
#include "script_export_space.h"
#include "script_engine_export.h"
#undef SCRIPT_REGISTRATOR

#pragma optimize("s",on)
template <typename TList> struct Register
{
	ASSERT_TYPELIST(TList);

	static void _Register(lua_State *L)
	{
		Register<TList::Tail>::_Register(L);
#ifdef XRGAME_EXPORTS
#	ifdef _DEBUG
		Msg("Exporting %s",typeid(TList::Head).name());
#	endif
#endif
		TList::Head::script_register(L);
	}
};

template <> struct Register<Loki::NullType>
{
	static void _Register(lua_State *L)
	{
	}
};

void export_classes	(lua_State *L)
{
	Register<script_type_list>::_Register(L);
}
