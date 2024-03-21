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

#include "raids.h"

#include "game.h"
#include "player.h"
#include "configmanager.h"

#include <algorithm>

extern Game g_game;
extern ConfigManager g_config;

Raids::Raids()
{
	loaded = false;
	started = false;
	running = NULL;
	lastRaidEnd = 0;
	checkRaidsEvent = 0;
}

Raids::~Raids()
{
	clear();
}

bool Raids::loadFromXml()
{
	if(isLoaded()) {
		return true;
	}

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file("data/raids/raids.xml");
	if(!result) {
		std::clog << "[Error - Raids::loadFromXml] Failed to load data/raids/raids.xml: " << result.description() << std::endl;
		return false;
	}

	for(pugi::xml_node raidNode = doc.child("raids").first_child(); raidNode; raidNode = raidNode.next_sibling()) {
		std::string name, file;
		uint32_t interval, margin;

		pugi::xml_attribute nameAttribute = raidNode.attribute("name");
		if(nameAttribute) {
			name = nameAttribute.as_string();
		} else {
			std::clog << "[Error - Raids::loadFromXml] Name tag missing for raid" << std::endl;
			continue;
		}

		pugi::xml_attribute fileAttribute = raidNode.attribute("file");
		if(fileAttribute) {
			file = fileAttribute.as_string();
		} else {
			std::ostringstream ss;
			ss << "raids/" << name << ".xml";
			file = ss.str();
			std::clog << "[Warning - Raids::loadFromXml] File tag missing for raid " << name << ". Using default: " << file << std::endl;
		}

		interval = pugi::cast<uint32_t>(raidNode.attribute("interval2").value()) * 60;
		if(interval == 0) {
			std::clog << "[Error - Raids::loadFromXml] interval2 tag missing or zero (would divide by 0) for raid: " << name << std::endl;
			continue;
		}

		pugi::xml_attribute marginAttribute = raidNode.attribute("margin");
		if(marginAttribute) {
			margin = pugi::cast<uint32_t>(marginAttribute.value()) * 60 * 1000;
		} else {
			std::clog << "[Warning - Raids::loadFromXml] margin tag missing for raid: " << name << std::endl;
			margin = 0;
		}

		Raid* newRaid = new Raid(name, interval, margin);
		if(newRaid->loadFromXml("data/raids/" + file)) {
			raidList.push_back(newRaid);
		} else {
			std::clog << "[Error - Raids::loadFromXml] Failed to load raid: " << name << std::endl;
			delete newRaid;
		}
	}

	loaded = true;
	return true;
}

#define MAX_RAND_RANGE 10000000

bool Raids::startup()
{
	if(!isLoaded() || isStarted())
		return false;
	
	setLastRaidEnd(OTSYS_TIME());
	
	checkRaidsEvent = Scheduler::getScheduler().addEvent(createSchedulerTask(CHECK_RAIDS_INTERVAL * 1000, boost::bind(&Raids::checkRaids, this)));

	started = true;
	return started;
}


void Raids::checkRaids()
{
	if(!getRunning())
	{
		uint64_t now = OTSYS_TIME();
		for(RaidList::iterator it = raidList.begin(); it != raidList.end(); ++it)
		{
			if(now >= (getLastRaidEnd() + (*it)->getMargin()))
			{
				if(MAX_RAND_RANGE*CHECK_RAIDS_INTERVAL/(*it)->getInterval() >= (uint32_t)random_range(0, MAX_RAND_RANGE))
				{
					#ifdef __DEBUG_RAID__
					char buffer[40];
					uint64_t tmp = time(NULL);
					formatDate(tmp, buffer);
					std::clog << buffer << " [Notice] Raids: Starting raid " << (*it)->getName() << std::endl;
					#endif
					setRunning(*it);
					(*it)->startRaid();
					break;
				}
			}

		}
	}
	checkRaidsEvent = Scheduler::getScheduler().addEvent(createSchedulerTask(CHECK_RAIDS_INTERVAL * 1000, boost::bind(&Raids::checkRaids, this)));
}

void Raids::clear()
{
	Scheduler::getScheduler().stopEvent(checkRaidsEvent);
	checkRaidsEvent = 0;

	RaidList::iterator it;
	for(it = raidList.begin(); it != raidList.end(); ++it)
		delete (*it);
	raidList.clear();

	loaded = false;
	started = false;
	running = NULL;
	lastRaidEnd = 0;
}

