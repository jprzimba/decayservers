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

#include "creatureevent.h"
#include "tools.h"
#include "player.h"

CreatureEvents::CreatureEvents() :
m_scriptInterface("CreatureScript Interface")
{
	m_scriptInterface.initState();
}

CreatureEvents::~CreatureEvents()
{
	CreatureEventList::iterator it;
	for(it = m_creatureEvents.begin(); it != m_creatureEvents.end(); ++it)
		delete it->second;
}

void CreatureEvents::clear()
{
	//clear creature events
	for(CreatureEventList::iterator it = m_creatureEvents.begin(); it != m_creatureEvents.end(); ++it)
		it->second->clearEvent();
	//clear lua state
	m_scriptInterface.reInitState();
}

LuaScriptInterface& CreatureEvents::getScriptInterface()
{
	return m_scriptInterface;
}

std::string CreatureEvents::getScriptBaseName()
{
	return "creaturescripts";
}

Event* CreatureEvents::getEvent(const std::string& nodeName)
{
	std::string tmpNodeName = asLowerCaseString(nodeName);
	if(tmpNodeName == "event" || tmpNodeName == "creaturevent" || tmpNodeName == "creatureevent" || tmpNodeName == "creaturescript")
		return new CreatureEvent(&m_scriptInterface);

	return NULL;
}

bool CreatureEvents::registerEvent(Event* event, xmlNodePtr p)
{
	CreatureEvent* creatureEvent = dynamic_cast<CreatureEvent*>(event);
	if(!creatureEvent)
		return false;

	if(creatureEvent->getEventType() == CREATURE_EVENT_NONE)
	{
		std::clog << "Error: [CreatureEvents::registerEvent] Trying to register event without type!." << std::endl;
		return false;
	}

	CreatureEvent* oldEvent = getEventByName(creatureEvent->getName(), false);
	if(oldEvent)
	{
		//if there was an event with the same that is not loaded
		//(happens when realoading), it is reused
		if(oldEvent->isLoaded() == false && oldEvent->getEventType() == creatureEvent->getEventType())
			oldEvent->copyEvent(creatureEvent);
		return false;
	}
	else
	{
		//if not, register it normally
		m_creatureEvents[creatureEvent->getName()] = creatureEvent;
		return true;
	}
}

CreatureEvent* CreatureEvents::getEventByName(const std::string& name, bool forceLoaded /*= true*/)
{
	CreatureEventList::iterator it = m_creatureEvents.find(name);
	if(it != m_creatureEvents.end())
	{
		if(!forceLoaded || it->second->isLoaded())
			return it->second;
	}
	return NULL;
}

uint32_t CreatureEvents::playerLogin(Player* player)
{
	//fire global event if is registered
	for(CreatureEventList::iterator it = m_creatureEvents.begin(); it != m_creatureEvents.end(); ++it)
	{
		if(it->second->getEventType() == CREATURE_EVENT_LOGIN)
		{
			if(!it->second->executeOnLogin(player))
				return 0;
		}
	}
	return 1;
}

uint32_t CreatureEvents::playerLogout(Player* player)
{
	//fire global event if is registered
	for(CreatureEventList::iterator it = m_creatureEvents.begin(); it != m_creatureEvents.end(); ++it)
	{
		if(it->second->getEventType() == CREATURE_EVENT_LOGOUT)
		{
			if(!it->second->executeOnLogout(player))
				return 0;
		}
	}
	return 1;
}

/////////////////////////////////////

CreatureEvent::CreatureEvent(LuaScriptInterface* _interface) :
Event(_interface)
{
	m_type = CREATURE_EVENT_NONE;
	m_loaded = false;
}

bool CreatureEvent::configureEvent(xmlNodePtr p)
{
	std::string str;
	//Name that will be used in monster xml files and
	// lua function to register events to reference this event
	if(readXMLString(p, "name", str))
		m_eventName = str;
	else
	{
		std::clog << "Error: [CreatureEvent::configureEvent] No name for creature event." << std::endl;
		return false;
	}
	
	if(readXMLString(p, "type", str))
	{
		std::string tmpStr = asLowerCaseString(str);
		if(tmpStr == "login")
			m_type = CREATURE_EVENT_LOGIN;
		else if(tmpStr == "logout")
			m_type = CREATURE_EVENT_LOGOUT;
		else if(tmpStr == "think")
			m_type = CREATURE_EVENT_THINK;
		else if(tmpStr == "preparedeath")
			m_type = CREATURE_EVENT_PREPAREDEATH;
		else if(tmpStr == "death")
			m_type = CREATURE_EVENT_DEATH;
		else if(tmpStr == "kill")
			m_type = CREATURE_EVENT_KILL;
		else if(tmpStr == "advance")
			m_type = CREATURE_EVENT_ADVANCE;
		else if(tmpStr == "look")
			m_type = CREATURE_EVENT_LOOK;
		else
		{
			std::clog << "Error: [CreatureEvent::configureEvent] No valid type for creature event." << str << std::endl;
			return false;
		}
	}
	else
	{
		std::clog << "Error: [CreatureEvent::configureEvent] No type for creature event."  << std::endl;
		return false;
	}
	m_loaded = true;
	return true;
}

