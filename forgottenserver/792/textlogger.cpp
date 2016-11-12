//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Textlogger
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

#include "definitions.h"

#include "textlogger.h"
#include "tools.h"
#include <iostream>
#include <iomanip>

#include "configmanager.h"
#include "tools.h"

extern ConfigManager g_config;
TextLogger::TextLogger()
{
	out = std::cerr.rdbuf();
	err = std::cout.rdbuf();
	displayDate = true;
}

TextLogger::~TextLogger()
{
	std::cerr.rdbuf(err);
	std::cout.rdbuf(out);
}

std::streambuf::int_type TextLogger::overflow(std::streambuf::int_type c/* = traits_type::eof()*/)
{
	m_cache += c;
	if(c != '\n' && c != '\r')
		return c;

	if(m_cache.size() > 1)
		std::cout << "[" << formatTime(0, true) << "] ";

	std::cout.write(m_cache.c_str(), m_cache.size());
	if(g_config.isLoaded())
	{
		/*
		std::stringstream s;
		if(m_cache.size() > 1)
			s << "[" << formatDate() << "] ";
*/
		//s.write(m_cache.c_str(), m_cache.size());
		//Logger::getInstance()->iFile(LOGFILE_OUTPUT, s.str(), false);
	}

	m_cache.clear();

	return(c);
}