bool Raids::reload()
{
	clear();
	return loadFromXml();
}

Raid* Raids::getRaidByName(const std::string& name)
{
	RaidList::iterator it;
	for(it = raidList.begin(); it != raidList.end(); it++)
	{
		if(strcasecmp((*it)->getName().c_str(), name.c_str()) == 0)
			return (*it);
	}

	return NULL;
}

Raid::Raid(const std::string& _name, uint32_t _interval, uint32_t _marginTime)
{
	loaded = false;
	name = _name;
	interval = _interval;
	nextEvent = 0;
	state = RAIDSTATE_IDLE;
	margin = _marginTime;
	nextEventEvent = 0;
}

Raid::~Raid()
{
	stopEvents();

	RaidEventVector::iterator it;
	for(it = raidEvents.begin(); it != raidEvents.end(); it++)
		delete (*it);
	raidEvents.clear();
}

bool Raid::loadFromXml(const std::string& _filename)
{
	if(isLoaded()) {
		return true;
	}

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(_filename.c_str());
	if(!result) {
		std::clog << "[Error - Raid::loadFromXml] Failed to load " << _filename << ": " << result.description() << std::endl;
		return false;
	}

	for(pugi::xml_node eventNode = doc.child("raid").first_child(); eventNode; eventNode = eventNode.next_sibling()) {
		RaidEvent* event;
		if(strcasecmp(eventNode.name(), "announce") == 0) {
			event = new AnnounceEvent();
		} else if(strcasecmp(eventNode.name(), "singlespawn") == 0) {
			event = new SingleSpawnEvent();
		} else if(strcasecmp(eventNode.name(), "areaspawn") == 0) {
			event = new AreaSpawnEvent();
		} else if(strcasecmp(eventNode.name(), "script") == 0) {
			event = new ScriptEvent();
		} else {
			continue;
		}

		if(event->configureRaidEvent(eventNode)) {
			raidEvents.push_back(event);
		} else {
			std::clog << "[Error - Raid::loadFromXml] In file (" << _filename << "), eventNode: " << eventNode.name() << std::endl;
			delete event;
		}
	}

	//sort by delay time
	std::sort(raidEvents.begin(), raidEvents.end(), RaidEvent::compareEvents);

	loaded = true;
	return true;
}

void Raid::startRaid()
{
	RaidEvent* raidEvent = getNextRaidEvent();
	if(raidEvent)
	{
		state = RAIDSTATE_EXECUTING;
		nextEventEvent = Scheduler::getScheduler().addEvent(createSchedulerTask(raidEvent->getDelay(), boost::bind(&Raid::executeRaidEvent, this, raidEvent)));
	}
}

void Raid::executeRaidEvent(RaidEvent* raidEvent)
{
	if(raidEvent->executeEvent())
	{
		nextEvent++;
		RaidEvent* newRaidEvent = getNextRaidEvent();
		if(newRaidEvent)
		{
			uint32_t ticks = (uint32_t)std::max<uint32_t>(((uint32_t)RAID_MINTICKS), ((int32_t)newRaidEvent->getDelay() - raidEvent->getDelay()));
			nextEventEvent = Scheduler::getScheduler().addEvent(createSchedulerTask(ticks, boost::bind(&Raid::executeRaidEvent, this, newRaidEvent)));
		}
		else
			resetRaid();
	}
	else
		resetRaid();
}

void Raid::resetRaid()
{
#ifdef __DEBUG_RAID__
	std::clog << "[Notice] Raids: Resetting raid." << std::endl;
#endif

	nextEvent = 0;
	state = RAIDSTATE_IDLE;
	Raids::getInstance()->setRunning(NULL);
	Raids::getInstance()->setLastRaidEnd(OTSYS_TIME());
}

void Raid::stopEvents()
{
	if(nextEventEvent != 0)
	{
		Scheduler::getScheduler().stopEvent(nextEventEvent);
		nextEventEvent = 0;
	}
}

RaidEvent* Raid::getNextRaidEvent()
{
	if(nextEvent < raidEvents.size())
		return raidEvents[nextEvent];
	else
		return NULL;
}

void Raid::addEvent(RaidEvent* event)
{
	raidEvents.push_back(event);
}

