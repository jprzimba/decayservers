////////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
////////////////////////////////////////////////////////////////////////
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
////////////////////////////////////////////////////////////////////////
#include "otpch.h"
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include "globalevent.h"
#include "tools.h"
#include "player.h"

GlobalEvents::GlobalEvents():
	m_interface("GlobalEvent Interface")
{
	m_interface.initState();
}

GlobalEvents::~GlobalEvents()
{
	clear();
}

void GlobalEvents::clearMap(GlobalEventMap& map)
{
	GlobalEventMap::iterator it;
	for(it = map.begin(); it != map.end(); ++it)
		delete it->second;

	map.clear();
}

void GlobalEvents::clear()
{
	clearMap(thinkMap);
	clearMap(serverMap);
	clearMap(timerMap);

	m_interface.reInitState();
}

Event* GlobalEvents::getEvent(const std::string& nodeName)
{
	if(asLowerCaseString(nodeName) == "globalevent")
		return new GlobalEvent(&m_interface);

	return NULL;
}

bool GlobalEvents::registerEvent(Event* event, xmlNodePtr p, bool override)
{
	GlobalEvent* globalEvent = dynamic_cast<GlobalEvent*>(event);
	if(!globalEvent)
		return false;

	GlobalEventMap* map = &thinkMap;
	if(globalEvent->getEventType() == GLOBALEVENT_TIMER)
		map = &timerMap;
	else if(globalEvent->getEventType() != GLOBALEVENT_NONE)
		map = &serverMap;

	GlobalEventMap::iterator it = map->find(globalEvent->getName());
	if(it == map->end())
	{
		map->insert(std::make_pair(globalEvent->getName(), globalEvent));
		return true;
	}

	if(override)
	{
		delete it->second;
		it->second = globalEvent;
		return true;
	}

	std::clog << "[Warning - GlobalEvents::configureEvent] Duplicate registered globalevent with name: " << globalEvent->getName() << std::endl;
	return false;
}

void GlobalEvents::startup()
{
	execute(GLOBALEVENT_STARTUP);
	Scheduler::getInstance().addEvent(createSchedulerTask(TIMER_INTERVAL,
		boost::bind(&GlobalEvents::timer, this)));
	Scheduler::getInstance().addEvent(createSchedulerTask(SCHEDULER_MINTICKS,
		boost::bind(&GlobalEvents::think, this)));
}

void GlobalEvents::timer()
{
	time_t now = time(NULL);
	tm* ts = localtime(&now);

	uint32_t hour = (uint32_t)ts->tm_hour, minute = (uint32_t)ts->tm_min;
	for(GlobalEventMap::iterator it = timerMap.begin(); it != timerMap.end(); ++it)
	{
		if(hour != it->second->getHour() || minute != it->second->getMinute())
			continue;

		if(!it->second->executeEvent())
			std::clog << "[Error - GlobalEvents::timer] Couldn't execute event: "
				<< it->second->getName() << std::endl;
	}

	now = std::max(1, (int32_t)(60 - ts->tm_sec)) * 1000;
	Scheduler::getInstance().addEvent(createSchedulerTask((int32_t)now,
		boost::bind(&GlobalEvents::timer, this)));
}

void GlobalEvents::think()
{
	time_t now = time(NULL);
	for(GlobalEventMap::iterator it = thinkMap.begin(); it != thinkMap.end(); ++it)
	{
		if((it->second->getLastExecution() + it->second->getInterval()) > now)
			continue;

		it->second->setLastExecution(now);
		if(!it->second->executeEvent())
			std::clog << "[Error - GlobalEvents::think] Couldn't execute event: "
				<< it->second->getName() << std::endl;
	}

	Scheduler::getInstance().addEvent(createSchedulerTask(SCHEDULER_MINTICKS,
		boost::bind(&GlobalEvents::think, this)));
}

void GlobalEvents::execute(GlobalEvent_t type)
{
	for(GlobalEventMap::iterator it = serverMap.begin(); it != serverMap.end(); ++it)
	{
		if(it->second->getEventType() == type)
			it->second->executeEvent();
	}
}

GlobalEventMap GlobalEvents::getEventMap(GlobalEvent_t type)
{
	switch(type)
	{
		case GLOBALEVENT_NONE:
			return thinkMap;

		case GLOBALEVENT_TIMER:
			return timerMap;

		case GLOBALEVENT_STARTUP:
		case GLOBALEVENT_SHUTDOWN:
		case GLOBALEVENT_GLOBALSAVE:
		case GLOBALEVENT_RECORD:
		{
			GlobalEventMap retMap;
			for(GlobalEventMap::iterator it = serverMap.begin(); it != serverMap.end(); ++it)
			{
				if(it->second->getEventType() == type)
					retMap[it->first] = it->second;
			}

			return retMap;
		}

		default:
			break;
	}

	return GlobalEventMap();
}

GlobalEvent::GlobalEvent(LuaInterface* _interface):
	Event(_interface)
{
	m_lastExecution = time(NULL);
	m_hour = m_minute = 0;
}

