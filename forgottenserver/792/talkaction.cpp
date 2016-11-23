//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// 
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
#include "otpch.h"

#include <sstream>
#include <fstream>

#include "ban.h"
#include "creature.h"
#include "player.h"
#include "talkaction.h"
#include "tools.h"
#include "game.h"
#include "configmanager.h"

extern ConfigManager g_config;
extern Game g_game;

TalkActions::TalkActions() :
m_scriptInterface("TalkAction Interface")
{
	m_scriptInterface.initState();
}

TalkActions::~TalkActions()
{
	clear();
}

void TalkActions::clear()
{
	TalkActionList::iterator it = wordsMap.begin();
	while(it != wordsMap.end())
	{
		delete it->second;
		wordsMap.erase(it);
		it = wordsMap.begin();
	}
	
	m_scriptInterface.reInitState();
}

LuaScriptInterface& TalkActions::getScriptInterface()
{
	return m_scriptInterface;	
}

std::string TalkActions::getScriptBaseName()
{
	return "talkactions";	
}

Event* TalkActions::getEvent(const std::string& nodeName)
{
	if(nodeName == "talkaction")
		return new TalkAction(&m_scriptInterface);

	return NULL;
}

bool TalkActions::registerEvent(Event* event, const pugi::xml_node& node)
{
	TalkAction* talkAction = dynamic_cast<TalkAction*>(event);
	if(!talkAction)
		return false;
	
	wordsMap.push_back(std::make_pair(talkAction->getWords(), talkAction));
	return true;
}

TalkActionResult_t TalkActions::onPlayerSpeak(Player* player, SpeakClasses type, const std::string& words)
{
	if(type != SPEAK_SAY)
		return TALKACTION_CONTINUE;

	std::string str_words_quote;
	std::string str_param_quote;
	std::string str_words_first_word;
	std::string str_param_first_word;
	
	// With quotation filtering
	size_t loc = words.find( '"', 0 );
	if(loc != std::string::npos && loc >= 0)
	{
		str_words_quote = std::string(words, 0, loc);
		str_param_quote = std::string(words, (loc+1), words.size()-loc-1);
	}
	else
	{
		str_words_quote = words;
		str_param_quote = std::string(""); 
	}
	
	trim_left(str_words_quote, " ");
	trim_right(str_param_quote, " ");
	
	// With whitespace filtering
	loc = words.find( ' ', 0 );
	if(loc != std::string::npos && loc >= 0)
	{
		str_words_first_word = std::string(words, 0, loc);
		str_param_first_word = std::string(words, (loc+1), words.size()-loc-1);
	}
	else
	{
		str_words_first_word = words;
		str_param_first_word = std::string(""); 
	}

	TalkActionList::iterator it;
	for(it = wordsMap.begin(); it != wordsMap.end(); ++it)
	{
		std::string cmdstring;
		std::string paramstring;
		if(it->second->getFilterType() == TALKACTION_MATCH_QUOTATION)
		{
			cmdstring = str_words_quote;
			paramstring = str_param_quote;
		}
		else if(it->second->getFilterType() == TALKACTION_MATCH_FIRST_WORD)
		{
			cmdstring = str_words_first_word;
			paramstring = str_param_first_word;
		}
		else
			continue;

		if(cmdstring == it->first || !it->second->isCaseSensitive() && strcasecmp(it->first.c_str(), cmdstring.c_str()) == 0)
		{
			bool ret = true;
			TalkAction* talkActionRef = it->second;
			if(player->getAccessLevel() < it->second->getAccessLevel())
			{
                if(player->getAccessLevel() > 0)
				{
                    player->sendCancel("You are not able to execute this action.");
                    g_game.addMagicEffect(player->getPosition(), NM_ME_POFF);
					ret = false;
                }
            }
			else
			{
				TalkAction* talkAction = it->second;

				if(talkAction->isScripted())
					ret = talkAction->executeSay(player, cmdstring, paramstring);
				else
				{
					TalkActionFunction* func = talkAction->getFunction();
					if(func)
					{
						func(player, cmdstring, paramstring);
						ret = false;
					}
				}
			}

            if(talkActionRef->getLog() && player)
			{
                std::string filename = "data/logs/" + player->getName() + ".txt";
                std::ofstream talkaction(filename.c_str(), std::ios_base::app);
                time_t timeNow = time(NULL);
                const tm* now = localtime(&timeNow);
                char buffer[32];
                strftime(buffer, sizeof(buffer), "%d/%m/%Y  %H:%M", now);
                talkaction << "["<<buffer<<"] TalkAction: " << words << std::endl;
                talkaction.close();
            }

			if(ret)
				return TALKACTION_CONTINUE;
			else
				return TALKACTION_BREAK;
		}
	}
	return TALKACTION_CONTINUE;
}


