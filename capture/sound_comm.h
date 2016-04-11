#ifndef _COMMON_INCLUDE_H__
#define _COMMON_INCLUDE_H__

// STL includes 
#include <string>
#include <map>
#include <list>
#include <functional>
#include <algorithm>
#include <cassert>

#define SOUND_AUTOLIB

typedef std::basic_string<unsigned char> UCHAR_STRING;

#include <atlcomcli.h>
// DirectSound includes
#include <mmsystem.h>
#include <mmreg.h>
#include <dsound.h>

// disable some warnning
#pragma warning(disable:4251)

// #define SOUND_AUTOLIB to automatically include the libs needed
#ifdef SOUND_AUTOLIB
//		#pragma comment( lib, "dxerr.lib" )
#pragma comment( lib, "dxguid.lib" )
#pragma comment( lib, "winmm.lib" )
#pragma comment( lib, "Dsound.lib" )
#pragma comment( lib, "comctl32.lib" )
#endif

#endif


