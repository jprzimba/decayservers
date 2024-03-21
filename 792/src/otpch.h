#ifdef __OTSERV_OTCP_H__
#error "Precompiled header should only be included once."
#endif

#define __OTSERV_OTCP_H__

//#undef __USE_OTPCH__

// Definitions should be global.
#include "definitions.h"

#ifdef __USE_OTPCH__

#if defined __WINDOWS__ || defined WIN32
#include <winerror.h>
#endif

//pugixml
#include <pugixml.hpp>

//boost
#include <boost/config.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/tokenizer.hpp>
#include <boost/regex.hpp>
#include <boost/asio.hpp>
//std
#include <list>
#include <vector>
#include <map>
#include <string>
//otserv
#include "thing.h"

#endif
