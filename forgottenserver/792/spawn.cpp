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

#include "spawn.h"
#include "game.h"
#include "player.h"
#include "npc.h"
#include "tools.h"
#include "configmanager.h"

extern ConfigManager g_config;
extern Monsters g_monsters;
extern Game g_game;

#define MINSPAWN_INTERVAL 1000
#define DEFAULTSPAWN_INTERVAL 60000

Spawns::Spawns()
{
	loaded = false;
	started = false;
	filename = "";
}

Spawns::~Spawns()
{
	clear();
}

bool Spawns::loadFromXml(const std::string& _filename)
{
	if(isLoaded())
		return true;

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(_filename.c_str());
	if(!result) {
		std::cout << "[Error - Spawns::loadFromXml] Failed to load " << _filename << ": " << result.description() << std::endl;
		return false;
	}

	filename = _filename;
	
	for (pugi::xml_node spawnNode = doc.child("spawns").first_child(); spawnNode; spawnNode = spawnNode.next_sibling()) {
		Position centerPos(
			pugi::cast<int32_t>(spawnNode.attribute("centerx").value()),
			pugi::cast<int32_t>(spawnNode.attribute("centery").value()),
			pugi::cast<int32_t>(spawnNode.attribute("centerz").value())
		);

		int32_t radius;
		pugi::xml_attribute radiusAttribute = spawnNode.attribute("radius");
		if(radiusAttribute) {
			radius = pugi::cast<int32_t>(radiusAttribute.value());
		} else {
			return false;
		}

		Spawn* spawn = new Spawn(centerPos, radius);
		spawnList.push_back(spawn);

		for (pugi::xml_node childNode = spawnNode.first_child(); childNode; childNode = childNode.next_sibling()) {
			if(strcasecmp(childNode.name(), "monster") == 0) {
				pugi::xml_attribute nameAttribute = childNode.attribute("name");
				if(!nameAttribute) {
					continue;
				}

				Direction dir;

				pugi::xml_attribute directionAttribute = childNode.attribute("direction");
				if(directionAttribute) {
					dir = static_cast<Direction>(pugi::cast<uint16_t>(directionAttribute.value()));
				} else {
					dir = NORTH;
				}

				Position pos(
					centerPos.x + pugi::cast<int32_t>(childNode.attribute("x").value()),
					centerPos.y + pugi::cast<int32_t>(childNode.attribute("y").value()),
					centerPos.z
				);
				uint32_t interval = pugi::cast<uint32_t>(childNode.attribute("spawntime").value()) * 1000;
				if(interval > MINSPAWN_INTERVAL) {
					spawn->addMonster(nameAttribute.as_string(), pos, dir, interval);
				} else {
					std::cout << "[Warning - Spawns::loadFromXml] " << nameAttribute.as_string() << ' ' << pos << " spawntime can not be less than " << MINSPAWN_INTERVAL / 1000 << " seconds." << std::endl;
				}
			} else if(strcasecmp(childNode.name(), "npc") == 0) {
				pugi::xml_attribute nameAttribute = childNode.attribute("name");
				if(!nameAttribute) {
					continue;
				}

				Npc* npc = Npc::createNpc(nameAttribute.as_string());
				if(!npc) {
					continue;
				}

				pugi::xml_attribute directionAttribute = childNode.attribute("direction");
				if(directionAttribute) {
					npc->setDirection(static_cast<Direction>(pugi::cast<uint16_t>(directionAttribute.value())));
				}

				npc->setMasterPos(Position(
					centerPos.x + pugi::cast<uint16_t>(childNode.attribute("x").value()),
					centerPos.y + pugi::cast<uint16_t>(childNode.attribute("y").value()),
					centerPos.z
				), radius);
				npcList.push_back(npc);
			}
		}
	}
	loaded = true;
	return true;
}

void Spawns::startup()
{
	if(!isLoaded() || isStarted())
		return;

	for(NpcList::iterator it = npcList.begin(); it != npcList.end(); ++it)
		g_game.placeCreature((*it), (*it)->getMasterPos(), true);
	npcList.clear();

	for(SpawnList::iterator it = spawnList.begin(); it != spawnList.end(); ++it)
		(*it)->startup();

	started = true;
}

void Spawns::clear()
{
	for(SpawnList::iterator it= spawnList.begin(); it != spawnList.end(); ++it)
		delete (*it);

	spawnList.clear();

	loaded = false;
	started = false;
	filename = "";
}

bool Spawns::isInZone(const Position& centerPos, int32_t radius, const Position& pos)
{
	if(radius == -1)
		return true;

	return ((pos.x >= centerPos.x - radius) && (pos.x <= centerPos.x + radius) &&
		(pos.y >= centerPos.y - radius) && (pos.y <= centerPos.y + radius));
}

void Spawn::startSpawnCheck()
{
	if(checkSpawnEvent == 0)
		checkSpawnEvent = Scheduler::getScheduler().addEvent(createSchedulerTask(getInterval(), boost::bind(&Spawn::checkSpawn, this)));
}

Spawn::Spawn(const Position& _pos, int32_t _radius)
{
	centerPos = _pos;
	radius = _radius;
	interval = DEFAULTSPAWN_INTERVAL;
	checkSpawnEvent = 0;
}

