////////////////////////////////////////////////////////////////////////////
//	Module 		: smart_cast.h
//	Created 	: 17.09.2004
//  Modified 	: 17.09.2004
//	Author		: Dmitriy Iassenev
//	Description : Smart dynamic cast
////////////////////////////////////////////////////////////////////////////
#ifdef DEBUG
#	define PURE_DYNAMIC_CAST
#endif // DEBUG

#define  TL_FAST_COMPILATION
#undef   STATIC_CHECK
#include <loki/typelist.h>
#undef   STATIC_CHECK
#define STATIC_CHECK(expr, msg) static_assert(expr, #msg)
#include <imdexlib/fast_dynamic_cast.hpp>
#define smart_cast imdex::fast_dynamic_cast