bool RaidEvent::configureRaidEvent(const pugi::xml_node& eventNode)
{
	pugi::xml_attribute delayAttribute = eventNode.attribute("delay");
	if(!delayAttribute) {
		std::clog << "[Error] Raid: delay tag missing." << std::endl;
		return false;
	}

	m_delay = pugi::cast<uint32_t>(delayAttribute.value());
	if(m_delay < RAID_MINTICKS) {
		m_delay = RAID_MINTICKS;
	}
	return true;
}

bool AnnounceEvent::configureRaidEvent(const pugi::xml_node& eventNode)
{
	if(!RaidEvent::configureRaidEvent(eventNode)) {
		return false;
	}

	pugi::xml_attribute messageAttribute = eventNode.attribute("message");
	if(messageAttribute) {
		m_message = messageAttribute.as_string();
	} else {
		std::clog << "[Error] Raid: message tag missing for announce event." << std::endl;
		return false;
	}

	pugi::xml_attribute typeAttribute = eventNode.attribute("type");
	if(typeAttribute) {
		std::string tmpStrValue = asLowerCaseString(typeAttribute.as_string());
		if(tmpStrValue == "warning") {
			m_messageType = MSG_STATUS_WARNING;
		} else if(tmpStrValue == "event") {
			m_messageType = MSG_EVENT_ADVANCE;
		} else if(tmpStrValue == "default") {
			m_messageType = MSG_EVENT_DEFAULT;
		} else if(tmpStrValue == "description") {
			m_messageType = MSG_INFO_DESCR;
		} else if(tmpStrValue == "smallstatus") {
			m_messageType = MSG_STATUS_SMALL;
		} else if(tmpStrValue == "blueconsole") {
			m_messageType = MSG_STATUS_CONSOLE_BLUE;
		} else if(tmpStrValue == "redconsole") {
			m_messageType = MSG_STATUS_CONSOLE_RED;
		} else {
			std::clog << "[Notice] Raid: Unknown type tag missing for announce event. Using default: " << (int32_t)m_messageType << std::endl;
		}
	} else {
		m_messageType = MSG_EVENT_ADVANCE;
		std::clog << "[Notice] Raid: type tag missing for announce event. Using default: " << (int32_t)m_messageType << std::endl;
	}
	return true;
}

bool AnnounceEvent::executeEvent()
{
	g_game.broadcastMessage(m_message, m_messageType);
	return true;
}

bool SingleSpawnEvent::configureRaidEvent(const pugi::xml_node& eventNode)
{
	if(!RaidEvent::configureRaidEvent(eventNode)) {
		return false;
	}

	pugi::xml_attribute nameAttribute = eventNode.attribute("name");
	if(nameAttribute) {
		m_monsterName = nameAttribute.as_string();
	} else {
		std::clog << "[Error] Raid: name tag missing for singlespawn event." << std::endl;
		return false;
	}

	pugi::xml_attribute xAttribute = eventNode.attribute("x");
	if(xAttribute) {
		m_position.x = pugi::cast<uint16_t>(xAttribute.value());
	} else {
		std::clog << "[Error] Raid: x tag missing for singlespawn event." << std::endl;
		return false;
	}

	pugi::xml_attribute yAttribute = eventNode.attribute("y");
	if(yAttribute) {
		m_position.y = pugi::cast<uint16_t>(yAttribute.value());
	} else {
		std::clog << "[Error] Raid: y tag missing for singlespawn event." << std::endl;
		return false;
	}

	pugi::xml_attribute zAttribute = eventNode.attribute("z");
	if(zAttribute) {
		m_position.z = pugi::cast<uint16_t>(zAttribute.value());
	} else {
		std::clog << "[Error] Raid: z tag missing for singlespawn event." << std::endl;
		return false;
	}
	return true;
}

bool SingleSpawnEvent::executeEvent()
{
	Monster* monster = Monster::createMonster(m_monsterName);
	if(!monster)
	{
		std::clog << "[Error] Raids: Cant create monster " << m_monsterName << std::endl;
		return false;
	}

	if(!g_game.placeCreature(monster, m_position))
	{
		delete monster;
		std::clog << "[Error] Raids: Cant place monster " << m_monsterName << std::endl;
		return false;
	}

	return true;
}