bool GlobalEvent::configureEvent(xmlNodePtr p)
{
	std::string strValue;
	if(!readXMLString(p, "name", strValue))
	{
		std::clog << "[Error - GlobalEvent::configureEvent] No name for a globalevent." << std::endl;
		return false;
	}

	m_name = strValue;
	m_eventType = GLOBALEVENT_NONE;
	if(readXMLString(p, "type", strValue))
	{
		std::string tmpStrValue = asLowerCaseString(strValue);
		if(tmpStrValue == "startup" || tmpStrValue == "start" || tmpStrValue == "load")
			m_eventType = GLOBALEVENT_STARTUP;
		else if(tmpStrValue == "shutdown" || tmpStrValue == "quit" || tmpStrValue == "exit")
			m_eventType = GLOBALEVENT_SHUTDOWN;
		else if(tmpStrValue == "globalsave" || tmpStrValue == "global")
			m_eventType = GLOBALEVENT_GLOBALSAVE;
		else if(tmpStrValue == "record" || tmpStrValue == "playersrecord")
			m_eventType = GLOBALEVENT_RECORD;
		else
		{
			std::clog << "[Error - GlobalEvent::configureEvent] No valid type \"" << strValue << "\" for globalevent with name " << m_name << std::endl;
			return false;
		}

		return true;
	}
	else if(readXMLString(p, "time", strValue) || readXMLString(p, "at", strValue))
	{
		IntegerVec params = vectorAtoi(explodeString(strValue, ":"));
		if(params.size() < 2 || params[0] > 23 || params[0] < 0 || params[1] > 59 || params[1] < 0)
		{
			std::clog << "[Error - GlobalEvent::configureEvent] No valid time \"" << strValue << "\" for globalevent with name " << m_name << std::endl;
			return false;
		}

		m_hour = params[0], m_minute = params[1];
		m_eventType = GLOBALEVENT_TIMER;
		return true;
	}
	else
	{
		int32_t intValue;
		if(readXMLInteger(p, "interval", intValue))
		{
			m_interval = intValue;
			return true;
		}
	}

	std::clog << "[Error - GlobalEvent::configureEvent] No interval for globalevent with name " << m_name << std::endl;
	return false;
}

std::string GlobalEvent::getScriptEventName() const
{
	switch(m_eventType)
	{
		case GLOBALEVENT_STARTUP:
			return "onStartup";
		case GLOBALEVENT_SHUTDOWN:
			return "onShutdown";
		case GLOBALEVENT_RECORD:
			return "onRecord";
		case GLOBALEVENT_TIMER:
			return "onTimer";
		case GLOBALEVENT_GLOBALSAVE:
			return "onGlobalSave";
		default:
			return "onThink";
	}
}

std::string GlobalEvent::getScriptEventParams() const
{
	switch(m_eventType)
	{
		case GLOBALEVENT_RECORD:
			return "current, old, cid";

		case GLOBALEVENT_NONE:
			return "interval, lastExecution, thinkInterval";

		default:
			return "";
	}
}

int32_t GlobalEvent::executeRecord(uint32_t current, uint32_t old, Player* player)
{
	//onRecord(current, old, cid)
	if(m_interface->reserveEnv())
	{
		ScriptEnviroment* env = m_interface->getEnv();
		if(m_scripted == EVENT_SCRIPT_BUFFER)
		{
			std::stringstream scriptstream;
			scriptstream << "local current = " << current << std::endl;
			scriptstream << "local old = " << old << std::endl;
			scriptstream << "local cid = " << env->addThing(player) << std::endl;

			if(m_scriptData)
				scriptstream << *m_scriptData;

			bool result = true;
			if(m_interface->loadBuffer(scriptstream.str()))
			{
				lua_State* L = m_interface->getState();
				result = m_interface->getGlobalBool(L, "_result", true);
			}

			m_interface->releaseEnv();
			return result;
		}
		else
		{
			#ifdef __DEBUG_LUASCRIPTS__
			char desc[125];
			sprintf(desc, "%s - %i to %i (%i)", getName().c_str(), old, current, player->getName().c_str());
			env->setEventDesc(desc);
			#endif

			env->setScriptId(m_scriptId, m_interface);
			lua_State* L = m_interface->getState();

			m_interface->pushFunction(m_scriptId);
			lua_pushnumber(L, current);
			lua_pushnumber(L, old);
			lua_pushnumber(L, env->addThing(player));

			bool result = m_interface->callFunction(3);
			m_interface->releaseEnv();
			return result;
		}
	}
	else
	{
		std::clog << "[Error - GlobalEvent::executeRecord] Call stack overflow." << std::endl;
		return 0;
	}
}

int32_t GlobalEvent::executeEvent()
{
	if(m_interface->reserveEnv())
	{
		ScriptEnviroment* env = m_interface->getEnv();
		if(m_scripted == EVENT_SCRIPT_BUFFER)
		{
			std::stringstream scriptstream;
			if(m_eventType == GLOBALEVENT_NONE)
				scriptstream << "local interval = " << m_interval << std::endl;
			else if(m_eventType == GLOBALEVENT_TIMER)
				scriptstream << "local time = " << m_interval << std::endl;

			if(m_scriptData)
				scriptstream << *m_scriptData;

			bool result = true;
			if(m_interface->loadBuffer(scriptstream.str()))
			{
				lua_State* L = m_interface->getState();
				result = m_interface->getGlobalBool(L, "_result", true);
			}

			m_interface->releaseEnv();
			return result;
		}
		else
		{
			env->setScriptId(m_scriptId, m_interface);
			lua_State* L = m_interface->getState();
			m_interface->pushFunction(m_scriptId);

			int32_t params = 0;
			if(m_eventType == GLOBALEVENT_NONE || m_eventType == GLOBALEVENT_TIMER)
			{
				lua_pushnumber(L, m_interval);
				params = 1;
			}

			bool result = m_interface->callFunction(params);
			m_interface->releaseEnv();
			return result;
		}
	}
	else
	{
		std::clog << "[Error - GlobalEvent::executeEvent] Call stack overflow." << std::endl;
		return 0;
	}
}
