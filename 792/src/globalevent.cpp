////////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
////////////////////////////////////////////////////////////////////////
// Global events
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

#include "globalevent.h"
#include "tools.h"
#include "player.h"
#include "scheduler.h"
#include <cmath>

GlobalEvents::GlobalEvents() :
	m_scriptInterface("GlobalEvent Interface")
{
	m_scriptInterface.initState();
	thinkEventId = 0;
	timerEventId = 0;
}

GlobalEvents::~GlobalEvents()
{
	clear();
}

void GlobalEvents::clearMap(GlobalEventMap& map)
{
	for(GlobalEventMap::iterator it = map.begin(); it != map.end(); ++it)
		delete it->second;

	map.clear();
}

void GlobalEvents::clear()
{
	Scheduler::getScheduler().stopEvent(thinkEventId);
	thinkEventId = 0;
	Scheduler::getScheduler().stopEvent(timerEventId);
	timerEventId = 0;

	clearMap(thinkMap);
	clearMap(serverMap);
	clearMap(timerMap);

	m_scriptInterface.reInitState();
}

Event* GlobalEvents::getEvent(const std::string& nodeName)
{
	if(asLowerCaseString(nodeName) == "globalevent")
		return new GlobalEvent(&m_scriptInterface);

	return NULL;
}

bool GlobalEvents::registerEvent(Event* event, const pugi::xml_node& node)
{
	GlobalEvent* globalEvent = dynamic_cast<GlobalEvent*>(event);
	if(!globalEvent)
		return false;

	if(globalEvent->getEventType() == GLOBALEVENT_TIMER)
	{
		GlobalEventMap::iterator it = timerMap.find(globalEvent->getName());
		if(it == timerMap.end())
		{
			timerMap.insert(std::make_pair(globalEvent->getName(), globalEvent));
			if(timerEventId == 0)
			{
				timerEventId = Scheduler::getScheduler().addEvent(createSchedulerTask(TIMER_INTERVAL,
					boost::bind(&GlobalEvents::timer, this)));
			}
			return true;
		}
	}
	else if(globalEvent->getEventType() != GLOBALEVENT_NONE)
	{
		GlobalEventMap::iterator it = serverMap.find(globalEvent->getName());
		if(it == serverMap.end())
		{
			serverMap.insert(std::make_pair(globalEvent->getName(), globalEvent));
			return true;
		}
	}
	else // think event
	{
		GlobalEventMap::iterator it = thinkMap.find(globalEvent->getName());
		if(it == thinkMap.end())
		{
			thinkMap.insert(std::make_pair(globalEvent->getName(), globalEvent));
			if(thinkEventId == 0)
			{
				thinkEventId = Scheduler::getScheduler().addEvent(createSchedulerTask(SCHEDULER_MINTICKS,
					boost::bind(&GlobalEvents::think, this)));
			}
			return true;
		}
	}

	std::clog << "[Warning - GlobalEvents::configureEvent] Duplicate registered globalevent with name: " << globalEvent->getName() << std::endl;
	return false;
}

void GlobalEvents::startup()
{
	execute(GLOBALEVENT_STARTUP);
}

void GlobalEvents::timer()
{
	time_t now = time(NULL);
	for(GlobalEventMap::iterator it = timerMap.begin(), end = timerMap.end(); it != end; ++it)
	{
		GlobalEvent* globalEvent = it->second;
		if(globalEvent->getNextExecution() > now)
			continue;

		globalEvent->setNextExecution(globalEvent->getNextExecution() + 86400);
		if(!globalEvent->executeEvent())
			std::clog << "[Error - GlobalEvents::timer] Failed to execute event: " << globalEvent->getName() << std::endl;
	}

	Scheduler::getScheduler().addEvent(createSchedulerTask(TIMER_INTERVAL,
		boost::bind(&GlobalEvents::timer, this)));
}

void GlobalEvents::think()
{
	int64_t now = OTSYS_TIME();
	for(GlobalEventMap::iterator it = thinkMap.begin(); it != thinkMap.end(); ++it)
	{
		GlobalEvent* globalEvent = it->second;
		if((globalEvent->getLastExecution() + globalEvent->getInterval()) > now)
			continue;

		globalEvent->setLastExecution(now);
		if(!globalEvent->executeEvent())
			std::clog << "[Error - GlobalEvents::think] Failed to execute event: " << globalEvent->getName() << std::endl;
	}

	Scheduler::getScheduler().addEvent(createSchedulerTask(SCHEDULER_MINTICKS,
		boost::bind(&GlobalEvents::think, this)));
}