std::string CreatureEvent::getScriptEventName()
{
	//Depending on the type script event name is different
	switch(m_type)
	{
		case CREATURE_EVENT_LOGIN:
			return "onLogin";
		case CREATURE_EVENT_LOGOUT:
			return "onLogout";
		case CREATURE_EVENT_THINK:
			return "onThink";
		case CREATURE_EVENT_PREPAREDEATH:
			return "onPrepareDeath";
		case CREATURE_EVENT_DEATH:
			return "onDeath";
		case CREATURE_EVENT_KILL:
			return "onKill";
		case CREATURE_EVENT_ADVANCE:
			return "onAdvance";
		case CREATURE_EVENT_LOOK:
			return "onLook";
		case CREATURE_EVENT_NONE:
		default:
			break;
	}

	return "";
}

void CreatureEvent::copyEvent(CreatureEvent* creatureEvent)
{
	m_scriptId = creatureEvent->m_scriptId;
	m_scriptInterface = creatureEvent->m_scriptInterface;
	m_scripted = creatureEvent->m_scripted;
	m_loaded = creatureEvent->m_loaded;
}

void CreatureEvent::clearEvent()
{
	m_scriptId = 0;
	m_scriptInterface = NULL;
	m_scripted = false;
	m_loaded = false;
}

uint32_t CreatureEvent::executeOnLogin(Player* player)
{
	//onLogin(cid)
	if(m_scriptInterface->reserveScriptEnv())
	{
		ScriptEnvironment* env = m_scriptInterface->getScriptEnv();

		env->setScriptId(m_scriptId, m_scriptInterface);
		env->setRealPos(player->getPosition());

		uint32_t cid = env->addThing(player);

		lua_State* L = m_scriptInterface->getLuaState();

		m_scriptInterface->pushFunction(m_scriptId);
		lua_pushnumber(L, cid);

		bool result = m_scriptInterface->callFunction(1);
		m_scriptInterface->releaseScriptEnv();
		return result;
	}
	else
	{
		std::clog << "[Error] Call stack overflow. CreatureEvent::executeOnLogin" << std::endl;
		return 0;
	}
}

uint32_t CreatureEvent::executeOnLogout(Player* player)
{
	//onLogout(cid)
	if(m_scriptInterface->reserveScriptEnv())
	{
		ScriptEnvironment* env = m_scriptInterface->getScriptEnv();

		env->setScriptId(m_scriptId, m_scriptInterface);
		env->setRealPos(player->getPosition());

		uint32_t cid = env->addThing(player);

		lua_State* L = m_scriptInterface->getLuaState();

		m_scriptInterface->pushFunction(m_scriptId);
		lua_pushnumber(L, cid);

		bool result = m_scriptInterface->callFunction(1);
		m_scriptInterface->releaseScriptEnv();
		return result;
	}
	else
	{
		std::clog << "[Error] Call stack overflow. CreatureEvent::executeOnLogout" << std::endl;
		return 0;
	}
}

uint32_t CreatureEvent::executeOnThink(Creature* creature, uint32_t interval)
{
	//onThink(cid, interval)
	if(m_scriptInterface->reserveScriptEnv())
	{
		ScriptEnvironment* env = m_scriptInterface->getScriptEnv();

		env->setScriptId(m_scriptId, m_scriptInterface);
		env->setRealPos(creature->getPosition());

		uint32_t cid = env->addThing(creature);

		lua_State* L = m_scriptInterface->getLuaState();

		m_scriptInterface->pushFunction(m_scriptId);
		lua_pushnumber(L, cid);
		lua_pushnumber(L, interval);

		bool result = m_scriptInterface->callFunction(2);
		m_scriptInterface->releaseScriptEnv();
		return result;
	}
	else
	{
		std::clog << "[Error] Call stack overflow. CreatureEvent::executeOnThink" << std::endl;
		return 0;
	}
}

uint32_t CreatureEvent::executeOnPrepareDeath(Player* player, Creature* killer)
{
	//onPrepareDeath(cid, killer)
	if(m_scriptInterface->reserveScriptEnv())
	{
		ScriptEnvironment* env = m_scriptInterface->getScriptEnv();

		env->setScriptId(m_scriptId, m_scriptInterface);
		env->setRealPos(player->getPosition());

		uint32_t cid = env->addThing(player);
		uint32_t killercid = env->addThing(killer);

		lua_State* L = m_scriptInterface->getLuaState();

		m_scriptInterface->pushFunction(m_scriptId);
		lua_pushnumber(L, cid);
		lua_pushnumber(L, killercid);

		bool result = m_scriptInterface->callFunction(2);
		m_scriptInterface->releaseScriptEnv();
		return result;
	}
	else
	{
		std::clog << "[Error] Call stack overflow. CreatureEvent::executeOnPrepareDeath" << std::endl;
		return 0;
	}
}

