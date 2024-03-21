//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// various definitions needed by most files
//////////////////////////////////////////////////////////////////////
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//////////////////////////////////////////////////////////////////////

#ifndef __OTSERV_DEFINITIONS_H__
#define __OTSERV_DEFINITIONS_H__

#include "exception.h"

//pugixml
#include "pugicast.h"
#include <pugixml.hpp>

#ifdef __DEBUG_EXCEPTION_REPORT__
	#define DEBUG_REPORT int *a = NULL; *a = 1;
#else
	#ifdef __EXCEPTION_TRACER__
		#define DEBUG_REPORT ExceptionHandler::dumpStack();
	#else
		#define DEBUG_REPORT
	#endif
#endif

#ifdef _WIN32
#ifndef WIN32
#define WIN32
#endif
#endif

enum passwordType_t
{
	PASSWORD_TYPE_PLAIN = 0,
	PASSWORD_TYPE_MD5,
	PASSWORD_TYPE_SHA1
};

#if defined _WIN32
#  ifndef WIN32
#    define WIN32
#  endif
#endif

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#if defined __WINDOWS__ || defined WIN32

#if defined _MSC_VER && defined NDEBUG
	#define _SECURE_SCL 0
	#define HAS_ITERATOR_DEBUGGING 0
#endif

#ifndef __FUNCTION__
	#define	__FUNCTION__ __func__
#endif

#define OTSYS_THREAD_RETURN  void
#ifndef WSAEWOULDBLOCK
#include <winsock2.h>
#endif
#ifndef EWOULDBLOCK
#define EWOULDBLOCK WSAEWOULDBLOCK
#endif

#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif

//Windows 2000	0x0500
//Windows XP	0x0501
//Windows 2003	0x0502
//Windows Vista	0x0600
#define _WIN32_WINNT 0x0601

#ifdef __GNUC__

	#include <assert.h>
	#define ATOI64 atoll
	
	#include <unordered_map>
	#include <unordered_set>
	#define OTSERV_HASH_MAP std::unordered_map
	#define OTSERV_HASH_SET std::unordered_set
	
#else

	typedef unsigned long long uint64_t;
	
	#define _WIN32_WINNT 0x0601

	#ifndef NOMINMAX
		#define NOMINMAX
	#endif

	#define _SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS
	#include <unordered_map>
	#include <unordered_set>
	#include <limits>
	#include <assert.h>
	#define OTSERV_HASH_MAP std::tr1::unordered_map
	#define OTSERV_HASH_SET std::tr1::unordered_set

	#include <cstring>
	inline int strcasecmp(const char *s1, const char *s2)
	{
		return ::_stricmp(s1, s2);
	}

	inline int strncasecmp(const char *s1, const char *s2, size_t n)
	{
		return ::_strnicmp(s1, s2, n);
	}

	/*typedef signed long long int64_t;
	typedef unsigned long uint32_t;
	typedef signed long int32_t;
	typedef unsigned short uint16_t;
	typedef signed short int16_t;
	typedef unsigned char uint8_t;*/
	
	#define ATOI64 _atoi64

	#pragma warning(disable:4786) // msvc too long debug names in stl
	#pragma warning(disable:4250) // 'class1' : inherits 'class2::member' via dominance
	#pragma warning(disable:4244)
	#pragma warning(disable:4267)
	#pragma warning(disable:4018)

#endif

//*nix systems
#else
	#define OTSYS_THREAD_RETURN void*

	#include <stdint.h>
	#include <string.h>
	#include <ext/hash_map>
	#include <ext/hash_set>
	#include <assert.h>
	#include <time.h>

	#define OTSERV_HASH_MAP __gnu_cxx::hash_map
	#define OTSERV_HASH_SET __gnu_cxx::hash_set
	
	#define ATOI64 atoll
	
#endif

#if defined(WIN32) && !defined(_MSC_VER)
	#include <stdint.h>
	#include <vector>
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#endif