void GlobalEvents::execute(GlobalEvent_t type)
{
	for(GlobalEventMap::iterator it = serverMap.begin(); it != serverMap.end(); ++it)
	{
		GlobalEvent* globalEvent = it->second;
		if(globalEvent->getEventType() == type)
			globalEvent->executeEvent();
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

GlobalEvent::GlobalEvent(LuaScriptInterface* _interface):
	Event(_interface)
{
	m_lastExecution = OTSYS_TIME();
	m_nextExecution = 0;
	m_interval = 0;
}

bool GlobalEvent::configureEvent(const pugi::xml_node& node)
{
	pugi::xml_attribute nameAttribute = node.attribute("name");
	if(!nameAttribute) {
		std::clog << "[Error - GlobalEvent::configureEvent] Missing name for a globalevent" << std::endl;
		return false;
	}

	m_name = nameAttribute.as_string();
	m_eventType = GLOBALEVENT_NONE;

	pugi::xml_attribute attr;
	if((attr = node.attribute("time"))) {
		std::vector<int32_t> params = vectorAtoi(explodeString(attr.as_string(), ":"));
		if(params[0] < 0 || params[0] > 23) {
			std::clog << "[Error - GlobalEvent::configureEvent] Invalid hour \"" << attr.as_string() << "\" for globalevent with name: " << m_name << std::endl;
			return false;
		}

		m_interval |= params[0] << 16;
		int32_t hour = params[0];
		int32_t min = 0;
		int32_t sec = 0;

		if(params.size() > 1) {
			if(params[1] < 0 || params[1] > 59) {
				std::clog << "[Error - GlobalEvent::configureEvent] Invalid minute \"" << attr.as_string() << "\" for globalevent with name: " << m_name << std::endl;
				return false;
			}

			min = params[1];

			if(params.size() > 2) {
				if(params[2] < 0 || params[2] > 59) {
					std::clog << "[Error - GlobalEvent::configureEvent] Invalid second \"" << attr.as_string() << "\" for globalevent with name: " << m_name << std::endl;
					return false;
				}

				sec = params[2];
			}
		}

		time_t current_time = time(NULL);
		tm* timeinfo = localtime(&current_time);
		timeinfo->tm_hour = hour;
		timeinfo->tm_min = min;
		timeinfo->tm_sec = sec;
		time_t difference = (time_t)difftime(mktime(timeinfo), current_time);

		if(difference < 0) {
			difference += 86400;
		}

		m_nextExecution = current_time + difference;
		m_eventType = GLOBALEVENT_TIMER;
	} else if((attr = node.attribute("type"))) {
		std::string tmpStrValue = asLowerCaseString(attr.as_string());
		if(tmpStrValue == "startup" || tmpStrValue == "start" || tmpStrValue == "load") {
			m_eventType = GLOBALEVENT_STARTUP;
		} else if(tmpStrValue == "shutdown" || tmpStrValue == "quit" || tmpStrValue == "exit") {
			m_eventType = GLOBALEVENT_SHUTDOWN;
		} else if(tmpStrValue == "record" || tmpStrValue == "playersrecord") {
			m_eventType = GLOBALEVENT_RECORD;
		} else {
			std::clog << "[Error - GlobalEvent::configureEvent] No valid type \"" << attr.as_string() << "\" for globalevent with name " << m_name << std::endl;
			return false;
		}
	} else if((attr = node.attribute("interval"))) {
		m_interval = std::max<int32_t>(SCHEDULER_MINTICKS, pugi::cast<int32_t>(attr.value()));
		m_nextExecution = OTSYS_TIME() + m_interval;
	} else {
		std::clog << "[Error - GlobalEvent::configureEvent] No interval for globalevent with name " << m_name << std::endl;
		return false;
	}
	return true;
}

std::string GlobalEvent::getScriptEventName()
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
			return "onTime";
		default:
			break;
	}

	return "onThink";
}

uint32_t GlobalEvent::executeRecord(uint32_t current, uint32_t old)
{
	//onRecord(current, old, cid)
	if(m_scriptInterface->reserveScriptEnv())
	{
		ScriptEnvironment* env = m_scriptInterface->getScriptEnv();

		env->setScriptId(m_scriptId, m_scriptInterface);

		lua_State* L = m_scriptInterface->getLuaState();
		m_scriptInterface->pushFunction(m_scriptId);
		lua_pushnumber(L, current);
		lua_pushnumber(L, old);

		bool result = m_scriptInterface->callFunction(2) != 0;
		m_scriptInterface->releaseScriptEnv();

		return result;
	}
	else
	{
		std::clog << "[Error - GlobalEvent::executeRecord] Call stack overflow." << std::endl;
		return 0;
	}
}

uint32_t GlobalEvent::executeEvent()
{
	if(m_scriptInterface->reserveScriptEnv())
	{
		ScriptEnvironment* env = m_scriptInterface->getScriptEnv();

		env->setScriptId(m_scriptId, m_scriptInterface);
		lua_State* L = m_scriptInterface->getLuaState();
		m_scriptInterface->pushFunction(m_scriptId);

		int32_t params = 0;
		if(m_eventType == GLOBALEVENT_NONE || m_eventType == GLOBALEVENT_TIMER)
		{
			lua_pushnumber(L, m_interval);
			params = 1;
		}

		bool result = m_scriptInterface->callFunction(params) != 0;
		m_scriptInterface->releaseScriptEnv();
		return result;
	}
	else
	{
		std::clog << "[Error - GlobalEvent::executeEvent] Call stack overflow." << std::endl;
		return 0;
	}
}