Spawn::~Spawn()
{
	Monster* monster;
	for(SpawnedMap::iterator it = spawnedMap.begin(); it != spawnedMap.end(); ++it)
	{
		monster = it->second;
		it->second = NULL;

		monster->setSpawn(NULL);
		if(monster->isRemoved())
			monster->releaseThing2();
	}

	spawnedMap.clear();
	spawnMap.clear();

	stopEvent();
}

bool Spawn::findPlayer(const Position& pos)
{
	SpectatorVec list;
	SpectatorVec::iterator it;
	g_game.getSpectators(list, pos);

	Player* tmpPlayer = NULL;
	for(it = list.begin(); it != list.end(); ++it)
	{
		if((tmpPlayer = (*it)->getPlayer()) && !tmpPlayer->hasFlag(PlayerFlag_IgnoredByMonsters))
			return true;
	}
	return false;
}

bool Spawn::isInSpawnZone(const Position& pos)
{
	return Spawns::getInstance()->isInZone(centerPos, radius, pos);
}

bool Spawn::spawnMonster(uint32_t spawnId, MonsterType* mType, const Position& pos, Direction dir, bool startup /*= false*/)
{
	Monster* monster = Monster::createMonster(mType);
	if(!monster)
		return false;

	if(startup)
	{
		//No need to send out events to the surrounding since there is no one out there to listen!
		if(!g_game.internalPlaceCreature(monster, pos, true))
		{
			delete monster;
			return false;
		}
	}
	else
	{
		if(!g_game.placeCreature(monster, pos, true))
		{
			delete monster;
			return false;
		}
	}

	monster->setDirection(dir);
	monster->setSpawn(this);
	monster->setMasterPos(pos, radius);
	monster->useThing2();

	spawnedMap.insert(spawned_pair(spawnId, monster));
	spawnMap[spawnId].lastSpawn = OTSYS_TIME();
	return true;
}

void Spawn::startup()
{
	for(SpawnMap::iterator it = spawnMap.begin(); it != spawnMap.end(); ++it)
	{
		uint32_t spawnId = it->first;
		spawnBlock_t& sb = it->second;

		spawnMonster(spawnId, sb.mType, sb.pos, sb.direction, true);
	}
}

void Spawn::checkSpawn()
{
#ifdef __DEBUG_SPAWN__
	std::cout << "[Notice] Spawn::checkSpawn " << this << std::endl;
#endif
	checkSpawnEvent = 0;

	Monster* monster;
	uint32_t spawnId;

	for(SpawnedMap::iterator it = spawnedMap.begin(); it != spawnedMap.end();)
	{
		spawnId = it->first;
		monster = it->second;

		if(monster->isRemoved())
		{
			if(spawnId != 0)
				spawnMap[spawnId].lastSpawn = OTSYS_TIME();

			monster->releaseThing2();
			spawnedMap.erase(it++);
		}
		else if(!isInSpawnZone(monster->getPosition()) && spawnId != 0)
		{
			spawnedMap.insert(spawned_pair(0, monster));
			spawnedMap.erase(it++);
		}
		else
			++it;
	}

	uint32_t spawnCount = 0;
	for(SpawnMap::iterator it = spawnMap.begin(); it != spawnMap.end(); ++it)
	{
		spawnId = it->first;
		spawnBlock_t& sb = it->second;

		if(spawnedMap.count(spawnId) == 0)
		{
			if(OTSYS_TIME() >= sb.lastSpawn + sb.interval)
			{
				if(findPlayer(sb.pos))
				{
					sb.lastSpawn = OTSYS_TIME();
					continue;
				}

				spawnMonster(spawnId, sb.mType, sb.pos, sb.direction);

				++spawnCount;
				if(spawnCount >= (uint32_t)g_config.getNumber(ConfigManager::RATE_SPAWN))
					break;
			}
		}
	}

	if(spawnedMap.size() < spawnMap.size())
		checkSpawnEvent = Scheduler::getScheduler().addEvent(createSchedulerTask(getInterval(), boost::bind(&Spawn::checkSpawn, this)));
#ifdef __DEBUG_SPAWN__
	else
		std::cout << "[Notice] Spawn::checkSpawn stopped " << this << std::endl;
#endif
}

bool Spawn::addMonster(const std::string& _name, const Position& _pos, Direction _dir, uint32_t _interval)
{
	MonsterType* mType = g_monsters.getMonsterType(_name);
	if(!mType)
	{
		std::cout << "[Spawn::addMonster] Can not find " << _name << std::endl;
		return false;
	}

	if(_interval < interval)
		interval = _interval;

	spawnBlock_t sb;
	sb.mType = mType;
	sb.pos = _pos;
	sb.direction = _dir;
	sb.interval = _interval;
	sb.lastSpawn = 0;

	uint32_t spawnId = (int32_t)spawnMap.size() + 1;
	spawnMap[spawnId] = sb;
	return true;
}

void Spawn::removeMonster(Monster* monster)
{
	for(SpawnedMap::iterator it = spawnedMap.begin(); it != spawnedMap.end(); ++it)
	{
		if(it->second == monster)
		{
			monster->releaseThing2();
			spawnedMap.erase(it);
			break;
		}
	}
}

void Spawn::stopEvent()
{
	if(checkSpawnEvent != 0)
	{
		Scheduler::getScheduler().stopEvent(checkSpawnEvent);
		checkSpawnEvent = 0;
	}
}