TalkAction::TalkAction(LuaScriptInterface* _interface) :
	Event(_interface),
	filterType(TALKACTION_MATCH_QUOTATION),
	registerlog(false),
	casesensitive(false),
	accessLevel(0),
	function(NULL)
{
	//
}

TalkAction::~TalkAction()
{
	//
}

bool TalkAction::configureEvent(const pugi::xml_node& node)
{
	pugi::xml_attribute attr;
	if (!(attr = node.attribute("words")))
	{
		std::cout << "[Error - TalkAction::configureEvent] No words for talk action or spell" << std::endl;
		return false;
	}
	
	commandString = attr.as_string();

	if ((attr = node.attribute("filter")))
	{
		std::string tmpStrValue = asLowerCaseString(attr.as_string());
		if (tmpStrValue == "quotation")
			filterType = TALKACTION_MATCH_QUOTATION;
		else if (tmpStrValue == "first word")
			filterType = TALKACTION_MATCH_FIRST_WORD;
	}
	
	if ((attr = node.attribute("registerlog")) || (attr = node.attribute("log")))
		registerlog = attr.as_string();

	if ((attr = node.attribute("case-sensitive")) || (attr = node.attribute("sensitive")))
		casesensitive = attr.as_string();
		
	if ((attr = node.attribute("access")))
		accessLevel = pugi::cast<int32_t>(attr.value());
	
	return true;	
}

bool TalkAction::loadFunction(const std::string& functionName)
{
	if(asLowerCaseString(functionName) == "banplayer")
		function = banPlayer;
	else
		return false;

	m_scripted = false;
	return true;
}

std::string TalkAction::getScriptEventName()
{
	return "onSay";
}

uint32_t TalkAction::executeSay(Creature* creature, const std::string& words, const std::string& param)
{
	//onSay(cid, words, param)
	if(m_scriptInterface->reserveScriptEnv())
	{
		ScriptEnvironment* env = m_scriptInterface->getScriptEnv();
	
		#ifdef __DEBUG_LUASCRIPTS__
		std::stringstream desc;
		desc << creature->getName() << " - " << words << " " << param;
		env->setEventDesc(desc.str());
		#endif
	
		env->setScriptId(m_scriptId, m_scriptInterface);
		env->setRealPos(creature->getPosition());
	
		uint32_t cid = env->addThing(creature);
	
		lua_State* L = m_scriptInterface->getLuaState();
	
		m_scriptInterface->pushFunction(m_scriptId);
		lua_pushnumber(L, cid);
		lua_pushstring(L, words.c_str());
		lua_pushstring(L, param.c_str());
	
		bool result = m_scriptInterface->callFunction(3) != 0;
		m_scriptInterface->releaseScriptEnv();
		return result;
	}
	else
	{
		std::cout << "[Error] Call stack overflow. TalkAction::executeSay" << std::endl;
		return 0;
	}
}

bool TalkAction::banPlayer(Player* player, const std::string& words, const std::string& param)
{
	Player* playerBan = g_game.getPlayerByName(param);
	if(playerBan)
	{
		if(playerBan->hasFlag(PlayerFlag_CannotBeBanned))
		{
			player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "You cannot ban this player.");
			return true;
		}

		playerBan->sendTextMessage(MSG_STATUS_CONSOLE_RED, "You have been banned.");
		uint32_t ip = playerBan->lastIP;
		if(ip > 0)
			IOBan::getInstance()->addIpBan(ip, (time(NULL) + 86400), 0);

		playerBan->kickPlayer(true);
		return true;
	}
	return false;
}