uint32_t CreatureEvent::executeOnDeath(Creature* creature, Item* corpse, Creature* killer)
{
	//onDeath(cid, corpse, killer)
	if(m_scriptInterface->reserveScriptEnv())
	{
		ScriptEnvironment* env = m_scriptInterface->getScriptEnv();

		env->setScriptId(m_scriptId, m_scriptInterface);
		env->setRealPos(creature->getPosition());

		uint32_t cid = env->addThing(creature);
		uint32_t corpseid = env->addThing(corpse);
		uint32_t killercid = env->addThing(killer);

		lua_State* L = m_scriptInterface->getLuaState();

		m_scriptInterface->pushFunction(m_scriptId);
		lua_pushnumber(L, cid);
		lua_pushnumber(L, corpseid);
		lua_pushnumber(L, killercid);

		bool result = m_scriptInterface->callFunction(3);
		m_scriptInterface->releaseScriptEnv();
		return result;
	}
	else
	{
		std::clog << "[Error] Call stack overflow. CreatureEvent::executeOnDeath" << std::endl;
		return 0;
	}
}

uint32_t CreatureEvent::executeOnKill(Creature* creature, Creature* target)
{
	//onKill(cid, target)
	if(m_scriptInterface->reserveScriptEnv())
	{
		ScriptEnvironment* env = m_scriptInterface->getScriptEnv();

		env->setScriptId(m_scriptId, m_scriptInterface);
		env->setRealPos(creature->getPosition());

		uint32_t cid = env->addThing(creature);
		uint32_t targetId = env->addThing(target);

		lua_State* L = m_scriptInterface->getLuaState();

		m_scriptInterface->pushFunction(m_scriptId);
		lua_pushnumber(L, cid);
		lua_pushnumber(L, targetId);

		bool result = m_scriptInterface->callFunction(2) != 0;
		m_scriptInterface->releaseScriptEnv();

		return result;
	}
	else
	{
		std::clog << "[Error] Call stack overflow. CreatureEvent::executeOnKill" << std::endl;
		return 0;
	}
}

uint32_t CreatureEvent::executeOnAdvance(Player* player, skills_t skill, uint32_t oldLevel, uint32_t newLevel)
{
	//onAdvance(cid, type, oldlevel, newlevel)
	if(m_scriptInterface->reserveScriptEnv())
	{
		ScriptEnvironment* env = m_scriptInterface->getScriptEnv();

		env->setScriptId(m_scriptId, m_scriptInterface);
		env->setRealPos(player->getPosition());

		uint32_t cid = env->addThing(player);

		lua_State* L = m_scriptInterface->getLuaState();

		m_scriptInterface->pushFunction(m_scriptId);
		lua_pushnumber(L, cid);
		lua_pushnumber(L, (uint32_t)skill);
		lua_pushnumber(L, oldLevel);
		lua_pushnumber(L, newLevel);

		bool result = m_scriptInterface->callFunction(4);
		m_scriptInterface->releaseScriptEnv();
		return result;
	}
	else
	{
		std::clog << "[Error] Call stack overflow. CreatureEvent::executeOnAdvance" << std::endl;
		return 0;
	}
}

uint32_t CreatureEvent::executeOnLook(Player* player, Thing* target, uint16_t itemId)
{
	//onLook(cid, thing, itemId)
	if(m_scriptInterface->reserveScriptEnv())
	{
		ScriptEnvironment* env = m_scriptInterface->getScriptEnv();

		env->setScriptId(m_scriptId, m_scriptInterface);
		env->setRealPos(player->getPosition());

		lua_State* L = m_scriptInterface->getLuaState();
		m_scriptInterface->pushFunction(m_scriptId);
		uint32_t cid = env->addThing(player);
		lua_pushnumber(L, cid);

		uint32_t target_id = 0;
		if(target)
		{
			if(target->getCreature())
				target_id = env->addThing(target->getCreature());
			else if(target->getItem())
				target_id = env->addThing(target->getItem());
			else
				target = NULL;
		}

		if(target)
			LuaScriptInterface::pushThing(L, target, target_id);
		else
			lua_pushnil(L);

		lua_pushnumber(L, itemId);

		bool result = m_scriptInterface->callFunction(3);
		m_scriptInterface->releaseScriptEnv();
		return result;
	}
	else
	{
		std::clog << "[Error] Call stack overflow. CreatureEvent::executeOnLook" << std::endl;
		return 0;
	}
}