bool AreaSpawnEvent::configureRaidEvent(const pugi::xml_node& eventNode)
{
	if(!RaidEvent::configureRaidEvent(eventNode)) {
		return false;
	}

	pugi::xml_attribute radiusAttribute = eventNode.attribute("radius");
	if(radiusAttribute) {
		int32_t radius = pugi::cast<int32_t>(radiusAttribute.value());
		Position centerPos;

		pugi::xml_attribute xAttribute = eventNode.attribute("centerx");
		if(xAttribute) {
			centerPos.x = pugi::cast<uint16_t>(xAttribute.value());
		} else {
			std::clog << "[Error] Raid: centerx tag missing for areaspawn event." << std::endl;
			return false;
		}

		pugi::xml_attribute yAttribute = eventNode.attribute("centery");
		if(yAttribute) {
			centerPos.y = pugi::cast<uint16_t>(yAttribute.value());
		} else {
			std::clog << "[Error] Raid: centery tag missing for areaspawn event." << std::endl;
			return false;
		}

		pugi::xml_attribute zAttribute = eventNode.attribute("centerz");
		if(zAttribute) {
			centerPos.z = pugi::cast<uint16_t>(zAttribute.value());
		} else {
			std::clog << "[Error] Raid: centerz tag missing for areaspawn event." << std::endl;
			return false;
		}

		m_fromPos.x = centerPos.x - radius;
		m_fromPos.y = centerPos.y - radius;
		m_fromPos.z = centerPos.z;

		m_toPos.x = centerPos.x + radius;
		m_toPos.y = centerPos.y + radius;
		m_toPos.z = centerPos.z;
	} else {
		pugi::xml_attribute fromxAttribute = eventNode.attribute("fromx");
		if(fromxAttribute) {
			m_fromPos.x = pugi::cast<uint16_t>(fromxAttribute.value());
		} else {
			std::clog << "[Error] Raid: fromx tag missing for areaspawn event." << std::endl;
			return false;
		}

		pugi::xml_attribute fromyAttribute = eventNode.attribute("fromy");
		if(fromyAttribute) {
			m_fromPos.y = pugi::cast<uint16_t>(fromyAttribute.value());
		} else {
			std::clog << "[Error] Raid: fromy tag missing for areaspawn event." << std::endl;
			return false;
		}

		pugi::xml_attribute fromzAttribute = eventNode.attribute("fromz");
		if(fromzAttribute) {
			m_fromPos.z = pugi::cast<uint16_t>(fromzAttribute.value());
		} else {
			std::clog << "[Error] Raid: fromz tag missing for areaspawn event." << std::endl;
			return false;
		}

		pugi::xml_attribute toxAttribute = eventNode.attribute("tox");
		if(toxAttribute) {
			m_toPos.x = pugi::cast<uint16_t>(toxAttribute.value());
		} else {
			std::clog << "[Error] Raid: tox tag missing for areaspawn event." << std::endl;
			return false;
		}

		pugi::xml_attribute toyAttribute = eventNode.attribute("toy");
		if(toyAttribute) {
			m_toPos.y = pugi::cast<uint16_t>(toyAttribute.value());
		} else {
			std::clog << "[Error] Raid: toy tag missing for areaspawn event." << std::endl;
			return false;
		}

		pugi::xml_attribute tozAttribute = eventNode.attribute("toz");
		if(tozAttribute) {
			m_toPos.z = pugi::cast<uint16_t>(tozAttribute.value());
		} else {
			std::clog << "[Error] Raid: toz tag missing for areaspawn event." << std::endl;
			return false;
		}
	}

	for(pugi::xml_node monsterNode = eventNode.child("monster").first_child(); monsterNode; monsterNode = monsterNode.next_sibling()) {
		std::string name;

		pugi::xml_attribute nameAttribute = monsterNode.attribute("name");
		if(nameAttribute) {
			name = nameAttribute.as_string();
		} else {
			std::clog << "[Error] Raid: name tag missing for monster node." << std::endl;
			return false;
		}

		int32_t minAmount;
		pugi::xml_attribute minAmountAttribute = monsterNode.attribute("minamount");
		if(minAmountAttribute) {
			minAmount = pugi::cast<uint32_t>(minAmountAttribute.value());
		} else {
			minAmount = 0;
		}

		int32_t maxAmount;
		pugi::xml_attribute maxAmountAttribute = monsterNode.attribute("maxamount");
		if(maxAmountAttribute) {
			maxAmount = pugi::cast<uint32_t>(maxAmountAttribute.value());
		} else {
			maxAmount = 0;
		}

		if(maxAmount == 0 && minAmount == 0) {
			pugi::xml_attribute amountAttribute = monsterNode.attribute("amount");
			if(amountAttribute) {
				minAmount = pugi::cast<uint32_t>(amountAttribute.value());
				maxAmount = minAmount;
			} else {
				std::clog << "[Error] Raid: amount tag missing for monster node." << std::endl;
				return false;
			}
		}

		addMonster(name, minAmount, maxAmount);
	}
	return true;
}

AreaSpawnEvent::~AreaSpawnEvent()
{
	MonsterSpawnList::iterator it;
	for(it = m_spawnList.begin(); it != m_spawnList.end(); it++)
	{
		delete (*it);
	}

	m_spawnList.clear();
}

void AreaSpawnEvent::addMonster(MonsterSpawn* monsterSpawn)
{
	m_spawnList.push_back(monsterSpawn);
}

void AreaSpawnEvent::addMonster(const std::string& monsterName, uint32_t minAmount, uint32_t maxAmount)
{
	MonsterSpawn* monsterSpawn = new MonsterSpawn();
	monsterSpawn->name = monsterName;
	monsterSpawn->minAmount = minAmount;
	monsterSpawn->maxAmount = maxAmount;
	addMonster(monsterSpawn);
}

bool AreaSpawnEvent::executeEvent()
{
	for(MonsterSpawn* spawn : m_spawnList) {
		uint32_t amount = random_range(spawn->minAmount, spawn->maxAmount);
		for(uint32_t i = 0; i < amount; ++i) {
			Monster* monster = Monster::createMonster(spawn->name);
			if(!monster) {
				std::clog << "[Error - AreaSpawnEvent::executeEvent] Can't create monster " << spawn->name << std::endl;
				return false;
			}

			bool success = false;
			for(int32_t tries = 0; tries < MAXIMUM_TRIES_PER_MONSTER; tries++)
			{
				Position pos;
				pos.x = random_range(m_fromPos.x, m_toPos.x);
				pos.y = random_range(m_fromPos.y, m_toPos.y);
				pos.z = random_range(m_fromPos.z, m_toPos.z);
				
				if(g_game.placeCreature(monster, pos))
				{
					success = true;
					break;
				}
			}

			if(!success) {
				delete monster;
			}
		}
	}
	return true;
}

LuaScriptInterface ScriptEvent::m_scriptInterface("Raid Interface");

ScriptEvent::ScriptEvent() :
Event(&m_scriptInterface)
{
	m_scriptInterface.initState();
}

void ScriptEvent::reInitScriptInterface()
{
	m_scriptInterface.reInitState();
}

bool ScriptEvent::configureRaidEvent(const pugi::xml_node& eventNode)
{
	if(!RaidEvent::configureRaidEvent(eventNode)) {
		return false;
	}

	pugi::xml_attribute scriptAttribute = eventNode.attribute("script");
	if(!scriptAttribute) {
		std::clog << "Error: [ScriptEvent::configureRaidEvent] No script file found for raid" << std::endl;
		return false;
	}

	if(!loadScript("data/raids/scripts/" + std::string(scriptAttribute.as_string()))) {
		std::clog << "Error: [ScriptEvent::configureRaidEvent] Can not load raid script." << std::endl;
		return false;
	}
	return true;
}

std::string ScriptEvent::getScriptEventName()
{
	return "onRaid";
}

bool ScriptEvent::executeEvent()
{
	//onRaid()
	if(m_scriptInterface.reserveScriptEnv())
	{
		ScriptEnvironment* env = m_scriptInterface.getScriptEnv();

		env->setScriptId(m_scriptId, &m_scriptInterface);
		
		m_scriptInterface.pushFunction(m_scriptId);
	
		bool result = m_scriptInterface.callFunction(0) != 0;
		m_scriptInterface.releaseScriptEnv();

		return result;
	}
	else
	{
		std::clog << "[Error] Call stack overflow. ScriptEvent::executeEvent" << std::endl;
		return 0;
	}
}
