//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Lua script interface
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

#include <string>
#include <iostream>
#include <iomanip>

#include "luascript.h"
#include "player.h"
#include "item.h"
#include "game.h"
#include "house.h"
#include "housetile.h"
#include "status.h"
#include "combat.h"
#include "spells.h"
#include "condition.h"
#include "monsters.h"
#include "baseevents.h"
#include "iologindata.h"
#include "configmanager.h"
#include "town.h"
#include "vocation.h"
#include "teleport.h"
#include "ban.h"
#include "databasemanager.h"

extern Game g_game;
extern Monsters g_monsters;
extern ConfigManager g_config;
extern Vocations g_vocations;
extern Spells* g_spells;

enum{
	EVENT_ID_LOADING = 1,
	EVENT_ID_USER = 1000,
};

ScriptEnvironment::ThingMap ScriptEnvironment::m_globalMap;

ScriptEnvironment::AreaMap ScriptEnvironment::m_areaMap;
uint32_t ScriptEnvironment::m_lastAreaId = 0;

ScriptEnvironment::CombatMap ScriptEnvironment::m_combatMap;
uint32_t ScriptEnvironment::m_lastCombatId = 0;

ScriptEnvironment::ConditionMap ScriptEnvironment::m_conditionMap;
uint32_t ScriptEnvironment::m_lastConditionId = 0;

ScriptEnvironment::DBResultMap ScriptEnvironment::m_tempResults;
uint32_t ScriptEnvironment::m_lastResultId = 0;

ScriptEnvironment::StorageMap ScriptEnvironment::m_globalStorageMap;

ScriptEnvironment::TempItemListMap ScriptEnvironment::m_tempItems;

ScriptEnvironment::ScriptEnvironment()
{
	resetEnv();
	m_lastUID = 70000;
}

ScriptEnvironment::~ScriptEnvironment()
{
	resetEnv();

	for(CombatMap::iterator it = m_combatMap.begin(); it != m_combatMap.end(); ++it)
		delete it->second;
	
	m_combatMap.clear();
	
	for(AreaMap::iterator it = m_areaMap.begin(); it != m_areaMap.end(); ++it)
		delete it->second;
	
	m_areaMap.clear();
}

void ScriptEnvironment::resetEnv()
{
	m_scriptId = 0;
	m_callbackId = 0;
	m_timerEvent = false;
	m_interface = NULL;
	m_localMap.clear();

	for(TempItemListMap::iterator mit = m_tempItems.begin(); mit != m_tempItems.end(); ++mit)
	{
		ItemList& itemList = mit->second;
		for(ItemList::iterator it = itemList.begin(); it != itemList.end(); ++it)
		{
			if((*it)->getParent() == VirtualCylinder::virtualCylinder)
				g_game.FreeThing(*it);
		}
	}
	m_tempItems.clear();

	if(!m_tempResults.empty())
	{
		Database* db = Database::getInstance();
		for(DBResultMap::iterator it = m_tempResults.begin(); it != m_tempResults.end(); ++it)
		{
			if(it->second)
				db->freeResult(it->second);
		}
	}
	m_tempResults.clear();

	m_realPos.x = 0;
	m_realPos.y = 0;
	m_realPos.z = 0;
}

bool ScriptEnvironment::saveGameState()
{
	if(!g_config.getBool(ConfigManager::SAVE_GLOBAL_STORAGE))
		return true;

	Database* db = Database::getInstance();

	DBQuery query;
	if(!db->executeQuery("DELETE FROM `global_storage`;"))
		return false;

	DBInsert stmt(db);
	stmt.setQuery("INSERT INTO `global_storage` (`key`, `value`) VALUES ");
	for(StorageMap::const_iterator it = m_globalStorageMap.begin(); it != m_globalStorageMap.end(); ++it)
	{
		query << it->first << "," << it->second;
		if(!stmt.addRow(query))
			return false;
	}
	return stmt.execute();
}

bool ScriptEnvironment::loadGameState()
{
	if(!g_config.getBool(ConfigManager::SAVE_GLOBAL_STORAGE))
		return true;

	Database* db = Database::getInstance();

	DBQuery query;
	DBResult* result;
	query << "SELECT `key`, `value` FROM `global_storage`;";
	if((result = db->storeQuery(query.str())))
	{
		do
		{
			m_globalStorageMap[result->getDataInt("key")] = result->getDataInt("value");
		}
		while(result->next());
		db->freeResult(result);
	}
	return true;
}

bool ScriptEnvironment::setCallbackId(int32_t callbackId, LuaScriptInterface* scriptInterface)
{
	if(m_callbackId == 0)
	{
		m_callbackId = callbackId;
		m_interface = scriptInterface;
		return true;
	}
	else
	{
		//nested callbacks are not allowed
		if(m_interface)
			m_interface->reportErrorFunc("Nested callbacks!");

		return false;
	}
}

void ScriptEnvironment::getEventInfo(int32_t& scriptId, std::string& desc, LuaScriptInterface*& scriptInterface, int32_t& callbackId, bool& timerEvent)
{
	scriptId = m_scriptId;
	desc = m_eventdesc;
	scriptInterface = m_interface;
	callbackId = m_callbackId;
	timerEvent = m_timerEvent;
}

void ScriptEnvironment::addUniqueThing(Thing* thing)
{
	Item* item = thing->getItem();
	if(item && item->getUniqueId() != 0)
	{
		int32_t uid = item->getUniqueId();

		Thing* tmp = m_globalMap[uid];
		if(!tmp)
			m_globalMap[uid] = thing;
		else
			std::clog << "Duplicate uniqueId " << uid << std::endl;
	}
}

void ScriptEnvironment::removeUniqueThing(Thing* thing)
{
	Item* item = thing->getItem();
	if(item && item->getUniqueId() != 0)
	{
		int32_t uid = item->getUniqueId();
		ThingMap::iterator it = m_globalMap.find(uid);
		if(it != m_globalMap.end())
			m_globalMap.erase(it);
	}
}

uint32_t ScriptEnvironment::addThing(Thing* thing)
{
	if(thing && !thing->isRemoved())
	{
		ThingMap::iterator it;
		for(it = m_localMap.begin(); it != m_localMap.end(); ++it)
		{
			if(it->second == thing)
				return it->first;
		}

		uint32_t newUid;
		if(Creature* creature = thing->getCreature())
			newUid = creature->getID();
		else
		{
			if(Item* item = thing->getItem())
			{
				uint32_t uid = item->getUniqueId();
				if(uid && item->getTile() == item->getParent())
				{
					m_localMap[uid] = thing;
					return uid;
				}
			}

			++m_lastUID;
			if(m_lastUID < 70000)
				m_lastUID = 70000;
		
			while(m_localMap[m_lastUID])
				++m_lastUID;

			newUid = m_lastUID;
		}

		m_localMap[newUid] = thing;
		return newUid;
	}
	else
		return 0;
}

void ScriptEnvironment::insertThing(uint32_t uid, Thing* thing)
{
	if(!m_localMap[uid])
		m_localMap[uid] = thing;
	else
		std::clog << std::endl << "Lua Script Error: Thing uid already taken.";
}

Thing* ScriptEnvironment::getThingByUID(uint32_t uid)
{
	Thing* tmp = m_localMap[uid];
	if(tmp && !tmp->isRemoved())
		return tmp;

	tmp = m_globalMap[uid];
	if(tmp && !tmp->isRemoved())
		return tmp;

	if(uid >= 0x10000000)
	{
		tmp = g_game.getCreatureByID(uid);
		if(tmp && !tmp->isRemoved())
		{
			m_localMap[uid] = tmp;
			return tmp;
		}
	}
	return NULL;
}

Item* ScriptEnvironment::getItemByUID(uint32_t uid)
{
	Thing* tmp = getThingByUID(uid);
	if(tmp)
	{
		if(Item* item = tmp->getItem())
			return item;
	}
	return NULL;
}

Container* ScriptEnvironment::getContainerByUID(uint32_t uid)
{
	Item* tmp = getItemByUID(uid);
	if(tmp)
	{
		if(Container* container = tmp->getContainer())
			return container;
	}
	return NULL;
}

Creature* ScriptEnvironment::getCreatureByUID(uint32_t uid)
{
	Thing* tmp = getThingByUID(uid);
	if(tmp)
	{
		if(Creature* creature = tmp->getCreature())
			return creature;
	}
	return NULL;
}

Player* ScriptEnvironment::getPlayerByUID(uint32_t uid)
{
	Thing* tmp = getThingByUID(uid);
	if(tmp)
	{
		if(Creature* creature = tmp->getCreature())
		{
			if(Player* player = creature->getPlayer())
				return player;
		}
	}
	return NULL;
}

void ScriptEnvironment::removeItemByUID(uint32_t uid)
{
	ThingMap::iterator it;	
	it = m_localMap.find(uid);
	if(it != m_localMap.end())
		m_localMap.erase(it);

	it = m_globalMap.find(uid);
	if(it != m_globalMap.end())
		m_globalMap.erase(it);
}

uint32_t ScriptEnvironment::addCombatArea(AreaCombat* area)
{
	uint32_t newAreaId = m_lastAreaId + 1;
	m_areaMap[newAreaId] = area;
	
	m_lastAreaId++;
	return newAreaId;
}

AreaCombat* ScriptEnvironment::getCombatArea(uint32_t areaId)
{
	AreaMap::const_iterator it = m_areaMap.find(areaId);
	if(it != m_areaMap.end())
		return it->second;

	return NULL;
}

uint32_t ScriptEnvironment::addCombatObject(Combat* combat)
{
	uint32_t newCombatId = m_lastCombatId + 1;
	m_combatMap[newCombatId] = combat;
	
	m_lastCombatId++;
	return newCombatId;
}

Combat* ScriptEnvironment::getCombatObject(uint32_t combatId)
{
	CombatMap::iterator it = m_combatMap.find(combatId);
	if(it != m_combatMap.end())
		return it->second;

	return NULL;
}

uint32_t ScriptEnvironment::addConditionObject(Condition* condition)
{
	uint32_t newConditionId = m_lastConditionId + 1;
	m_conditionMap[newConditionId] = condition;
	
	m_lastConditionId++;
	return m_lastConditionId;
}

Condition* ScriptEnvironment::getConditionObject(uint32_t conditionId)
{
	ConditionMap::iterator it = m_conditionMap.find(conditionId);
	if(it != m_conditionMap.end())
		return it->second;

	return NULL;
}

void ScriptEnvironment::addTempItem(ScriptEnvironment* env, Item* item)
{
	m_tempItems[env].push_back(item);
}

void ScriptEnvironment::removeTempItem(Item* item)
{
	for(TempItemListMap::iterator mit = m_tempItems.begin(); mit != m_tempItems.end(); ++mit)
	{
		ItemList& itemList = mit->second;
		ItemList::iterator it = std::find(itemList.begin(), itemList.end(), item);
		if(it != itemList.end())
		{
			itemList.erase(it);
			break;
		}
	}
}

DBResult* ScriptEnvironment::getResultByID(uint32_t id)
{
	DBResultMap::iterator it = m_tempResults.find(id);
	if(it != m_tempResults.end())
		return it->second;

	return NULL;
}

uint32_t ScriptEnvironment::addResult(DBResult* res)
{
	uint32_t newResultId = m_lastResultId + 1;
	m_tempResults[newResultId] = res;

	m_lastResultId++;
	return m_lastResultId;
}

bool ScriptEnvironment::removeResult(uint32_t id)
{
	DBResultMap::iterator it = m_tempResults.find(id);
	if(it == m_tempResults.end())
		return false;

	if(it->second)
		Database::getInstance()->freeResult(it->second);

	m_tempResults.erase(it);
	return true;
}

void ScriptEnvironment::addGlobalStorageValue(const uint32_t key, const int32_t value)
{
	m_globalStorageMap[key] = value;
}

bool ScriptEnvironment::getGlobalStorageValue(const uint32_t key, int32_t& value) const
{
	StorageMap::const_iterator it;
	it = m_globalStorageMap.find(key);
	if(it != m_globalStorageMap.end())
	{
		value = it->second;
		return true;
	}
	else
	{
		value = 0;
		return false;
	}
}

std::string LuaScriptInterface::getErrorDesc(ErrorCode_t code)
{
	switch(code)
	{
		case LUA_ERROR_PLAYER_NOT_FOUND:
			return "Player not found";
			break;
		case LUA_ERROR_CREATURE_NOT_FOUND:
			return "Creature not found";
			break;
		case LUA_ERROR_ITEM_NOT_FOUND:
			return "Item not found";
			break;
		case LUA_ERROR_THING_NOT_FOUND:
			return "Thing not found";
			break;
		case LUA_ERROR_TILE_NOT_FOUND:
			return "Tile not found";
			break;
		case LUA_ERROR_HOUSE_NOT_FOUND:
			return "House not found";
			break;
		case LUA_ERROR_COMBAT_NOT_FOUND:
			return "Combat not found";
			break;
		case LUA_ERROR_CONDITION_NOT_FOUND:
			return "Condition not found";
			break;
		case LUA_ERROR_AREA_NOT_FOUND:
			return "Area not found";
			break;
		case LUA_ERROR_CONTAINER_NOT_FOUND:
			return "Container not found";
			break;
		case LUA_ERROR_VARIANT_NOT_FOUND:
			return "Variant not found";
			break;
		case LUA_ERROR_VARIANT_UNKNOWN:
			return "Unknown variant type";
			break;
		case LUA_ERROR_SPELL_NOT_FOUND:
			return "Spell not found";
			break;
		default:
			return "Wrong error code!";
			break;
	}
}

ScriptEnvironment LuaScriptInterface::m_scriptEnv[16];
int32_t LuaScriptInterface::m_scriptEnvIndex = -1;

LuaScriptInterface::LuaScriptInterface(std::string interfaceName)
{
	m_luaState = NULL;
	m_interfaceName = interfaceName;
	m_lastEventTimerId = 1000;
}

LuaScriptInterface::~LuaScriptInterface()
{
	for(LuaTimerEvents::iterator it = m_timerEvents.begin(); it != m_timerEvents.end(); ++it)
		Scheduler::getScheduler().stopEvent(it->second.eventId);

	closeState();
}
	
bool LuaScriptInterface::reInitState()
{
	closeState();
	return initState();
}

void LuaScriptInterface::dumpLuaStack()
{
	int32_t a = lua_gettop(m_luaState);
	std::clog << "stack size: " << a << std::endl;
	for(int32_t i = 1; i <= a ; ++i)
		std::clog << lua_typename(m_luaState, lua_type(m_luaState,-i)) << " " << lua_topointer(m_luaState, -i) << std::endl;
}

/// Same as lua_pcall, but adds stack trace to error strings in called function.
int32_t LuaScriptInterface::protectedCall(lua_State* L, int32_t nargs, int32_t nresults)
{
	int error_index = lua_gettop(L) - nargs;
	lua_pushcfunction(L, luaErrorHandler);
	lua_insert(L, error_index);

	int ret = lua_pcall(L, nargs, nresults, error_index);
	lua_remove(L, error_index);
	return ret;
}

int32_t LuaScriptInterface::loadFile(const std::string& file, Npc* npc /* = NULL*/)
{
	//loads file as a chunk at stack top
	int32_t ret = luaL_loadfile(m_luaState, file.c_str());
	if(ret != 0)
	{
		m_lastLuaError = popString(m_luaState);
		return -1;
	}

	//check that it is loaded as a function
	if(lua_isfunction(m_luaState, -1) == 0)
		return -1;

	m_loadingFile = file;
	reserveScriptEnv();

	ScriptEnvironment* env = this->getScriptEnv();
	env->setScriptId(EVENT_ID_LOADING, this);
	env->setNpc(npc);

	//execute it
	ret = protectedCall(m_luaState, 0, 0);
	if(ret != 0)
	{
		reportError(NULL, popString(m_luaState));
		this->releaseScriptEnv();
		return -1;
	}

	this->releaseScriptEnv();
	return 0;
}

int32_t LuaScriptInterface::loadBuffer(const std::string& text, Npc* npc /* = NULL*/)
{
	//loads file as a chunk at stack top
	const char* buffer = text.c_str();
	int ret = luaL_loadbuffer(m_luaState, buffer, strlen(buffer), "loadBuffer");
	if(ret != 0)
	{
		m_lastLuaError = popString(m_luaState);
		reportError(NULL, m_lastLuaError);
		return -1;
	}

	//check that it is loaded as a function
	if(lua_isfunction(m_luaState, -1) == 0)
		return -1;

	m_loadingFile = "loadBuffer";
	reserveScriptEnv();

	ScriptEnvironment* env = this->getScriptEnv();
	env->setScriptId(EVENT_ID_LOADING, this);
	env->setNpc(npc);

	//execute it
	ret = protectedCall(m_luaState, 0, 0);
	if(ret != 0)
	{
		reportError(NULL, std::string(popString(m_luaState)));
		this->releaseScriptEnv();
		return -1;
	}

	this->releaseScriptEnv();
	return 0;
}

int32_t LuaScriptInterface::getEvent(const std::string& eventName)
{
	//get our events table
	lua_getfield(m_luaState, LUA_REGISTRYINDEX, "EVENTS");
	if(lua_istable(m_luaState, -1) == 0)
	{
		lua_pop(m_luaState, 1);
		return -1;
	}

	//get current event function pointer
	lua_getglobal(m_luaState, eventName.c_str());
	if(lua_isfunction(m_luaState, -1) == 0)
	{
		lua_pop(m_luaState, 1);
		return -1;
	}

	//save in our events table
	lua_pushnumber(m_luaState, m_runningEventId);
	lua_pushvalue(m_luaState, -2);
	lua_rawset(m_luaState, -4);
	lua_pop(m_luaState, 2);

	//reset global value of this event
	lua_pushnil(m_luaState);
	lua_setglobal(m_luaState, eventName.c_str());

	m_cacheFiles[m_runningEventId] = m_loadingFile + ":" + eventName;
	++m_runningEventId;
	return m_runningEventId - 1;
}

const std::string& LuaScriptInterface::getFileById(int32_t scriptId)
{
	const static std::string unk = "(Unknown scriptfile)";
	if(scriptId != EVENT_ID_LOADING)
	{
		ScriptsCache::iterator it = m_cacheFiles.find(scriptId);
		if(it != m_cacheFiles.end())
			return it->second;
		else
			return unk;
	}
	else
		return m_loadingFile;
}

std::string LuaScriptInterface::getStackTrace(const std::string& error_desc)
{
	lua_State* L = m_luaState;
	lua_getfield(L, LUA_GLOBALSINDEX, "debug");
	if(!lua_istable(L, -1))
	{
		lua_pop(L, 1);
		return error_desc;
	}

	lua_getfield(L, -1, "traceback");
	if(!lua_isfunction(L, -1))
	{
		lua_pop(L, 1);
		return error_desc;
	}

	lua_pushstring(L, error_desc.c_str());
	lua_call(L, 1, 1);
	std::string trace(lua_tostring(L, -1));
	lua_pop(L, 1);
	return trace;
}

void LuaScriptInterface::reportError(const char* function, const std::string& error_desc)
{
	ScriptEnvironment* env = getScriptEnv();
	int32_t scriptId;
	int32_t callbackId;
	bool timerEvent;
	std::string event_desc;
	LuaScriptInterface* scriptInterface;
	env->getEventInfo(scriptId, event_desc, scriptInterface, callbackId, timerEvent);
	
	std::clog << std::endl << "Lua Script Error: ";
	if(scriptInterface)
	{
		std::clog << "[" << scriptInterface->getInterfaceName() << "] " << std::endl;
		if(timerEvent)
			std::clog << "in a timer event called from: " << std::endl;
		if(callbackId)
			std::clog << "in callback: " << scriptInterface->getFileById(callbackId) << std::endl;
		std::clog << scriptInterface->getFileById(scriptId) << std::endl;
	}
	std::clog << event_desc << std::endl;
	if(function)
		std::clog << function << "(). ";
	std::clog << error_desc << std::endl;
}

bool LuaScriptInterface::pushFunction(int32_t functionId)
{
	lua_getfield(m_luaState, LUA_REGISTRYINDEX, "EVENTS");
	if(lua_istable(m_luaState, -1) != 0)
	{
		lua_pushnumber(m_luaState, functionId);
		lua_rawget(m_luaState, -2);
		lua_remove(m_luaState, -2);
		if(lua_isfunction(m_luaState, -1) != 0)
			return true;
	}
	return false;
}

bool LuaScriptInterface::initState()
{
	m_luaState = luaL_newstate();
	if(!m_luaState)
		return false;
	
	luaL_openlibs(m_luaState);

	registerFunctions();

	if(loadFile("data/global.lua") == -1)
		std::clog << "Warning: [LuaScriptInterface::initState] Can not load data/global.lua." << std::endl;
	
	lua_newtable(m_luaState);
	lua_setfield(m_luaState, LUA_REGISTRYINDEX, "EVENTS");
	
	m_runningEventId = EVENT_ID_USER;
	return true;
}

bool LuaScriptInterface::closeState()
{
	if(m_luaState)
	{
		m_cacheFiles.clear();

		LuaTimerEvents::iterator it, end;
		for(it = m_timerEvents.begin(), end = m_timerEvents.end(); it != end; ++it)
		{
			LuaTimerEventDesc& timerEventDesc = it->second;
			for(std::list<int32_t>::iterator lt = timerEventDesc.parameters.begin(), lend = timerEventDesc.parameters.end(); lt != lend; ++lt)
				luaL_unref(m_luaState, LUA_REGISTRYINDEX, *lt);

			timerEventDesc.parameters.clear();

			luaL_unref(m_luaState, LUA_REGISTRYINDEX, timerEventDesc.function);
		}
		m_timerEvents.clear();

		lua_close(m_luaState);
	}
	return true;
}

void LuaScriptInterface::executeTimerEvent(uint32_t eventIndex)
{
	LuaTimerEvents::iterator it = m_timerEvents.find(eventIndex);
	if(it != m_timerEvents.end())
	{
		LuaTimerEventDesc& timerEventDesc = it->second;

		//push function
		lua_rawgeti(m_luaState, LUA_REGISTRYINDEX, timerEventDesc.function);

		//push parameters
		for(std::list<int32_t>::reverse_iterator rt = timerEventDesc.parameters.rbegin(), rend = timerEventDesc.parameters.rend(); rt != rend; ++rt)
			lua_rawgeti(m_luaState, LUA_REGISTRYINDEX, *rt);

		//call the function
		if(reserveScriptEnv())
		{
			ScriptEnvironment* env = getScriptEnv();
			env->setTimerEvent();
			env->setScriptId(timerEventDesc.scriptId, this);
			callFunction(timerEventDesc.parameters.size());
			releaseScriptEnv();
		}
		else
			std::clog << "[Error] Call stack overflow. LuaScriptInterface::executeTimerEvent" << std::endl;

		//free resources
		for(std::list<int32_t>::iterator lt = timerEventDesc.parameters.begin(), end = timerEventDesc.parameters.end(); lt != end; ++lt)
			luaL_unref(m_luaState, LUA_REGISTRYINDEX, *lt);

		timerEventDesc.parameters.clear();

		luaL_unref(m_luaState, LUA_REGISTRYINDEX, timerEventDesc.function);
		m_timerEvents.erase(it);
	}
}

int32_t LuaScriptInterface::luaErrorHandler(lua_State* L)
{
	lua_getfield(L, LUA_GLOBALSINDEX, "debug");
	if(!lua_istable(L, -1))
	{
		lua_pop(L, 1);
		return 1;
	}
	lua_getfield(L, -1, "traceback");
	if(!lua_isfunction(L, -1))
	{
		lua_pop(L, 2);
		return 1;
	}
	lua_pushvalue(L, 1);
	lua_pushinteger(L, 2);
	lua_call(L, 2, 1);
	return 1;
}

int32_t LuaScriptInterface::callFunction(uint32_t nParams)
{
	bool result = false;
	int32_t size0 = lua_gettop(m_luaState);
	int32_t ret = protectedCall(m_luaState, nParams, 1);
	if(ret != 0)
		LuaScriptInterface::reportError(NULL, LuaScriptInterface::popString(m_luaState));
	else
		result = LuaScriptInterface::popBoolean(m_luaState);

	if((lua_gettop(m_luaState) + (int)nParams + 1) != size0)
		LuaScriptInterface::reportError(NULL, "Stack size changed!");

	return result;
}

void LuaScriptInterface::pushVariant(lua_State* L, const LuaVariant& var)
{
	lua_newtable(L);
	setField(L, "type", var.type);

	switch(var.type)
	{
		case VARIANT_NUMBER: setField(L, "number", var.number); break;
		case VARIANT_STRING: setField(L, "string", var.text); break;
		case VARIANT_TARGETPOSITION:
		case VARIANT_POSITION:
		{
			lua_pushstring(L, "pos");
			pushPosition(L, var.pos);
			lua_settable(L, -3);
			break;
		}
		case VARIANT_NONE:
			break;
 	}
}

void LuaScriptInterface::pushThing(lua_State* L, Thing* thing, uint32_t thingid)
{
	lua_newtable(L);
	if(thing && thing->getItem())
	{
		const Item* item = thing->getItem();
		setField(L, "uid", thingid);
		setField(L, "itemid", item->getID());

		if(item->hasSubType())
			setField(L, "type", item->getSubType());
		else
			setField(L, "type", 0);

		setField(L, "actionid", item->getActionId());
	}
	else if(thing && thing->getCreature())
	{
		const Creature* creature = thing->getCreature();
		setField(L, "uid", thingid);
		setField(L, "itemid", 1);
		char type;
		if(creature->getPlayer())
			type = 1;
		else if(creature->getMonster())
			type = 2;
		else
			type = 3; //npc
		setField(L, "type", type);
		setField(L, "actionid", 0);
	}
	else
	{
		setField(L, "uid", 0);
		setField(L, "itemid", 0);
		setField(L, "type", 0);
		setField(L, "actionid", 0);
	}
}

void LuaScriptInterface::pushPosition(lua_State* L, const PositionEx& position)
{
	lua_newtable(L);
	setField(L, "z", position.z);
	setField(L, "y", position.y);
	setField(L, "x", position.x);
	setField(L, "stackpos", position.stackpos);
}

void LuaScriptInterface::pushPosition(lua_State* L, const Position& position, uint32_t stackpos)
{
	lua_newtable(L);
	setField(L, "z", position.z);
	setField(L, "y", position.y);
	setField(L, "x", position.x);
	setField(L, "stackpos", stackpos);
}

void LuaScriptInterface::pushCallback(lua_State* L, int32_t callback)
{
	lua_rawgeti(L, LUA_REGISTRYINDEX, callback);
}

LuaVariant LuaScriptInterface::popVariant(lua_State* L)
{
	uint32_t type = getField(L, "type");

	LuaVariant var;
	var.type = (LuaVariantType_t)type;

	switch(type)
	{
		case VARIANT_NUMBER:
		{
			var.number = getFieldU32(L, "number");
			break;
		}

		case VARIANT_STRING:
		{
			var.text = getField(L, "string");
			break;
		}

		case VARIANT_POSITION:
		case VARIANT_TARGETPOSITION:
		{
			lua_pushstring(L, "pos");
			lua_gettable(L, -2);
			popPosition(L, var.pos);
			break;
		}

		default:
		{
			var.type = VARIANT_NONE;
			break;
		}
	}

	lua_pop(L, 1); //table

	return var;
}

void LuaScriptInterface::popPosition(lua_State* L, PositionEx& position)
{
	position.z = getField(L, "z");
	position.y = getField(L, "y");
	position.x = getField(L, "x");
	position.stackpos = getField(L, "stackpos");

	lua_pop(L, 1); //table
}

void LuaScriptInterface::popPosition(lua_State* L, Position& position, uint32_t& stackpos)
{
	position.z = getField(L, "z");
	position.y = getField(L, "y");
	position.x = getField(L, "x");
	stackpos = getField(L, "stackpos");
	lua_pop(L, 1); //table
}

uint32_t LuaScriptInterface::popNumber(lua_State* L)
{
	lua_pop(L,1);
	return (uint32_t)lua_tonumber(L, 0);
}

double LuaScriptInterface::popFloatNumber(lua_State* L)
{
	lua_pop(L,1);
	return (double)lua_tonumber(L, 0);
}

std::string LuaScriptInterface::popString(lua_State* L)
{
	if(lua_gettop(L) == 0)
		return "";

	std::string str;
	size_t len;
	const char* c_str = lua_tolstring(L, -1, &len);
	if(c_str && len > 0)
		str.assign(c_str, len);

	lua_pop(L, 1);
	return str;
}

int32_t LuaScriptInterface::popCallback(lua_State* L)
{
	return luaL_ref(L, LUA_REGISTRYINDEX);
}

bool LuaScriptInterface::popBoolean(lua_State* L)
{
	if(lua_gettop(L) == 0)
		return false;

	bool value = lua_toboolean(L, -1) != 0;
	lua_pop(L, 1);
	return value;
}

void LuaScriptInterface::setField(lua_State* L, const char* index, int32_t val)
{
	lua_pushstring(L, index);
	lua_pushnumber(L, (double)val);
	lua_settable(L, -3);
}

void LuaScriptInterface::setField(lua_State* L, const char* index, const std::string& val)
{
	lua_pushstring(L, index);
	lua_pushstring(L, val.c_str());
	lua_settable(L, -3);
}

void LuaScriptInterface::setFieldBool(lua_State* L, const char* index, bool val)
{
	lua_pushstring(L, index);
	lua_pushboolean(L, val);
	lua_settable(L, -3);
}

int32_t LuaScriptInterface::getField(lua_State* L, const char* key)
{
	int32_t result;
	lua_pushstring(L, key);
	lua_gettable(L, -2); // get table[key]
	result = (int32_t)lua_tonumber(L, -1);
	lua_pop(L, 1); // remove number and key
	return result;
}

uint32_t LuaScriptInterface::getFieldU32(lua_State* L, const char* key)
{
	uint32_t result;
	lua_pushstring(L, key);
	lua_gettable(L, -2); // get table[key]
	result = (uint32_t)lua_tonumber(L, -1);
	lua_pop(L, 1); // remove number and key
	return result;
}

bool LuaScriptInterface::getFieldBool(lua_State* L, const char* key)
{
	bool result;
	lua_pushstring(L, key);
	lua_gettable(L, -2); // get table[key]
	result = (lua_toboolean(L, -1) == 1);
	lua_pop(L, 1); // remove number and key
	return result;
}

std::string LuaScriptInterface::getFieldString(lua_State* L, const char* key)
{
	std::string result;
	lua_pushstring(L, key);
	lua_gettable(L, -2); // get table[key]
	result = lua_tostring(L, -1);
	lua_pop(L, 1); // remove number and key
	return result;
}

void LuaScriptInterface::registerFunctions()
{
	//lua_register(L, "name", C_function);
	
	//getPlayerFood(cid)
	lua_register(m_luaState, "getPlayerFood", LuaScriptInterface::luaGetPlayerFood);

	//getCreatureHealth(cid)
	lua_register(m_luaState, "getCreatureHealth", LuaScriptInterface::luaGetCreatureHealth);

	//getCreatureMaxHealth(cid)
	lua_register(m_luaState, "getCreatureMaxHealth", LuaScriptInterface::luaGetCreatureMaxHealth);

	//getCreatureMana(cid)
	lua_register(m_luaState, "getCreatureMana", LuaScriptInterface::luaGetCreatureMana);

	//getCreatureMaxMana(cid)
	lua_register(m_luaState, "getCreatureMaxMana", LuaScriptInterface::luaGetCreatureMaxMana);

	//getCreatureLookDirection(cid)
	lua_register(m_luaState, "getCreatureLookDirection", LuaScriptInterface::luaGetCreatureLookDirection);

	//getPlayerLevel(cid)
	lua_register(m_luaState, "getPlayerLevel", LuaScriptInterface::luaGetPlayerLevel);

	//getPlayerMagLevel(cid)
	lua_register(m_luaState, "getPlayerMagLevel", LuaScriptInterface::luaGetPlayerMagLevel);

	//getPlayerName(cid)	
	lua_register(m_luaState, "getPlayerName", LuaScriptInterface::luaGetPlayerName);

	//getPlayerAccess(cid)	
	lua_register(m_luaState, "getPlayerAccess", LuaScriptInterface::luaGetPlayerAccess);

	//getPlayerSkill(cid, skillid)
	lua_register(m_luaState, "getPlayerSkill", LuaScriptInterface::luaGetPlayerSkill);

	//getPlayerTown(cid)
	lua_register(m_luaState, "getPlayerTown", LuaScriptInterface::luaGetPlayerTown);

	//getPlayerVocation(cid)
	lua_register(m_luaState, "getPlayerVocation", LuaScriptInterface::luaGetPlayerVocation);

	//getPlayerItemCount(cid,itemid)
	lua_register(m_luaState, "getPlayerItemCount", LuaScriptInterface::luaGetPlayerItemCount);

	//getPlayerSoul(cid)
	lua_register(m_luaState, "getPlayerSoul", LuaScriptInterface::luaGetPlayerSoul);

	//getPlayerFreeCap(cid)
	lua_register(m_luaState, "getPlayerFreeCap", LuaScriptInterface::luaGetPlayerFreeCap);

	//getPlayerLight(cid)
	lua_register(m_luaState, "getPlayerLight", LuaScriptInterface::luaGetPlayerLight);

	//getPlayerSlotItem(cid, slot)
	lua_register(m_luaState, "getPlayerSlotItem", LuaScriptInterface::luaGetPlayerSlotItem);

	//getPlayerItemById(cid, deepSearch, itemId, <optional> subType)
	lua_register(m_luaState, "getPlayerItemById", LuaScriptInterface::luaGetPlayerItemById);

	//getPlayerDepotItems(cid, depotid)
	lua_register(m_luaState, "getPlayerDepotItems", LuaScriptInterface::luaGetPlayerDepotItems);

	//getPlayerGuildId(cid)
	lua_register(m_luaState, "getPlayerGuildId", LuaScriptInterface::luaGetPlayerGuildId);

	//getPlayerGuildLevel(cid)
	lua_register(m_luaState, "getPlayerGuildLevel", LuaScriptInterface::luaGetPlayerGuildLevel);

	//getPlayerGuildName(cid)
	lua_register(m_luaState, "getPlayerGuildName", LuaScriptInterface::luaGetPlayerGuildName);

	//getPlayerGuildRank(cid)
	lua_register(m_luaState, "getPlayerGuildRank", LuaScriptInterface::luaGetPlayerGuildRank);

	//getPlayerGuildNick(cid)
	lua_register(m_luaState, "getPlayerGuildNick", LuaScriptInterface::luaGetPlayerGuildNick);

	//getPlayerSex(cid)
	lua_register(m_luaState, "getPlayerSex", LuaScriptInterface::luaGetPlayerSex);

	//getPlayerLookDir(cid)
	lua_register(m_luaState, "getPlayerLookDir", LuaScriptInterface::luaGetPlayerLookDir);

	//getPlayerGUID(cid)
	lua_register(m_luaState, "getPlayerGUID", LuaScriptInterface::luaGetPlayerGUID);

	//getPlayerFlagValue(cid, flag)
	lua_register(m_luaState, "getPlayerFlagValue", LuaScriptInterface::luaGetPlayerFlagValue);

	//getPlayerLossPercent(cid)
	lua_register(m_luaState, "getPlayerLossPercent", LuaScriptInterface::luaGetPlayerLossPercent);

	//getPlayerSkullType(cid)
	lua_register(m_luaState, "getPlayerSkullType", LuaScriptInterface::luaGetPlayerSkullType);

	//getPlayerGroupId(cid)
	lua_register(m_luaState, "getPlayerGroupId", LuaScriptInterface::luaGetPlayerGroupId);

	//setPlayerGroupId(cid, newGroupId)
	lua_register(m_luaState, "setPlayerGroupId", LuaScriptInterface::luaSetPlayerGroupId);

	//playerLearnInstantSpell(cid, name)
	lua_register(m_luaState, "playerLearnInstantSpell", LuaScriptInterface::luaPlayerLearnInstantSpell);

	//canPlayerLearnInstantSpell(cid, name)
	lua_register(m_luaState, "canPlayerLearnInstantSpell", LuaScriptInterface::luaCanPlayerLearnInstantSpell);

	//getPlayerLearnedInstantSpell(cid, name)
	lua_register(m_luaState, "getPlayerLearnedInstantSpell", LuaScriptInterface::luaGetPlayerLearnedInstantSpell);

	//getPlayerInstantSpellCount(cid)
	lua_register(m_luaState, "getPlayerInstantSpellCount", LuaScriptInterface::luaGetPlayerInstantSpellCount);

	//getPlayerInstantSpellInfo(cid, index)
	lua_register(m_luaState, "getPlayerInstantSpellInfo", LuaScriptInterface::luaGetPlayerInstantSpellInfo);

	//getInstantSpellInfoByName(cid, name)
	lua_register(m_luaState, "getInstantSpellInfoByName", LuaScriptInterface::luaGetInstantSpellInfoByName);

	//getInstantSpellWords(name)
	lua_register(m_luaState, "getInstantSpellWords", LuaScriptInterface::luaGetInstantSpellWords);

	//getPlayerStorageValue(uid, valueid)
	lua_register(m_luaState, "getPlayerStorageValue", LuaScriptInterface::luaGetPlayerStorageValue);

	//setPlayerStorageValue(uid, valueid, newvalue)
	lua_register(m_luaState, "setPlayerStorageValue", LuaScriptInterface::luaSetPlayerStorageValue);

	//getGlobalStorageValue(valueid)
	lua_register(m_luaState, "getGlobalStorageValue", LuaScriptInterface::luaGetGlobalStorageValue);

	//setGlobalStorageValue(valueid, newvalue)
	lua_register(m_luaState, "setGlobalStorageValue", LuaScriptInterface::luaSetGlobalStorageValue);

	//getOnlinePlayers()
	lua_register(m_luaState, "getOnlinePlayers", LuaScriptInterface::luaGetOnlinePlayers);

	//getTilePzInfo(pos)
	//1 is pz. 0 no pz.
	lua_register(m_luaState, "getTilePzInfo", LuaScriptInterface::luaGetTilePzInfo);

	//getTileHouseInfo(pos)
	//0 no house. != 0 house id
	lua_register(m_luaState, "getTileHouseInfo", LuaScriptInterface::luaGetTileHouseInfo);

	//getItemRWInfo(uid)
	lua_register(m_luaState, "getItemRWInfo", LuaScriptInterface::luaGetItemRWInfo);

	//getThingfromPos(pos)
	lua_register(m_luaState, "getThingfromPos", LuaScriptInterface::luaGetThingfromPos);

	//getThing(uid)
	lua_register(m_luaState, "getThing", LuaScriptInterface::luaGetThing);

	//queryTileAddThing(uid, pos, <optional> flags)
	lua_register(m_luaState, "queryTileAddThing", LuaScriptInterface::luaQueryTileAddThing);

	//getThingPos(uid)
	lua_register(m_luaState, "getThingPos", LuaScriptInterface::luaGetThingPos);

	//getTileItemById(pos, itemId, <optional> subType)
	lua_register(m_luaState, "getTileItemById", LuaScriptInterface::luaGetTileItemById);

	//getTileItemByType(pos, type)
	lua_register(m_luaState, "getTileItemByType", LuaScriptInterface::luaGetTileItemByType);

	//getTileThingByPos(pos)
	lua_register(m_luaState, "getTileThingByPos", LuaScriptInterface::luaGetTileThingByPos);

	//getTopCreature(pos)
	lua_register(m_luaState, "getTopCreature", LuaScriptInterface::luaGetTopCreature);

	//doRemoveItem(uid, <optional> n)
	lua_register(m_luaState, "doRemoveItem", LuaScriptInterface::luaDoRemoveItem);

	//doPlayerFeed(cid, food)
	lua_register(m_luaState, "doPlayerFeed", LuaScriptInterface::luaDoFeedPlayer);

	//doPlayerSendCancel(cid, text)
	lua_register(m_luaState, "doPlayerSendCancel", LuaScriptInterface::luaDoPlayerSendCancel);

	//doPlayerSendDefaultCancel(cid, ReturnValue)
	lua_register(m_luaState, "doPlayerSendDefaultCancel", LuaScriptInterface::luaDoSendDefaultCancel);

	//doTeleportThing(cid, newpos[, pushmove])
	lua_register(m_luaState, "doTeleportThing", LuaScriptInterface::luaDoTeleportThing);

	//doTransformItem(uid, toitemid, <optional> count/subtype)	
	lua_register(m_luaState, "doTransformItem", LuaScriptInterface::luaDoTransformItem);

	//doCreatureSay(cid, text, type)
	lua_register(m_luaState, "doCreatureSay", LuaScriptInterface::luaDoCreatureSay);

	//doSendMagicEffect(pos, type[, player])
	lua_register(m_luaState, "doSendMagicEffect", LuaScriptInterface::luaDoSendMagicEffect);

	//doSendDistanceShoot(frompos, topos, type)
	lua_register(m_luaState, "doSendDistanceShoot", LuaScriptInterface::luaDoSendDistanceShoot);

	//doChangeTypeItem(uid, newtype)	
	lua_register(m_luaState, "doChangeTypeItem", LuaScriptInterface::luaDoChangeTypeItem);

	//doSetItemActionId(uid, actionid)
	lua_register(m_luaState, "doSetItemActionId", LuaScriptInterface::luaDoSetItemActionId);

	//doSetItemText(uid, text)
	lua_register(m_luaState, "doSetItemText", LuaScriptInterface::luaDoSetItemText);

	//doSetItemSpecialDescription(uid, desc)
	lua_register(m_luaState, "doSetItemSpecialDescription", LuaScriptInterface::luaDoSetItemSpecialDescription);

	//doSendAnimatedText(pos, text, color)
	lua_register(m_luaState, "doSendAnimatedText", LuaScriptInterface::luaDoSendAnimatedText);

	//doPlayerAddSkillTry(cid, skillid, n)
	lua_register(m_luaState, "doPlayerAddSkillTry", LuaScriptInterface::luaDoPlayerAddSkillTry);

	//doCreatureAddHealth(cid, health)
	lua_register(m_luaState, "doCreatureAddHealth", LuaScriptInterface::luaDoCreatureAddHealth);

	//doPlayerAddMana(cid, mana)
	lua_register(m_luaState, "doPlayerAddMana", LuaScriptInterface::luaDoPlayerAddMana);

	//doPlayerAddManaSpent(cid, mana)
	lua_register(m_luaState, "doPlayerAddManaSpent", LuaScriptInterface::luaDoPlayerAddManaSpent);

	//doPlayerAddSoul(cid, soul)
	lua_register(m_luaState, "doPlayerAddSoul", LuaScriptInterface::luaDoPlayerAddSoul);

	//doPlayerAddItem(uid, itemid, <optional> count/subtype)
	//Returns uid of the created item
	lua_register(m_luaState, "doPlayerAddItem", LuaScriptInterface::luaDoPlayerAddItem);

	//doPlayerAddItemEx(cid, uid, <optional: default: 0> canDropOnMap)
	lua_register(m_luaState, "doPlayerAddItemEx", LuaScriptInterface::luaDoPlayerAddItemEx);

	//doPlayerSendTextMessage(cid, MessageClasses, message)
	lua_register(m_luaState, "doPlayerSendTextMessage", LuaScriptInterface::luaDoPlayerSendTextMessage);

	//doPlayerRemoveMoney(cid, money)
	lua_register(m_luaState, "doPlayerRemoveMoney", LuaScriptInterface::luaDoPlayerRemoveMoney);

	//doPlayerAddMoney(cid, money)
	lua_register(m_luaState, "doPlayerAddMoney", LuaScriptInterface::luaDoPlayerAddMoney);

	//doShowTextDialog(cid, itemid, text)
	lua_register(m_luaState, "doShowTextDialog", LuaScriptInterface::luaDoShowTextDialog);

	//doAddMapMark(cid, pos, type, <optional> description)
	lua_register(m_luaState, "doAddMapMark", LuaScriptInterface::luaDoAddMark);

	//doDecayItem(uid)
	lua_register(m_luaState, "doDecayItem", LuaScriptInterface::luaDoDecayItem);

	//doCreateItem(itemid, type/count, pos)
	//Returns uid of the created item, only works on tiles.
	lua_register(m_luaState, "doCreateItem", LuaScriptInterface::luaDoCreateItem);

	//doCreateItemEx(itemid, <optional> count/subtype)
	lua_register(m_luaState, "doCreateItemEx", LuaScriptInterface::luaDoCreateItemEx);

	//doTileAddItemEx(pos, uid)
	lua_register(m_luaState, "doTileAddItemEx", LuaScriptInterface::luaDoTileAddItemEx);

	//doRelocate(pos, posTo)
	//Moves all moveable objects from pos to posTo
	lua_register(m_luaState, "doRelocate", LuaScriptInterface::luaDoRelocate);

	//doCreateTeleport(itemid, topos, createpos)
	lua_register(m_luaState, "doCreateTeleport", LuaScriptInterface::luaDoCreateTeleport);

	//doSummonCreature(name, pos)
	lua_register(m_luaState, "doSummonCreature", LuaScriptInterface::luaDoSummonCreature);

	//doConvinceCreature(cid, target)
	lua_register(m_luaState, "doConvinceCreature", LuaScriptInterface::luaDoConvinceCreature);

	//getMonsterTargetList(cid)
	lua_register(m_luaState, "getMonsterTargetList", LuaScriptInterface::luaGetMonsterTargetList);

	//getMonsterFriendList(cid)
	lua_register(m_luaState, "getMonsterFriendList", LuaScriptInterface::luaGetMonsterFriendList);

	//doSetMonsterTarget(cid, target)
	lua_register(m_luaState, "doSetMonsterTarget", LuaScriptInterface::luaDoSetMonsterTarget);

	//doMonsterChangeTarget(cid)
	lua_register(m_luaState, "doMonsterChangeTarget", LuaScriptInterface::luaDoMonsterChangeTarget);

	//doAddCondition(cid, condition)
	lua_register(m_luaState, "doAddCondition", LuaScriptInterface::luaDoAddCondition);

	//doRemoveCondition(cid, type)
	lua_register(m_luaState, "doRemoveCondition", LuaScriptInterface::luaDoRemoveCondition);

	//doRemoveCreature(cid)
	lua_register(m_luaState, "doRemoveCreature", LuaScriptInterface::luaDoRemoveCreature);

	//doMoveCreature(cid, direction)
	lua_register(m_luaState, "doMoveCreature", LuaScriptInterface::luaDoMoveCreature);

	//doPlayerSetTown(cid, townid)
	lua_register(m_luaState, "doPlayerSetTown", LuaScriptInterface::luaDoPlayerSetTown);

	//doPlayerSetVocation(cid,voc)
	lua_register(m_luaState, "doPlayerSetVocation", LuaScriptInterface::luaDoPlayerSetVocation);

	//doPlayerRemoveItem(cid, itemid, count, <optional> subtype)
	lua_register(m_luaState, "doPlayerRemoveItem", LuaScriptInterface::luaDoPlayerRemoveItem);

	//doPlayerAddExp(cid, exp)
	lua_register(m_luaState, "doPlayerAddExp", LuaScriptInterface::luaDoPlayerAddExp);

	//doPlayerSetGuildId(cid, id)
	lua_register(m_luaState, "doPlayerSetGuildId", LuaScriptInterface::luaDoPlayerSetGuildId);

	//doPlayerSetGuildRank(cid, rank)
	lua_register(m_luaState, "doPlayerSetGuildRank", LuaScriptInterface::luaDoPlayerSetGuildRank);

	//doPlayerSetGuildNick(cid, nick)
	lua_register(m_luaState, "doPlayerSetGuildNick", LuaScriptInterface::luaDoPlayerSetGuildNick);

	//doPlayerAddOutfit(cid,looktype,addons)
	lua_register(m_luaState, "doPlayerAddOutfit", LuaScriptInterface::luaDoPlayerAddOutfit);

	//doPlayerRemOutfit(cid,looktype,addons)
	lua_register(m_luaState, "doPlayerRemOutfit", LuaScriptInterface::luaDoPlayerRemOutfit);

	//canPlayerWearOutfit(cid, looktype, addons)
	lua_register(m_luaState, "canPlayerWearOutfit", LuaScriptInterface::luaCanPlayerWearOutfit);

	//doSetCreatureLight(cid, lightLevel, lightColor, time)
	lua_register(m_luaState, "doSetCreatureLight", LuaScriptInterface::luaDoSetCreatureLight);

	//doSetCreatureDropLoot(cid, doDrop)
	lua_register(m_luaState, "doSetCreatureDropLoot", LuaScriptInterface::luaDoSetCreatureDropLoot);

	//getCreatureMaster(cid)
	//returns the creature's master or itself if the creature isn't a summon
	lua_register(m_luaState, "getCreatureMaster", LuaScriptInterface::luaGetCreatureMaster);

	//getCreatureSummons(cid)
	//returns a table with all the summons of the creature
	lua_register(m_luaState, "getCreatureSummons", LuaScriptInterface::luaGetCreatureSummons);

	//getSpectators(centerPos, rangex, rangey, multifloor)
	lua_register(m_luaState, "getSpectators", LuaScriptInterface::luaGetSpectators);

	//getCreatureCondition(cid, condition)
	lua_register(m_luaState, "getCreatureCondition", LuaScriptInterface::luaGetCreatureCondition);

	//isPlayer(cid)
	lua_register(m_luaState, "isPlayer", LuaScriptInterface::luaIsPlayer);

	//isCreature(cid)
	lua_register(m_luaState, "isCreature", LuaScriptInterface::luaIsCreature);

	//isContainer(uid)
	lua_register(m_luaState, "isContainer", LuaScriptInterface::luaIsContainer);

	//isCorpse(uid)
	lua_register(m_luaState, "isCorpse", LuaScriptInterface::luaIsCorpse);

	//isMoveable(uid)
	lua_register(m_luaState, "isMoveable", LuaScriptInterface::luaIsMoveable);

	//getCreatureByName(name)
	lua_register(m_luaState, "getCreatureByName", LuaScriptInterface::luaGetCreatureByName);

	//getPlayerGUIDByName(name)
	lua_register(m_luaState, "getPlayerGUIDByName", LuaScriptInterface::luaGetPlayerGUIDByName);

	//registerCreatureEvent(uid, eventName)
	lua_register(m_luaState, "registerCreatureEvent", LuaScriptInterface::luaRegisterCreatureEvent);

	//getContainerSize(uid)
	lua_register(m_luaState, "getContainerSize", LuaScriptInterface::luaGetContainerSize);

	//getContainerCap(uid)
	lua_register(m_luaState, "getContainerCap", LuaScriptInterface::luaGetContainerCap);

	//getContainerItem(uid, slot)
	lua_register(m_luaState, "getContainerItem", LuaScriptInterface::luaGetContainerItem);

	//doAddContainerItem(uid, itemid, <optional> count/subtype)
	lua_register(m_luaState, "doAddContainerItem", LuaScriptInterface::luaDoAddContainerItem);

	//getDepotId(uid)
	lua_register(m_luaState, "getDepotId", LuaScriptInterface::luaGetDepotId);

	//getHouseOwner(houseid)
	lua_register(m_luaState, "getHouseOwner", LuaScriptInterface::luaGetHouseOwner);

	//getHouseName(houseid)
	lua_register(m_luaState, "getHouseName", LuaScriptInterface::luaGetHouseName);

	//getHouseEntry(houseid)
	lua_register(m_luaState, "getHouseEntry", LuaScriptInterface::luaGetHouseEntry);

	//getHouseRent(houseid)
	lua_register(m_luaState, "getHouseRent", LuaScriptInterface::luaGetHouseRent);

	//getHouseTown(houseid)
	lua_register(m_luaState, "getHouseTown", LuaScriptInterface::luaGetHouseTown);

	//getHouseAccessList(houseid, listid)
	lua_register(m_luaState, "getHouseAccessList", LuaScriptInterface::luaGetHouseAccessList);

	//getHouseByPlayerGUID(playerGUID)
	lua_register(m_luaState, "getHouseByPlayerGUID", LuaScriptInterface::luaGetHouseByPlayerGUID);

	//getHouseTilesSize(houseid)
	lua_register(m_luaState, "getHouseTilesSize", LuaScriptInterface::luaGetHouseTilesSize);

	//setHouseAccessList(houseid, listid, listtext)
	lua_register(m_luaState, "setHouseAccessList", LuaScriptInterface::luaSetHouseAccessList);

	//setHouseOwner(houseid, ownerGUID)
	lua_register(m_luaState, "setHouseOwner", LuaScriptInterface::luaSetHouseOwner);

	//getWorldType()
	lua_register(m_luaState, "getWorldType", LuaScriptInterface::luaGetWorldType);

	//getWorldTime()
	lua_register(m_luaState, "getWorldTime", LuaScriptInterface::luaGetWorldTime);

	//getWorldLight()
	lua_register(m_luaState, "getWorldLight", LuaScriptInterface::luaGetWorldLight);

	//getWorldCreatures(type)
	//0 players, 1 monsters, 2 npcs, 3 all
	lua_register(m_luaState, "getWorldCreatures", LuaScriptInterface::luaGetWorldCreatures);

	//getWorldUpTime()
	lua_register(m_luaState, "getWorldUpTime", LuaScriptInterface::luaGetWorldUpTime);

	//broadcastMessage(message, type)
	lua_register(m_luaState, "broadcastMessage", LuaScriptInterface::luaBroadcastMessage);

	//doPlayerBroadcastMessage(cid, message, <optional> type)
	lua_register(m_luaState, "doPlayerBroadcastMessage", LuaScriptInterface::luaDoPlayerBroadcastMessage);

	//getGuildId(guild_name)
	lua_register(m_luaState, "getGuildId", LuaScriptInterface::luaGetGuildId);

	//getPlayerSex(cid)
	lua_register(m_luaState, "getPlayerSex", LuaScriptInterface::luaGetPlayerSex);

	//doPlayerSetSex(cid, newSex)
	lua_register(m_luaState, "doPlayerSetSex", LuaScriptInterface::luaDoPlayerSetSex);

	//createCombatArea( {area}, <optional> {extArea} )
	lua_register(m_luaState, "createCombatArea", LuaScriptInterface::luaCreateCombatArea);

	//createConditionObject(type)
	lua_register(m_luaState, "createConditionObject", LuaScriptInterface::luaCreateConditionObject);

	//setCombatArea(combat, area)
	lua_register(m_luaState, "setCombatArea", LuaScriptInterface::luaSetCombatArea);

	//setCombatCondition(combat, condition)
	lua_register(m_luaState, "setCombatCondition", LuaScriptInterface::luaSetCombatCondition);

	//setCombatParam(combat, key, value)
	lua_register(m_luaState, "setCombatParam", LuaScriptInterface::luaSetCombatParam);

	//setConditionParam(condition, key, value)
	lua_register(m_luaState, "setConditionParam", LuaScriptInterface::luaSetConditionParam);

	//addDamageCondition(condition, rounds, time, value)
	lua_register(m_luaState, "addDamageCondition", LuaScriptInterface::luaAddDamageCondition);

	//addOutfitCondition(condition, lookTypeEx, lookType, lookHead, lookBody, lookLegs, lookFeet)
	lua_register(m_luaState, "addOutfitCondition", LuaScriptInterface::luaAddOutfitCondition);

	//setCombatCallBack(combat, key, function_name)
	lua_register(m_luaState, "setCombatCallback", LuaScriptInterface::luaSetCombatCallBack);

	//setCombatFormula(combat, type, mina, minb, maxa, maxb)
	lua_register(m_luaState, "setCombatFormula", LuaScriptInterface::luaSetCombatFormula);

	//setConditionFormula(combat, mina, minb, maxa, maxb)
	lua_register(m_luaState, "setConditionFormula", LuaScriptInterface::luaSetConditionFormula);

	//doCombat(cid, combat, param)
	lua_register(m_luaState, "doCombat", LuaScriptInterface::luaDoCombat);

	//createCombatObject()
	lua_register(m_luaState, "createCombatObject", LuaScriptInterface::luaCreateCombatObject);

	//doAreaCombatHealth(cid, type, pos, area, min, max, effect)
	lua_register(m_luaState, "doAreaCombatHealth", LuaScriptInterface::luaDoAreaCombatHealth);

	//doTargetCombatHealth(cid, target, type, min, max, effect)
	lua_register(m_luaState, "doTargetCombatHealth", LuaScriptInterface::luaDoTargetCombatHealth);

	//doAreaCombatMana(cid, pos, area, min, max, effect)
	lua_register(m_luaState, "doAreaCombatMana", LuaScriptInterface::luaDoAreaCombatMana);

	//doTargetCombatMana(cid, target, min, max, effect)
	lua_register(m_luaState, "doTargetCombatMana", LuaScriptInterface::luaDoTargetCombatMana);

	//doAreaCombatCondition(cid, pos, area, condition, effect)
	lua_register(m_luaState, "doAreaCombatCondition", LuaScriptInterface::luaDoAreaCombatCondition);

	//doTargetCombatCondition(cid, target, condition, effect)
	lua_register(m_luaState, "doTargetCombatCondition", LuaScriptInterface::luaDoTargetCombatCondition);

	//doAreaCombatDispel(cid, pos, area, type, effect)
	lua_register(m_luaState, "doAreaCombatDispel", LuaScriptInterface::luaDoAreaCombatDispel);

	//doTargetCombatDispel(cid, target, type, effect)
	lua_register(m_luaState, "doTargetCombatDispel", LuaScriptInterface::luaDoTargetCombatDispel);

	//doChallengeCreature(cid, target)
	lua_register(m_luaState, "doChallengeCreature", LuaScriptInterface::luaDoChallengeCreature);

	//numberToVariant(number)
	lua_register(m_luaState, "numberToVariant", LuaScriptInterface::luaNumberToVariant);

	//stringToVariant(string)
	lua_register(m_luaState, "stringToVariant", LuaScriptInterface::luaStringToVariant);

	//positionToVariant(pos)
	lua_register(m_luaState, "positionToVariant", LuaScriptInterface::luaPositionToVariant);

	//targetPositionToVariant(pos)
	lua_register(m_luaState, "targetPositionToVariant", LuaScriptInterface::luaTargetPositionToVariant);

	//variantToNumber(var)
	lua_register(m_luaState, "variantToNumber", LuaScriptInterface::luaVariantToNumber);

	//variantToString(var)
	lua_register(m_luaState, "variantToString", LuaScriptInterface::luaVariantToString);

	//variantToPosition(var)
	lua_register(m_luaState, "variantToPosition", LuaScriptInterface::luaVariantToPosition);

	//doChangeSpeed(cid, delta)
	lua_register(m_luaState, "doChangeSpeed", LuaScriptInterface::luaDoChangeSpeed);

	//doCreatureChangeOutfit(cid, outfit)
	lua_register(m_luaState, "doCreatureChangeOutfit", LuaScriptInterface::luaDoCreatureChangeOutfit);

	//doSetMonsterOutfit(cid, name, time)
	lua_register(m_luaState, "doSetMonsterOutfit", LuaScriptInterface::luaSetMonsterOutfit);

	//doSetItemOutfit(cid, item, time)
	lua_register(m_luaState, "doSetItemOutfit", LuaScriptInterface::luaSetItemOutfit);

	//doSetCreatureOutfit(cid, outfit, time)
	lua_register(m_luaState, "doSetCreatureOutfit", LuaScriptInterface::luaSetCreatureOutfit);

	//getCreatureOutfit(cid)
	lua_register(m_luaState, "getCreatureOutfit", LuaScriptInterface::luaGetCreatureOutfit);

	//getCreaturePosition(cid)
	lua_register(m_luaState, "getCreaturePosition", LuaScriptInterface::luaGetCreaturePosition);

	//getCreatureName(cid)
	lua_register(m_luaState, "getCreatureName", LuaScriptInterface::luaGetCreatureName);

	//getCreatureSpeed(cid)
	lua_register(m_luaState, "getCreatureSpeed", LuaScriptInterface::luaGetCreatureSpeed);

	//getCreatureBaseSpeed(cid)
	lua_register(m_luaState, "getCreatureBaseSpeed", LuaScriptInterface::luaGetCreatureBaseSpeed);

	//getCreatureTarget(cid)
	lua_register(m_luaState, "getCreatureTarget", LuaScriptInterface::luaGetCreatureTarget);

	//isItemStackable(itemid)
	lua_register(m_luaState, "isItemStackable", LuaScriptInterface::luaIsItemStackable);

	//isItemRune(itemid)
	lua_register(m_luaState, "isItemRune", LuaScriptInterface::luaIsItemRune);

	//isItemDoor(itemid)
	lua_register(m_luaState, "isItemDoor", LuaScriptInterface::luaIsItemDoor);

	//isItemContainer(itemid)
	lua_register(m_luaState, "isItemContainer", LuaScriptInterface::luaIsItemContainer);

	//isItemFluidContainer(itemid)
	lua_register(m_luaState, "isItemFluidContainer", LuaScriptInterface::luaIsItemFluidContainer);

	//isItemMoveable(itemid)
	lua_register(m_luaState, "isItemMoveable", LuaScriptInterface::luaIsItemMoveable);

	//getItemDescriptions(itemid)
	lua_register(m_luaState, "getItemDescriptions", LuaScriptInterface::luaGetItemDescriptions);

	//getItemName(itemid)
	lua_register(m_luaState, "getItemName", LuaScriptInterface::luaGetItemName);

	//getItemWeight(itemid, count, <optional: default: 1> precise)
	lua_register(m_luaState, "getItemWeight", LuaScriptInterface::luaGetItemWeight);

	//hasProperty(uid)
	lua_register(m_luaState, "hasProperty", LuaScriptInterface::luaHasProperty);

	//getItemIdByName(name)
	lua_register(m_luaState, "getItemIdByName", LuaScriptInterface::luaGetItemIdByName);

	//isSightClear(fromPos, toPos, floorCheck)
	lua_register(m_luaState, "isSightClear", LuaScriptInterface::luaIsSightClear);

	//getFluidSourceType(type)
	lua_register(m_luaState, "getFluidSourceType", LuaScriptInterface::luaGetFluidSourceType);

	//isInArray(array, value)
	lua_register(m_luaState, "isInArray", LuaScriptInterface::luaIsInArray);

	//addEvent(callback, delay, ...)
	lua_register(m_luaState, "addEvent", LuaScriptInterface::luaAddEvent);

	//stopEvent(eventid)
	lua_register(m_luaState, "stopEvent", LuaScriptInterface::luaStopEvent);

	//doPlayerPopupFYI(cid, message)
	lua_register(m_luaState, "doPlayerPopupFYI", LuaScriptInterface::luaDoPlayerPopupFYI);

	//mayNotMove(cid, value)
	lua_register(m_luaState, "mayNotMove", LuaScriptInterface::luaMayNotMove);

	//doPlayerAddPremiumDays(cid, days)
	lua_register(m_luaState, "doPlayerAddPremiumDays", LuaScriptInterface::luaDoPlayerAddPremiumDays);

	//doPlayerRemovePremiumDays(cid, days)
	lua_register(m_luaState, "doPlayerRemovePremiumDays", LuaScriptInterface::luaDoPlayerRemovePremiumDays);

	//getPlayerPremiumDays(cid)
	lua_register(m_luaState, "getPlayerPremiumDays", LuaScriptInterface::luaGetPlayerPremiumDays);

	//getPromotedVocation(vocation)
	lua_register(m_luaState, "getPromotedVocation", LuaScriptInterface::luaGetPromotedVocation);

	//getPlayerBlessing(cid, blessing)
	lua_register(m_luaState, "getPlayerBlessing", LuaScriptInterface::luaGetPlayerBlessing);

	//doPlayerAddBlessing(cid, blessing)
	lua_register(m_luaState, "doPlayerAddBlessing", LuaScriptInterface::luaDoPlayerAddBlessing);

	//saveServer()
	lua_register(m_luaState, "doSaveServer", LuaScriptInterface::luaDoSaveServer);

	//getPlayersByAccountNumber(accountNumber)
	lua_register(m_luaState, "getPlayersByAccountNumber", LuaScriptInterface::luaGetPlayersByAccountNumber);

	//getAccountNumberByPlayerName(name)
	lua_register(m_luaState, "getAccountNumberByPlayerName", LuaScriptInterface::luaGetAccountNumberByPlayerName);

	//getIPByPlayerName(name)
	lua_register(m_luaState, "getIPByPlayerName", LuaScriptInterface::luaGetIPByPlayerName);

	//getPlayersByIPAddress(ip[, mask = 0xFFFFFFFF])
	lua_register(m_luaState, "getPlayersByIPAddress", LuaScriptInterface::luaGetPlayersByIPAddress);

	//debugPrint(text)
	lua_register(m_luaState, "debugPrint", LuaScriptInterface::luaDebugPrint);

	//bit operations for Lua, based on bitlib project release 24
	//bit.bnot, bit.band, bit.bor, bit.bxor, bit.lshift, bit.rshift
	luaL_register(m_luaState, "bit", LuaScriptInterface::luaBitReg);

	//db table
	luaL_register(m_luaState, "db", LuaScriptInterface::luaDatabaseTable);

	//result table
	luaL_register(m_luaState, "result", LuaScriptInterface::luaResultTable);

	//isPlayerGhost(cid)
	lua_register(m_luaState, "isPlayerGhost", LuaScriptInterface::luaIsPlayerGhost);

	//getPlayerParty(cid)
	lua_register(m_luaState, "getPlayerParty", LuaScriptInterface::luaGetPlayerParty);

	//doPlayerJoinParty(cid, leaderId)
	lua_register(m_luaState, "doPlayerJoinParty", LuaScriptInterface::luaDoPlayerJoinParty);

	//getPartyMembers(leaderId)
	lua_register(m_luaState, "getPartyMembers", LuaScriptInterface::luaGetPartyMembers);

	//doCleanMap()
	lua_register(m_luaState, "doCleanMap", LuaScriptInterface::luaDoCleanMap);

	//getPlayerBalance(cid)
	lua_register(m_luaState, "getPlayerBalance", LuaScriptInterface::luaGetPlayerBankBalance);

	//doPlayerSetBalance(cid, balance)
	lua_register(m_luaState, "doPlayerSetBalance", LuaScriptInterface::luaDoPlayerSetBankBalance);

	//isPlayerPzLocked(cid)
	lua_register(m_luaState, "isPlayerPzLocked", LuaScriptInterface::luaIsPlayerPzLocked);

	//getPlayerIp(cid)
	lua_register(m_luaState, "getPlayerIp", LuaScriptInterface::luaGetPlayerIp);

	//getPlayerMasterPososition(cid)
	lua_register(m_luaState, "getPlayerMasterPososition", LuaScriptInterface::luaGetPlayerMasterPos);

	//isValidItemId(itemid)
	lua_register(m_luaState, "isValidItemId", LuaScriptInterface::luaIsValidItemId);

	//getGameState()
	lua_register(m_luaState, "getGameState", LuaScriptInterface::luaGetGameState);

	//doSetGameState(id)
	lua_register(m_luaState, "doSetGameState", LuaScriptInterface::luaDoSetGameState);

	//getPlayersOnline()
	lua_register(m_luaState, "getPlayersOnline", LuaScriptInterface::luaGetPlayersOnline);

	//getClosestFreeTile(cid, targetpos[, extended = false[, ignoreHouse = true]])
	lua_register(m_luaState, "getClosestFreeTile", LuaScriptInterface::luaGetClosestFreeTile);

	//getTownName(townId)
	lua_register(m_luaState, "getTownName", LuaScriptInterface::luaGetTownName);

	//getTownTemplePosition(townId[, displayError])
	lua_register(m_luaState, "getTownTemplePosition", LuaScriptInterface::luaGetTownTemplePosition);

	//getPlayerByNameWildcard(name~[, ret = false])
	lua_register(m_luaState, "getPlayerByNameWildcard", LuaScriptInterface::luaGetPlayerByNameWildcard);

	//getTownId(townName)
	lua_register(m_luaState, "getTownId", LuaScriptInterface::luaGetTownId);

	//doCreateMonster(name, pos[, displayError = true])
	lua_register(m_luaState, "doCreateMonster", LuaScriptInterface::luaDoCreateMonster);

	//doCreateNpc(name, pos[, displayError = true])
	lua_register(m_luaState, "doCreateNpc", LuaScriptInterface::luaDoCreateNpc);

	//doSummonMonster(cid, name)
	lua_register(m_luaState, "doSummonMonster", LuaScriptInterface::luaDoSummonMonster);

	//getExperienceStage(level)
	lua_register(m_luaState, "getExperienceStage", LuaScriptInterface::luaGetExperienceStage);
}

int32_t LuaScriptInterface::internalGetPlayerInfo(lua_State* L, PlayerInfo_t info)
{
	uint32_t cid = popNumber(L);
	ScriptEnvironment* env = getScriptEnv();
	const Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		const Tile *tile;
		Position pos;
		uint32_t stackpos;
		int32_t value;

		switch(info)
		{
			case PlayerInfoAccess:
				value = player->accessLevel;
				break;

			case PlayerInfoLevel:
				value = player->level;
				break;

			case PlayerInfoMagLevel:
				value = player->magLevel;
				break;

			case PlayerInfoMasterPos:
			{
				Position pos;
				pos = player->masterPos;
				pushPosition(L, pos, 0);
				return 1;
			}

			case PlayerInfoName:
				lua_pushstring(L, player->name.c_str());
				return 1;

			case PlayerInfoLookDirection:
				value = player->direction;
				break;

			case PlayerInfoTown:
				value = player->getTown();
				break;

			case PlayerInfoGUID:
				value = player->guid;
				break;

			case PlayerInfoPremiumDays:
				value = player->premiumDays;
				break;

			case PlayerInfoSkullType:
				value = (uint32_t)player->getSkull();
				break;

			case PlayerInfoFood:
			{
				Condition* condition = player->getCondition(CONDITION_REGENERATION, CONDITIONID_DEFAULT);
				if(condition)
					value = condition->getTicks() / 1000;
				else
					value = 0;
				break;
			}

			case PlayerInfoVocation:
				value = player->getVocationId();
				break;

			case PlayerInfoSoul:
				value = player->getPlayerInfo(PLAYERINFO_SOUL);
				break;

			case PlayerInfoFreeCap:
				value = (int32_t)player->getFreeCapacity();
				break;

			case PlayerInfoGuildId:
				value = player->getGuildId();
				break;

			case PlayerInfoGuildLevel:
				value = player->getGuildLevel();
				break;

			case PlayerInfoGuildName:
				lua_pushstring(L, player->getGuildName().c_str());
				return 1;
	
			case PlayerInfoGuildRank:
				lua_pushstring(L, player->getGuildRank().c_str());
				return 1;

			case PlayerInfoGuildNick:
				lua_pushstring(L, player->getGuildNick().c_str());
				return 1;

			case PlayerInfoSex:
				value = player->getSex();
				break;

			case PlayerInfoGroupId:
				value = player->groupId;
				break;

			case PlayerInfoPzLock:
				lua_pushboolean(L, player->isPzLocked());
				return 1;
	
			case PlayerInfoGhostStatus:
				value = player->isInGhostMode();
				break;

			case PlayerInfoIp:
				value = (int32_t)player->getIP();
				break;

			case PlayerInfoBankBalance:
				lua_pushnumber(L, player->getBankBalance());
				return 1;

			default:
				std::string error_str = "Unknown player info. info = " + info;
				reportErrorFunc(error_str);
				value = 0;
				break;
		}
		lua_pushnumber(L, value);
		return 1;
	}
	else
	{
		lua_pushboolean(L, false);
		return 1;
	}
	
	lua_pushboolean(L, true);
	return 1;
}
//getPlayer[Info](uid)
int32_t LuaScriptInterface::luaGetPlayerGuildLevel(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoGuildLevel);
}

int32_t LuaScriptInterface::luaIsPlayerPzLocked(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoPzLock);
}

int32_t LuaScriptInterface::luaGetPlayerFood(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoFood);
}
	
int32_t LuaScriptInterface::luaGetPlayerAccess(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoAccess);
}
	
int32_t LuaScriptInterface::luaGetPlayerLevel(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoLevel);
}
	
int32_t LuaScriptInterface::luaGetPlayerMagLevel(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoMagLevel);
}

int32_t LuaScriptInterface::luaGetPlayerName(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoName);
}
	
int32_t LuaScriptInterface::luaGetPlayerVocation(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoVocation);
}

int32_t LuaScriptInterface::luaGetPlayerSoul(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoSoul);
}

int32_t LuaScriptInterface::luaGetPlayerFreeCap(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoFreeCap);
}
	
int32_t LuaScriptInterface::luaGetPlayerGuildId(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoGuildId);
}
	
int32_t LuaScriptInterface::luaGetPlayerGuildName(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoGuildName);
}
	
int32_t LuaScriptInterface::luaGetPlayerGuildRank(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoGuildRank);
}
	
int32_t LuaScriptInterface::luaGetPlayerGuildNick(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoGuildNick);
}
	
int32_t LuaScriptInterface::luaGetPlayerSex(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoSex);
}
	
int32_t LuaScriptInterface::luaGetPlayerLookDir(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoLookDirection);
}
	
int32_t LuaScriptInterface::luaGetPlayerTown(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoTown);
}
	
int32_t LuaScriptInterface::luaGetPlayerGroupId(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoGroupId);
}
	
int32_t LuaScriptInterface::luaGetPlayerGUID(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoGUID);
}

int32_t LuaScriptInterface::luaGetPlayerPremiumDays(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoPremiumDays);
}

int32_t LuaScriptInterface::luaGetPlayerSkullType(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoSkullType);
}

int32_t LuaScriptInterface::luaIsPlayerGhost(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoGhostStatus);
}

int32_t LuaScriptInterface::luaGetPlayerIp(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoIp);
}

int32_t LuaScriptInterface::luaGetPlayerBankBalance(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoBankBalance);
}

int32_t LuaScriptInterface::luaGetPlayerMasterPos(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoMasterPos);
}

int32_t LuaScriptInterface::luaGetPlayerFlagValue(lua_State* L)
{
	//getPlayerFlagValue(cid, flag)
	uint32_t flagindex = popNumber(L);
	uint32_t cid = popNumber(L);
	
	ScriptEnvironment* env = getScriptEnv();
	
	Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		if(flagindex < PlayerFlag_LastFlag)
			lua_pushboolean(L, player->hasFlag((PlayerFlags)flagindex));
		else
		{
			reportErrorFunc("No valid flag index.");
			lua_pushboolean(L, false);
		}
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaPlayerLearnInstantSpell(lua_State* L)
{
	//playerLearnInstantSpell(cid, name)
	std::string spellName = popString(L);
	uint32_t cid = popNumber(L);
	
	ScriptEnvironment* env = getScriptEnv();

	Player* player = env->getPlayerByUID(cid);
	if(!player)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	InstantSpell* spell = g_spells->getInstantSpellByName(spellName);
	if(!spell)
	{
		std::string error_str = (std::string)"Spell \"" + spellName + (std::string)"\" not found";
		reportErrorFunc(error_str);
		lua_pushboolean(L, false);
		return 1;
	}

	player->learnInstantSpell(spell->getName());
	lua_pushboolean(L, true);
	return 1;
}

int32_t LuaScriptInterface::luaCanPlayerLearnInstantSpell(lua_State* L)
{
	//canPlayerLearnInstantSpell(cid, name)
	std::string spellName = popString(L);
	uint32_t cid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	Player* player = env->getPlayerByUID(cid);
	if(!player)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	InstantSpell* spell = g_spells->getInstantSpellByName(spellName);
	if(!spell)
	{
		std::string error_str = (std::string)"Spell \"" + spellName + (std::string)"\" not found";
		reportErrorFunc(error_str);
		lua_pushboolean(L, false);
		return 1;
	}

	if(!player->hasFlag(PlayerFlag_IgnoreSpellCheck))
	{
		if(player->getLevel() < spell->getLevel())
		{
		lua_pushboolean(L, false);
			return 1;
		}
		
		if(player->getMagicLevel() < spell->getMagicLevel())
		{
			lua_pushboolean(L, false);
			return 1;
		}
	}

	lua_pushboolean(L, true);
	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerLearnedInstantSpell(lua_State* L)
{
	//getPlayerLearnedInstantSpell(cid, name)
	std::string spellName = popString(L);
	uint32_t cid = popNumber(L);
	
	ScriptEnvironment* env = getScriptEnv();

	Player* player = env->getPlayerByUID(cid);
	if(!player)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	InstantSpell* spell = g_spells->getInstantSpellByName(spellName);
	if(!spell)
	{
		std::string error_str = (std::string)"Spell \"" + spellName + (std::string)"\" not found";
		reportErrorFunc(error_str);
		lua_pushboolean(L, false);
		return 1;
	}

	if(!player->hasLearnedInstantSpell(spellName))
	{
		lua_pushboolean(L, false);
		return 1;
	}

	lua_pushboolean(L, true);
	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerInstantSpellCount(lua_State* L)
{
	//getPlayerInstantSpellCount(cid)
	uint32_t cid = popNumber(L);
	
	ScriptEnvironment* env = getScriptEnv();
	
	Player* player = env->getPlayerByUID(cid);
	if(player)
		lua_pushnumber(L, g_spells->getInstantSpellCount(player));
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerInstantSpellInfo(lua_State* L)
{
	//getPlayerInstantSpellInfo(cid, index)
	uint32_t index = popNumber(L);
	uint32_t cid = popNumber(L);
	
	ScriptEnvironment* env = getScriptEnv();

	Player* player = env->getPlayerByUID(cid);
	if(!player)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	InstantSpell* spell = g_spells->getInstantSpellByIndex(player, index);
	if(!spell)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_SPELL_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	lua_newtable(L);
	setField(L, "name", spell->getName());
	setField(L, "words", spell->getWords());
	setField(L, "level", spell->getLevel());
	setField(L, "mlevel", spell->getMagicLevel());
	setField(L, "mana", spell->getManaCost(player));
	setField(L, "manapercent", spell->getManaPercent());
	return 1;
}

int32_t LuaScriptInterface::luaGetInstantSpellInfoByName(lua_State* L)
{
	//getInstantSpellInfoByName(cid, name)
	std::string spellName = popString(L);
	uint32_t cid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	Player* player = env->getPlayerByUID(cid);
	if(!player && cid != 0)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	InstantSpell* spell = g_spells->getInstantSpellByName(spellName);
	if(!spell)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_SPELL_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	lua_newtable(L);
	setField(L, "name", spell->getName());
	setField(L, "words", spell->getWords());
	setField(L, "level", spell->getLevel());
	setField(L, "mlevel", spell->getMagicLevel());
	setField(L, "mana", (player != NULL ? spell->getManaCost(player) : 0));
	setField(L, "manapercent", spell->getManaPercent());
	return 1;
}

int32_t LuaScriptInterface::luaGetInstantSpellWords(lua_State* L)
{
	//getInstantSpellWords(name)
	std::string spellName = popString(L);
	InstantSpell* spell = g_spells->getInstantSpellByName(spellName);
	if(!spell)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_SPELL_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	lua_pushstring(L, spell->getWords().c_str());
	return 1;
}

int32_t LuaScriptInterface::luaDoRemoveItem(lua_State* L)
{
	//doRemoveItem(uid, <optional> n)
	int32_t parameters = lua_gettop(L);

	int32_t count = -1;
	if(parameters > 1)
		count = popNumber(L);

	uint32_t uid = popNumber(L);
	
	ScriptEnvironment* env = getScriptEnv();
	
	Item* item = env->getItemByUID(uid);
	if(!item)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	ReturnValue ret = g_game.internalRemoveItem(item, count);
	if(ret != RET_NOERROR)
	{
		lua_pushboolean(L, false);
		return 1;
	}
	
	lua_pushboolean(L, true);
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerRemoveItem(lua_State* L)
{
	//doPlayerRemoveItem(cid, itemid, count, <optional> subtype)
	int32_t parameters = lua_gettop(L);
	
	int32_t subType = -1;
	if(parameters > 3)
		subType = popNumber(L);
	
	uint32_t count = popNumber(L);
	uint16_t itemId = (uint16_t)popNumber(L);
	uint32_t cid = popNumber(L);
	
	ScriptEnvironment* env = getScriptEnv();
	
	Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		if(g_game.removeItemOfType(player, itemId, count, subType))
			lua_pushboolean(L, true);
		else
			lua_pushboolean(L, false);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoFeedPlayer(lua_State* L)
{
	//doFeedPlayer(cid, food)
	int32_t food = (int32_t)popNumber(L);
	uint32_t cid = popNumber(L);
	
	ScriptEnvironment* env = getScriptEnv();
	
	Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		player->addDefaultRegeneration((food * 1000) * 3);
		lua_pushboolean(L, true);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerSendCancel(lua_State* L)
{
	//doPlayerSendCancel(cid, text)
	std::string text = popString(L);
	uint32_t cid = popNumber(L);
	
	ScriptEnvironment* env = getScriptEnv();
	
	const Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		player->sendCancel(text);
		lua_pushboolean(L, true);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoSendDefaultCancel(lua_State* L)
{
	//doPlayerSendDefaultCancel(cid, ReturnValue)
	ReturnValue ret = (ReturnValue)popNumber(L);
	uint32_t cid = popNumber(L);
	
	ScriptEnvironment* env = getScriptEnv();
	
	const Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		player->sendCancelMessage(ret);
		lua_pushboolean(L, true);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}

	return 1;
}

int32_t LuaScriptInterface::luaDoTeleportThing(lua_State* L)
{
	//doTeleportThing(cid, newpos, <optional> pushmove)
	int32_t parameters = lua_gettop(L);

	bool pushMove = true;
	if(parameters > 2)
		pushMove = popNumber(L) == 1;

	PositionEx pos;
	popPosition(L, pos);
	uint32_t uid = popNumber(L);
	
	ScriptEnvironment* env = getScriptEnv();
	
	Thing* tmp = env->getThingByUID(uid);
	if(tmp)
	{
		if(g_game.internalTeleport(tmp, pos, pushMove) == RET_NOERROR)
			lua_pushboolean(L, true);
		else
			lua_pushboolean(L, false);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_THING_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoTransformItem(lua_State* L)
{
	//doTransformItem(uid, toitemid, <optional> count/subtype)
	int32_t parameters = lua_gettop(L);

	int32_t count = -1;
	if(parameters > 2)
		count = popNumber(L);

	uint16_t toId = (uint16_t)popNumber(L);
	uint32_t uid = popNumber(L);
	
	ScriptEnvironment* env = getScriptEnv();
	
	Item* item = env->getItemByUID(uid);
	if(!item)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	const ItemType& it = Item::items[toId];
	if(it.stackable && count > 100)
	{
		reportErrorFunc("Stack count cannot be higher than 100.");
		count = 100;
	}

	Item* newItem = g_game.transformItem(item, toId, count);
	
	if(item->isRemoved())
		env->removeItemByUID(uid);

	if(newItem && newItem != item)
		env->insertThing(uid, newItem);

	lua_pushboolean(L, true);
	return 1;
}

int32_t LuaScriptInterface::luaDoCreatureSay(lua_State* L)
{
	//doCreatureSay(uid,text,type)
	uint32_t type = popNumber(L);
	std::string text = popString(L);
	uint32_t cid = popNumber(L);
					
	ScriptEnvironment* env = getScriptEnv();
	
	Creature* creature = env->getCreatureByUID(cid);
	if(creature)
	{
		g_game.internalCreatureSay(creature, (SpeakClasses)type,std::string(text));
		lua_pushboolean(L, true);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoSendMagicEffect(lua_State* L)
{
	//doSendMagicEffect(pos, type[, player])
	ScriptEnvironment* env = getScriptEnv();

	uint32_t parameters = lua_gettop(L);
	SpectatorVec list;
	if(parameters > 2)
	{
		uint32_t cid = popNumber(L);
		Player* player = env->getPlayerByUID(cid);
		if(player)
			list.push_back(player);
	}

	uint32_t type = popNumber(L);
	PositionEx pos;
	popPosition(L, pos);

	if(pos.x == 0xFFFF)
		pos = env->getRealPos();

	if(!list.empty())
		g_game.addMagicEffect(list, pos, type);
	else
		g_game.addMagicEffect(pos, type);

	lua_pushboolean(L, true);
	return 1;
}

int32_t LuaScriptInterface::luaDoSendDistanceShoot(lua_State* L)
{
	//doSendDistanceShoot(frompos, topos, type)
	uint32_t type = popNumber(L);
	PositionEx toPos;
	popPosition(L, toPos);
	PositionEx fromPos;
	popPosition(L, fromPos);
	ScriptEnvironment* env = getScriptEnv();

	if(fromPos.x == 0xFFFF)
		fromPos = env->getRealPos();

	if(toPos.x == 0xFFFF)
		toPos = env->getRealPos();

	g_game.addDistanceEffect(fromPos, toPos, type);
	lua_pushboolean(L, true);
	return 1;
}

int32_t LuaScriptInterface::luaDoChangeTypeItem(lua_State* L)
{
	//doChangeTypeItem(uid,new_type)
	int32_t subtype = (int32_t)popNumber(L);
	uint32_t uid = popNumber(L);
	
	ScriptEnvironment* env = getScriptEnv();
	
	Item* item = env->getItemByUID(uid);
	if(!item)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	Item* newItem = g_game.transformItem(item, item->getID(), subtype);

	if(item->isRemoved())
		env->removeItemByUID(uid);

	if(newItem && newItem != item)
		env->insertThing(uid, newItem);

	lua_pushboolean(L, true);
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerAddSkillTry(lua_State* L)
{
	//doPlayerAddSkillTry(uid,skillid,n)
	uint32_t n = popNumber(L);
	uint32_t skillid = popNumber(L);
	uint32_t cid = popNumber(L);
	
	ScriptEnvironment* env = getScriptEnv();
	
	Player* player = env->getPlayerByUID(cid);
	if(player){
		player->addSkillAdvance((skills_t)skillid, n);
		lua_pushboolean(L, true);
	}
	else{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}


int32_t LuaScriptInterface::luaDoCreatureAddHealth(lua_State* L)
{
	//doCreatureAddHealth(uid,health)
	int32_t healthChange = (int32_t)popNumber(L);
	uint32_t cid = popNumber(L);
	ScriptEnvironment* env = getScriptEnv();
	Creature* creature = env->getCreatureByUID(cid);
	if(creature)
	{
		if(healthChange >= 0)
			g_game.combatChangeHealth(COMBAT_HEALING, NULL, creature, healthChange);
		else
			g_game.combatChangeHealth(COMBAT_UNDEFINEDDAMAGE, NULL, creature, healthChange);
		lua_pushboolean(L, true);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerAddMana(lua_State* L)
{
	//doPlayerAddMana(uid,mana)
	int32_t manaChange = (int32_t)popNumber(L);
	uint32_t cid = popNumber(L);
	
	ScriptEnvironment* env = getScriptEnv();
	
	Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		g_game.combatChangeMana(NULL, player, manaChange);
		lua_pushboolean(L, true);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerAddManaSpent(lua_State* L)
{
	//doPlayerAddManaSpent(cid,mana)
	uint32_t mana = popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		player->addManaSpent(mana);
		lua_pushboolean(L, true);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerAddItem(lua_State* L)
{
	//doPlayerAddItem(cid, itemid, <optional: default: 1> count/subtype, <optional: default: 1> canDropOnMap)
	//doPlayerAddItem(cid, itemid, <optional: default: 1> count, <optional: default: 1> canDropOnMap, <optional: default: 1>subtype)
	int32_t parameters = lua_gettop(L);

	int32_t subType = 1;
	if(parameters > 4)
		subType = popNumber(L);

	bool canDropOnMap = true;
	if(parameters > 3)
		canDropOnMap = popBoolean(L);

	uint32_t count = 1;
	if(parameters > 2)
		count = popNumber(L);

	uint32_t itemId = (uint32_t)popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();
	Player* player = env->getPlayerByUID(cid);
	if(!player)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	const ItemType& it = Item::items[itemId];

	int32_t itemCount = 1;
	if(parameters > 4)
	{
		//subtype already supplied, count then is the amount
		itemCount = std::max<int32_t>((int32_t)1, (int32_t)count);
	}
	else if(it.hasSubType())
	{
		if(it.stackable)
			itemCount = (int32_t)std::ceil((float)count / 100);

		subType = count;
	}
	else
		itemCount = std::max<int32_t>((int32_t)1, (int32_t)count);

	while(itemCount > 0)
	{
		int32_t stackCount = std::min<int32_t>((int32_t)100, (int32_t)subType);

		Item* newItem = Item::CreateItem(itemId, stackCount);
		if(!newItem)
		{
			reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
			lua_pushboolean(L, false);
			return 1;
		}

		if(it.stackable)
			subType -= stackCount;

		ReturnValue ret = g_game.internalPlayerAddItem(player, newItem, canDropOnMap);
		if(ret != RET_NOERROR)
		{
			delete newItem;
			lua_pushboolean(L, false);
			return 1;
		}

		--itemCount;
		if(itemCount == 0)
		{
			if(newItem->getParent())
			{
				uint32_t uid = env->addThing(newItem);
				lua_pushnumber(L, uid);
				return 1;
			}
			else
			{
				//stackable item stacked with existing object, newItem will be released
				lua_pushboolean(L, false);
				return 1;
			}
		}
	}
	lua_pushboolean(L, false);
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerAddItemEx(lua_State* L)
{
	//doPlayerAddItemEx(cid, uid, <optional: default: 0> canDropOnMap)
	int32_t parameters = lua_gettop(L);

	bool canDropOnMap = false;
	if(parameters > 2)
		canDropOnMap = popNumber(L) == 1;

	uint32_t uid = (uint32_t)popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();
	Player* player = env->getPlayerByUID(cid);
	if(!player)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	Item* item = env->getItemByUID(uid);
	if(!item)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	if(item->getParent() != VirtualCylinder::virtualCylinder)
	{
		reportErrorFunc("Item already has a parent");
		lua_pushboolean(L, false);
		return 1;
	}

	ReturnValue ret = RET_NOERROR;
	if(canDropOnMap)
		ret = g_game.internalPlayerAddItem(player, item);
	else
		ret = g_game.internalAddItem(player, item);

	lua_pushnumber(L, ret);
	return 1;
}

int32_t LuaScriptInterface::luaDoTileAddItemEx(lua_State* L)
{
	//doTileAddItemEx(pos, uid)
	uint32_t uid = (uint32_t)popNumber(L);
	PositionEx pos;
	popPosition(L, pos);

	ScriptEnvironment* env = getScriptEnv();
	Tile* tile = g_game.getTile(pos.x, pos.y, pos.z);
	if(!tile)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_TILE_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	Item* item = env->getItemByUID(uid);
	if(!item)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	if(item->getParent() != VirtualCylinder::virtualCylinder)
	{
		reportErrorFunc("Item already has a parent");
		lua_pushboolean(L, false);
		return 1;
	}

	ReturnValue ret = g_game.internalAddItem(tile, item);
	lua_pushnumber(L, ret);
	return 1;
}

int32_t LuaScriptInterface::luaDoRelocate(lua_State* L)
{
	//doRelocate(pos, posTo)
	//Moves all moveable objects from pos to posTo

	PositionEx toPos;
	popPosition(L, toPos);

	PositionEx fromPos;
	popPosition(L, fromPos);

	Tile* fromTile = g_game.getTile(fromPos.x, fromPos.y, fromPos.z);
	if(!fromTile)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_TILE_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	Tile* toTile = g_game.getTile(toPos.x, toPos.y, toPos.z);
	if(!toTile)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_TILE_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	int32_t thingCount = fromTile->getThingCount();
	for(int32_t i = thingCount - 1; i >= 0; --i)
	{
		Thing* thing = fromTile->__getThing(i);
		if(thing)
		{
			if(Item* item = thing->getItem())
			{
				if(item->isPushable())
					g_game.internalTeleport(item, toPos, false);
			}
			else if(Creature* creature = thing->getCreature())
				g_game.internalTeleport(creature, toPos, true);
		}
	}

	lua_pushboolean(L, true);
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerSendTextMessage(lua_State* L)
{
	//doPlayerSendTextMessage(cid, MessageClasses, message)
	std::string text = popString(L);
	uint32_t messageClass = popNumber(L);
	uint32_t cid = popNumber(L);
	
	ScriptEnvironment* env = getScriptEnv();
	
	const Player* player = env->getPlayerByUID(cid);
	if(!player)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	player->sendTextMessage((MessageClasses)messageClass, text);
	lua_pushboolean(L, true);
	return 1;
}

int32_t LuaScriptInterface::luaDoSendAnimatedText(lua_State* L)
{
	//doSendAnimatedText(pos, text, color)
	uint32_t color = popNumber(L);
	std::string text = popString(L);
	PositionEx pos;
	popPosition(L, pos);
	
	ScriptEnvironment* env = getScriptEnv();
	
	SpectatorVec list;
	SpectatorVec::iterator it;

	if(pos.x == 0xFFFF)
		pos = env->getRealPos();
	
	g_game.addAnimatedText(pos, color, text);
	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerSkill(lua_State* L)
{
	//getPlayerSkill(cid, skillid)
	uint32_t skillid = popNumber(L);
	uint32_t cid = popNumber(L);
	
	ScriptEnvironment* env = getScriptEnv();
	
	const Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		if(skillid <= 6)
		{
			uint32_t value = player->skills[skillid][SKILL_LEVEL];
			lua_pushnumber(L, value);
		}
		else
		{
			reportErrorFunc("No valid skillId");
			lua_pushboolean(L, false);
		}
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerLossPercent(lua_State* L)
{
	//getPlayerLossPercent(cid)
	uint32_t cid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		uint32_t value = (uint32_t)(player->getLostPercent() * 100);
		lua_pushnumber(L, value);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}


int32_t LuaScriptInterface::luaDoSetCreatureDropLoot(lua_State* L)
{
	//doSetCreatureDropLoot(cid, doDrop)
	bool doDrop = popNumber(L) == 1;
	uint32_t cid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	Creature* creature = env->getCreatureByUID(cid);
	if(creature){
		creature->setDropLoot(doDrop);
	}
	else{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}

	return 1;
}

int32_t LuaScriptInterface::luaDoShowTextDialog(lua_State* L)
{
	//doShowTextDialog(cid, itemid, text)
	std::string text = popString(L);
	uint32_t itemId = popNumber(L);
	uint32_t cid = popNumber(L);
	
	ScriptEnvironment* env = getScriptEnv();
	
	Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		player->setWriteItem(NULL, 0);
		player->sendTextWindow(itemId, text);
		lua_pushboolean(L, true);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int LuaScriptInterface::luaDoAddMark(lua_State *L)
{
	//doAddMapMark(cid, pos, type, <optional> description)
	int32_t parameters = lua_gettop(L);
	std::string description = "";
	Position pos;
	uint32_t stackpos;

	if(parameters > 3)
		description = popString(L);

	uint32_t type = popNumber(L);
	popPosition(L, pos, stackpos);
	uint32_t cid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();
	Player* player = env->getPlayerByUID(cid);
	if(!player)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}

	player->sendAddMarker(pos, type, description);
	lua_pushboolean(L, true);
	return 1;
}

int32_t LuaScriptInterface::luaGetItemRWInfo(lua_State* L)
{
	//getItemRWInfo(uid)
	uint32_t uid = popNumber(L);
	
	ScriptEnvironment* env = getScriptEnv();
	
	const Item* item = env->getItemByUID(uid);
	if(item)
	{
		uint32_t rwflags = 0;
		if(item->isReadable())
			rwflags |= 1;
		
		if(item->canWriteText())
			rwflags |= 2;
			
		lua_pushnumber(L, rwflags);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoDecayItem(lua_State* L)
{
	//doDecayItem(uid)
	//Note: to stop decay set decayTo = 0 in items.otb
	uint32_t uid = popNumber(L);
	
	ScriptEnvironment* env = getScriptEnv();
	
	Item* item = env->getItemByUID(uid);
	if(item){
		g_game.startDecay(item);
		lua_pushboolean(L, true);
	}
	else{
		reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetThingfromPos(lua_State* L)
{
	//Consider using getTileItemById/getTileItemByType/getTileThingByPos/getTopCreature instead.

	//getThingfromPos(pos)
	//Note: 
	//	stackpos = 255. Get the top thing(item moveable or creature)
	//	stackpos = 254. Get MagicFieldtItem
	//	stackpos = 253. Get the top creature (moveable creature)
	
	PositionEx pos;
	popPosition(L, pos);
	
	ScriptEnvironment* env = getScriptEnv();

	Tile* tile = g_game.getMap()->getTile(pos);
	Thing *thing = NULL;
	
	if(tile)
	{
		if(pos.stackpos == 255)
		{
			thing = tile->getTopCreature();
			if(thing == NULL)
			{
				Item* item = tile->getTopDownItem();
				if(item && !item->isNotMoveable())
					thing = item;
			}
		}
		else if(pos.stackpos == 254)
			thing = tile->getFieldItem();
		else if(pos.stackpos == 253)
			thing = tile->getTopCreature();
		else
			thing = tile->__getThing(pos.stackpos);
		
		if(thing)
		{
			uint32_t thingid = env->addThing(thing);
			pushThing(L, thing, thingid);
		}
		else
			pushThing(L, NULL, 0);
		return 1;
		
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_TILE_NOT_FOUND));
		pushThing(L, NULL, 0);
		return 1;
	}
}

int32_t LuaScriptInterface::luaGetTileItemById(lua_State* L)
{
	//getTileItemById(pos, itemId, <optional> subType)
	ScriptEnvironment* env = getScriptEnv();

	uint32_t parameters = lua_gettop(L);
	
	int32_t subType = -1;
	if(parameters > 2)
		subType = (int32_t)popNumber(L);

	int32_t itemId = (int32_t)popNumber(L);

	PositionEx pos;
	popPosition(L, pos);

	Tile* tile = g_game.getTile(pos.x, pos.y, pos.z);	
	if(!tile)
	{
		pushThing(L, NULL, 0);
		return 1;
	}

	Item* item = g_game.findItemOfType(tile, itemId, false, subType);
	if(!item)
	{
		pushThing(L, NULL, 0);
		return 1;
	}

	uint32_t uid = env->addThing(item);
	pushThing(L, item, uid);
	return 1;
}

int32_t LuaScriptInterface::luaGetTileItemByType(lua_State* L)
{
	//getTileItemByType(pos, type)

	ScriptEnvironment* env = getScriptEnv();

	uint32_t rType = (uint32_t)popNumber(L);

	if(rType >= ITEM_TYPE_LAST)
	{
		reportErrorFunc("Not a valid item type");
		pushThing(L, NULL, 0);
		return 1;
	}

	PositionEx pos;
	popPosition(L, pos);

	Tile* tile = g_game.getTile(pos.x, pos.y, pos.z);
	if(!tile)
	{
		pushThing(L, NULL, 0);
		return 1;
	}

	for(uint32_t i = 0; i < tile->getThingCount(); ++i)
	{
		if(Item* item = tile->__getThing(i)->getItem())
		{
			const ItemType& it = Item::items[item->getID()];
			if(it.type == (ItemTypes_t) rType)
			{
				uint32_t uid = env->addThing(item);
				pushThing(L, item, uid);
				return 1;
			}
		}
	}

	pushThing(L, NULL, 0);
	return 1;
}

int32_t LuaScriptInterface::luaGetTileThingByPos(lua_State* L)
{
	//getTileThingByPos(pos)
	
	PositionEx pos;
	popPosition(L, pos);

	ScriptEnvironment* env = getScriptEnv();

	Tile* tile = g_game.getTile(pos.x, pos.y, pos.z);
	if(!tile)
	{
		if(pos.stackpos == -1)
		{
			lua_pushnumber(L, -1);
			return 1;
		}
		else
		{
			pushThing(L, NULL, 0);
			return 1;
		}
	}

	if(pos.stackpos == -1)
	{
		lua_pushnumber(L, tile->getThingCount());
		return 1;
	}

	Thing* thing = tile->__getThing(pos.stackpos);
	if(!thing)
	{
		pushThing(L, NULL, 0);
		return 1;
	}

	uint32_t uid = env->addThing(thing);
	pushThing(L, thing, uid);
	return 1;
}

int32_t LuaScriptInterface::luaGetTopCreature(lua_State* L)
{
	//getTopCreature(pos)
	PositionEx pos;
	popPosition(L, pos);

	ScriptEnvironment* env = getScriptEnv();

	Tile* tile = g_game.getTile(pos.x, pos.y, pos.z);
	if(!tile)
	{
		pushThing(L, NULL, 0);
		return 1;
	}

	Thing* thing = tile->getTopCreature();
	if(!thing || !thing->getCreature())
	{
		pushThing(L, NULL, 0);
		return 1;
	}

	uint32_t uid = env->addThing(thing);
	pushThing(L, thing, uid);
	return 1;
}

int32_t LuaScriptInterface::luaDoCreateItem(lua_State* L)
{
	//doCreateItem(itemid, <optional> type/count, pos)
	//Returns uid of the created item, only works on tiles.
	uint32_t parameters = lua_gettop(L);

	PositionEx pos;
	popPosition(L, pos);

	uint32_t count = 1;
	if(parameters > 2)
		count = popNumber(L);

	uint32_t itemId = (uint32_t)popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	Tile* tile = g_game.getTile(pos.x, pos.y, pos.z);
	if(!tile)
	{
		std::ostringstream ss;
		ss << pos << " " << getErrorDesc(LUA_ERROR_TILE_NOT_FOUND);
		reportErrorFunc(ss.str().c_str());
		lua_pushboolean(L, false);
		return 1;
	}

	const ItemType& it = Item::items[itemId];

	int32_t itemCount = 1;
	int32_t subType = 1;
	if(it.hasSubType())
	{
		if(it.stackable)
			itemCount = (int32_t)std::ceil((float)count / 100);

		subType = count;
	}
	else
		itemCount = std::max<int32_t>((int32_t)1, (int32_t)count);

	while(itemCount > 0)
	{
		int32_t stackCount = std::min<int32_t>((int32_t)100, (int32_t)subType);
		Item* newItem = Item::CreateItem(itemId, stackCount);
		if(!newItem)
		{
			reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
			lua_pushboolean(L, false);
			return 1;
		}

		if(it.stackable)
			subType -= stackCount;

		ReturnValue ret = g_game.internalAddItem(tile, newItem, INDEX_WHEREEVER, FLAG_NOLIMIT);
		if(ret != RET_NOERROR)
		{
			delete newItem;
			lua_pushboolean(L, false);
			return 1;
		}

		--itemCount;
		if(itemCount == 0)
		{
			if(newItem->getParent())
			{
				uint32_t uid = env->addThing(newItem);
				lua_pushnumber(L, uid);
				return 1;
			}
			else
			{
				//stackable item stacked with existing object, newItem will be released
				lua_pushboolean(L, false);
				return 1;
			}
		}
	}
	lua_pushboolean(L, false);
	return 1;
}

int32_t LuaScriptInterface::luaDoCreateItemEx(lua_State* L)
{
	//doCreateItemEx(itemid, <optional> count/subtype)
	//Returns uid of the created item

	int32_t parameters = lua_gettop(L);

	uint32_t count = 0;
	if(parameters > 1)
		count = popNumber(L);

	uint32_t itemId = (uint32_t)popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	const ItemType& it = Item::items[itemId];
	if(it.stackable && count > 100)
	{
		reportErrorFunc("Stack count cannot be higher than 100.");
		count = 100;
	}

	Item* newItem = Item::CreateItem(itemId, count);
	if(!newItem)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	newItem->setParent(VirtualCylinder::virtualCylinder);
	env->addTempItem(env, newItem);

	uint32_t uid = env->addThing(newItem);
	lua_pushnumber(L, uid);
	return 1;
}

int32_t LuaScriptInterface::luaDoCreateTeleport(lua_State* L)
{
	//doCreateTeleport(itemid, topos, createpos)
	PositionEx createPos;
	popPosition(L, createPos);
	PositionEx toPos;
	popPosition(L, toPos);
	uint32_t itemId = (uint32_t)popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	Tile* tile = g_game.getMap()->getTile(createPos);
	if(!tile)
	{
		std::ostringstream ss;
		ss << createPos << " " << getErrorDesc(LUA_ERROR_TILE_NOT_FOUND);
		reportErrorFunc(ss.str().c_str());
		lua_pushboolean(L, false);
		return 1;
	}

	Item* newItem = Item::CreateItem(itemId);
	if(!newItem)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	Teleport* newTeleport = newItem->getTeleport();
	if(!newTeleport)
	{
		delete newItem;
		reportErrorFunc("Invalid teleport ItemID.");
		lua_pushboolean(L, false);
		return 1;
	}

	newTeleport->setDestPos(toPos);

	ReturnValue ret = g_game.internalAddItem(tile, newTeleport, INDEX_WHEREEVER, FLAG_NOLIMIT);
	if(ret != RET_NOERROR)
	{
		delete newItem;
		reportErrorFunc("Can not add Item");
		lua_pushboolean(L, false);
		return 1;
	}

	if(newItem->getParent())
	{
		uint32_t uid = env->addThing(newItem);
		lua_pushnumber(L, uid);
	}
	else
		lua_pushboolean(L, false); //stackable item stacked with existing object, newItem will be released

	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerStorageValue(lua_State* L)
{
	//getPlayerStorageValue(cid, valueid)
	uint32_t key = popNumber(L);
	uint32_t cid = popNumber(L);
	
	ScriptEnvironment* env = getScriptEnv();
	
	const Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		int32_t value;
		if(player->getStorageValue(key, value))
			lua_pushnumber(L, value);
		else
			lua_pushnumber(L, -1);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaSetPlayerStorageValue(lua_State* L)
{
	//setPlayerStorageValue(cid, valueid, newvalue)
	int32_t value = (int32_t)popNumber(L);
	uint32_t key = popNumber(L);
	uint32_t cid = popNumber(L);
	if(IS_IN_KEYRANGE(key, RESERVED_RANGE))
	{
		char error_str[45];
		sprintf(error_str, "Accessing reserved range: %d", key);
		reportErrorFunc(error_str);
		lua_pushboolean(L, false);
		return 1;
	}
	
	ScriptEnvironment* env = getScriptEnv();
	
	Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		player->addStorageValue(key,value);
		lua_pushnumber(L, 0);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoSetItemActionId(lua_State* L)
{
	//doSetItemActionId(uid, actionid)
	uint32_t actionid = popNumber(L);
	uint32_t uid = popNumber(L);
	
	ScriptEnvironment* env = getScriptEnv();
	
	Item* item = env->getItemByUID(uid);
	if(item)
	{
		item->setActionId(actionid);
		lua_pushboolean(L, true);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoSetItemText(lua_State* L)
{
	//doSetItemText(uid, text)
	std::string text = popString(L);
	uint32_t uid = popNumber(L);
	
	ScriptEnvironment* env = getScriptEnv();
	
	Item* item = env->getItemByUID(uid);
	if(item)
	{
		std::string str(text);
		item->setText(str);
		lua_pushboolean(L, true);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoSetItemSpecialDescription(lua_State* L)
{
	//doSetItemSpecialDescription(uid, desc)
	std::string desc = popString(L);
	uint32_t uid = popNumber(L);
	
	ScriptEnvironment* env = getScriptEnv();
	
	Item* item = env->getItemByUID(uid);
	if(item)
	{
		if(desc == "")
			item->resetSpecialDescription();
		else
			item->setSpecialDescription(desc);

		lua_pushboolean(L, true);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetTilePzInfo(lua_State* L)
{
	//getTilePzInfo(pos)
	PositionEx pos;
	popPosition(L, pos);
	
	Tile *tile = g_game.getMap()->getTile(pos);
	if(tile)
	{
		if(tile->hasFlag(TILESTATE_PROTECTIONZONE))
			lua_pushboolean(L, true);
		else
			lua_pushboolean(L, false);
	}
	else
	{
		std::ostringstream ss;
		ss << pos << " " << getErrorDesc(LUA_ERROR_TILE_NOT_FOUND);
		reportErrorFunc(ss.str().c_str());
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetTileHouseInfo(lua_State* L)
{
	//getTileHouseInfo(pos)
	PositionEx pos;
	popPosition(L, pos);
	
	Tile *tile = g_game.getMap()->getTile(pos);
	if(tile)
	{
		if(HouseTile* houseTile = dynamic_cast<HouseTile*>(tile))
		{
			House* house = houseTile->getHouse();
			if(house)
				lua_pushnumber(L, house->getHouseId());
			else
				lua_pushboolean(L, false);
		}
		else
			lua_pushboolean(L, false);
	}
	else
	{
		std::ostringstream ss;
		ss << pos << " " << getErrorDesc(LUA_ERROR_TILE_NOT_FOUND);
		reportErrorFunc(ss.str().c_str());
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoSummonCreature(lua_State* L)
{
	//doSummonCreature(name, pos)
	PositionEx pos;
	popPosition(L, pos);
	std::string name = popString(L);
	
	ScriptEnvironment* env = getScriptEnv();
	
	Monster* monster = Monster::createMonster(name);
	if(!monster)
	{
		std::string error_str = (std::string)"Monster name(" + name + (std::string)") not found";
		reportErrorFunc(error_str);
		lua_pushboolean(L, false);
		return 1;
	}
	
	if(!g_game.placeCreature(monster, pos))
	{
		delete monster;
		lua_pushboolean(L, false);
		return 1;
	}
	
	uint32_t cid = env->addThing((Thing*)monster);
	lua_pushnumber(L, cid);
	return 1;
}

int32_t LuaScriptInterface::luaDoRemoveCreature(lua_State* L)
{
	//doRemoveCreature(cid)
	uint32_t cid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	Creature* creature = env->getCreatureByUID(cid);
	if(creature)
	{
		Player* player = creature->getPlayer();
		if(player)
			player->kickPlayer(true); //Players will get kicked without restrictions
		else
			g_game.removeCreature(creature); //Monsters/NPCs will get removed
		lua_pushboolean(L, true);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerRemoveMoney(lua_State* L)
{
	//doPlayerRemoveMoney(cid,money)
	uint32_t money = popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();
	
	Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		if(g_game.removeMoney(player, money))
			lua_pushboolean(L, true);
		else
			lua_pushboolean(L, false);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerAddMoney(lua_State* L)
{
	//doPlayerAddMoney(cid,money)
	uint32_t money = popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();
	
	Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		g_game.addMoney(player, money);
		lua_pushboolean(L, true);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerSetTown(lua_State* L)
{
	//doPlayerSetTown(cid, townid)
	uint32_t townid = (uint32_t)popNumber(L);
	uint32_t cid = popNumber(L);
	
	ScriptEnvironment* env = getScriptEnv();
	
	Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		Town* town = Towns::getInstance().getTown(townid);
		if(town)
		{
			player->masterPos = town->getTemplePosition();
			player->setTown(townid);
			lua_pushboolean(L, true);
		}
		else
		{
			reportErrorFunc("Not found townid");
			lua_pushboolean(L, false);
		}
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerSetVocation(lua_State* L)
{
	//doPlayerSetVocation(cid,voc)
	uint32_t voc = popNumber(L);
	uint32_t cid = popNumber(L);
	
	ScriptEnvironment* env = getScriptEnv();
	
	Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		player->setVocation(voc);
		uint32_t promotedVocation = g_vocations.getPromotedVocation(player->getVocationId());
		if(promotedVocation == 0 && player->getVocationId() != promotedVocation)
			player->addStorageValue(STORAGEVALUE_PROMOTION, 1);
		lua_pushboolean(L, true);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerSetSex(lua_State* L)
{
	//doPlayerSetSex(cid,voc)
	uint32_t newSex = popNumber(L);
	uint32_t cid = popNumber(L);
	
	ScriptEnvironment* env = getScriptEnv();
	Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		player->setSex((PlayerSex_t)newSex);
		lua_pushboolean(L, true);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDebugPrint(lua_State* L)
{
	//debugPrint(text)
	std::string text = popString(L);
	reportErrorFunc(text);
	return 0;
}

int32_t LuaScriptInterface::luaDoPlayerAddSoul(lua_State* L)
{
	//doPlayerAddSoul(cid,soul)
	int32_t addsoul = popNumber(L);
	uint32_t cid = popNumber(L);
	
	ScriptEnvironment* env = getScriptEnv();
	
	Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		player->changeSoul(addsoul);
		lua_pushboolean(L, true);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerItemCount(lua_State* L)
{
	//getPlayerItemCount(cid,itemid)
	uint32_t itemId = (uint32_t)popNumber(L);
	uint32_t cid = popNumber(L);
	
	ScriptEnvironment* env = getScriptEnv();
	
	const Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		uint32_t n = player->__getItemTypeCount(itemId);
		lua_pushnumber(L, n);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetHouseOwner(lua_State* L)
{
	//getHouseOwner(houseid)
	uint32_t houseid = popNumber(L);
	
	House* house = Houses::getInstance().getHouse(houseid);
	if(house)
	{
		uint32_t owner = house->getHouseOwner();
		lua_pushnumber(L, owner);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_HOUSE_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetHouseName(lua_State* L)
{
	//getHouseName(houseid)
	uint32_t houseid = popNumber(L);
	
	House* house = Houses::getInstance().getHouse(houseid);
	if(house)
		lua_pushstring(L, house->getName().c_str());
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_HOUSE_NOT_FOUND));
		lua_pushnil(L);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetHouseEntry(lua_State* L)
{
	//getHouseEntry(houseid)
	uint32_t houseid = popNumber(L);
	
	House* house = Houses::getInstance().getHouse(houseid);
	if(house)
		pushPosition(L, house->getEntryPosition(), 0);
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_HOUSE_NOT_FOUND));
		lua_pushnil(L);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetHouseRent(lua_State* L)
{
	//getHouseRent(houseid)
	uint32_t houseid = popNumber(L);
	
	House* house = Houses::getInstance().getHouse(houseid);
	if(house)
		lua_pushnumber(L, house->getRent());
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_HOUSE_NOT_FOUND));
		lua_pushnil(L);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetHouseTown(lua_State* L)
{
	//getHouseTown(houseid)
	uint32_t houseid = popNumber(L);
	
	House* house = Houses::getInstance().getHouse(houseid);
	if(house)
		lua_pushnumber(L, house->getTownId());
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_HOUSE_NOT_FOUND));
		lua_pushnil(L);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetHouseAccessList(lua_State* L)
{
	//getHouseAccessList(houseid, listid)
	uint32_t listid = popNumber(L);
	uint32_t houseid = popNumber(L);
	
	House* house = Houses::getInstance().getHouse(houseid);
	if(house)
	{
		std::string list;
		if(house->getAccessList(listid, list))
			lua_pushstring(L, list.c_str());
		else
		{
			reportErrorFunc("No valid listid.");
			lua_pushnil(L);
		}
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_HOUSE_NOT_FOUND));
		lua_pushnil(L);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetHouseByPlayerGUID(lua_State* L)
{
	//getHouseByPlayerGUID(playerGUID)
	uint32_t guid = popNumber(L);
	
	House* house = Houses::getInstance().getHouseByPlayerId(guid);
	if(house)
		lua_pushnumber(L, house->getHouseId());
	else
		lua_pushnil(L);
	return 1;
}

int32_t LuaScriptInterface::luaGetHouseTilesSize(lua_State* L)
{
	//getHouseTilesSize(houseid)
	uint32_t houseid = popNumber(L);

	House* house = Houses::getInstance().getHouse(houseid);
	if(house)
		lua_pushnumber(L, house->getHouseTileSize());
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_HOUSE_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaSetHouseAccessList(lua_State* L)
{
	//setHouseAccessList(houseid, listid, listtext)
	std::string list = popString(L);
	uint32_t listid = popNumber(L);
	uint32_t houseid = popNumber(L);
	
	House* house = Houses::getInstance().getHouse(houseid);
	if(house)
	{
		house->setAccessList(listid, list);
		lua_pushboolean(L, true);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_HOUSE_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaSetHouseOwner(lua_State* L)
{
	//setHouseOwner(houseid, owner)
	uint32_t owner = popNumber(L);
	uint32_t houseid = popNumber(L);
	
	House* house = Houses::getInstance().getHouse(houseid);
	if(house)
	{
		house->setHouseOwner(owner);
		lua_pushboolean(L, true);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_HOUSE_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetWorldType(lua_State* L)
{
	//getWorldType()
	switch(g_game.getWorldType())
	{
		case WORLD_TYPE_NO_PVP:
			lua_pushnumber(L, 1);
			break;
		case WORLD_TYPE_PVP:
			lua_pushnumber(L, 2);
			break;
		case WORLD_TYPE_PVP_ENFORCED:
			lua_pushnumber(L, 3);
			break;
		default:
			lua_pushboolean(L, false);
			break;
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetWorldTime(lua_State* L)
{
	//getWorldTime()
	uint32_t time = g_game.getLightHour();
	lua_pushnumber(L, time);
	return 1;
}

int32_t LuaScriptInterface::luaGetWorldLight(lua_State* L)
{
	//getWorldLight()
	LightInfo lightInfo;
	g_game.getWorldLightInfo(lightInfo);
	lua_pushnumber(L, lightInfo.level);
	lua_pushnumber(L, lightInfo.color);
	return 1;
}

int32_t LuaScriptInterface::luaGetWorldCreatures(lua_State* L)
{
	//getWorldCreatures(type)
	//0 players, 1 monsters, 2 npcs, 3 all
	uint32_t type = popNumber(L);
	uint32_t value;
	switch(type)
	{
		case 0:
			value = g_game.getPlayersOnline();
			break;
		case 1:
			value = g_game.getMonstersOnline();
			break;
		case 2:
			value = g_game.getNpcsOnline();
			break;
		case 3:
			value = g_game.getCreaturesOnline();
			break;
		default:
			reportErrorFunc("Wrong creature type.");
			lua_pushboolean(L, false);
			return 1;
			break;
	}
	lua_pushnumber(L, value);
	return 1;
}

int32_t LuaScriptInterface::luaGetWorldUpTime(lua_State* L)
{
	//getWorldUpTime()
	uint32_t uptime = 0;
	Status* status = Status::getInstance();
	if(status)
		uptime = status->getUptime();
	lua_pushnumber(L, uptime);
	return 1;
}

int32_t LuaScriptInterface::luaBroadcastMessage(lua_State* L)
{
	//broadcastMessage(message, type)
	uint32_t type = MSG_STATUS_WARNING;
	int32_t parameters = lua_gettop(L);
	if(parameters >= 2)
		type = popNumber(L);

	std::string message = popString(L);
	if(g_game.broadcastMessage(message, (MessageClasses)type))
		lua_pushboolean(L, true);
	else
	{
		reportErrorFunc("Bad messageClass type.");
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerBroadcastMessage(lua_State* L)
{
	//doPlayerBroadcastMessage(cid, message, <optional> type)
	uint32_t type = SPEAK_BROADCAST;
	int32_t parameters = lua_gettop(L);
	if(parameters >= 3)
		type = popNumber(L);

	std::string message = popString(L);
	uint32_t cid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();
	Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		if(g_game.playerBroadcastMessage(player, message, (SpeakClasses)type))
			lua_pushboolean(L, true);
		else
		{
			reportErrorFunc("Bad speakClass type.");
			lua_pushboolean(L, false);
		}
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerLight(lua_State* L)
{
	//getPlayerLight(cid)
	uint32_t cid = popNumber(L);
	
	ScriptEnvironment* env = getScriptEnv();
	const Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		LightInfo lightInfo;
		player->getCreatureLight(lightInfo);
		lua_pushnumber(L, lightInfo.level);
		lua_pushnumber(L, lightInfo.color);//color
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerAddExp(lua_State* L)
{
	//doPlayerAddExp(cid,exp)
	uint64_t exp = popNumber(L);
	uint32_t cid = popNumber(L);
	
	ScriptEnvironment* env = getScriptEnv();
	Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		if(exp > 0)
		{
			player->addExperience(NULL, exp);
			lua_pushboolean(L, true);
		}
		else
			lua_pushboolean(L, false);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerSlotItem(lua_State* L)
{
	//getPlayerSlotItem(cid, slot)
	uint32_t slot = popNumber(L);
	uint32_t cid = popNumber(L);
	
	ScriptEnvironment* env = getScriptEnv();
	const Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		Thing* thing = player->__getThing(slot);
		if(thing)
		{
			uint32_t uid = env->addThing(thing);
			pushThing(L, thing, uid);
		}
		else
			pushThing(L, NULL, 0);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		pushThing(L, NULL, 0);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerItemById(lua_State* L)
{
	//getPlayerItemById(cid, deepSearch, itemId, <optional> subType)
	ScriptEnvironment* env = getScriptEnv();

	uint32_t parameters = lua_gettop(L);

	int32_t subType = -1;
	if(parameters > 3)
		subType = (int32_t)popNumber(L);

	int32_t itemId = (int32_t)popNumber(L);
	bool deepSearch = popNumber(L) == 1;
	uint32_t cid = popNumber(L);

	Player* player = env->getPlayerByUID(cid);
	if(!player)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		pushThing(L, NULL, 0);
		return 1;
	}

	Item* item = g_game.findItemOfType(player, itemId, deepSearch, subType);
	if(!item)
	{
		pushThing(L, NULL, 0);
		return 1;
	}

	uint32_t uid = env->addThing(item);
	pushThing(L, item, uid);
	return 1;
}

int32_t LuaScriptInterface::luaGetThing(lua_State* L)
{
	//getThing(uid)
	uint32_t uid = popNumber(L);
	
	ScriptEnvironment* env = getScriptEnv();
	
	Thing* thing = env->getThingByUID(uid);
	if(thing)
		pushThing(L, thing, uid);
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_THING_NOT_FOUND));
		pushThing(L, NULL, 0);
	}
	return 1;
}

int32_t LuaScriptInterface::luaQueryTileAddThing(lua_State* L)
{
	//queryTileAddThing(uid, pos, <optional> flags)
	int32_t parameters = lua_gettop(L);

	uint32_t flags = 0;
	if(parameters > 2)
		flags = popNumber(L);

	PositionEx pos;
	popPosition(L, pos);
	uint32_t uid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	Tile* tile = g_game.getTile(pos.x, pos.y, pos.z);
	if(!tile)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_TILE_NOT_FOUND));
		lua_pushnumber(L, (uint32_t)RET_NOTPOSSIBLE);
		return 1;
	}

	Thing* thing = env->getThingByUID(uid);
	if(!thing)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_THING_NOT_FOUND));
		lua_pushnumber(L, (uint32_t)RET_NOTPOSSIBLE);
		return 1;
	}

	ReturnValue ret = tile->__queryAdd(0, thing, 1, flags);
	lua_pushnumber(L, (uint32_t)ret);
	return 1;
}

int32_t LuaScriptInterface::luaGetThingPos(lua_State* L)
{
	//getThingPos(uid)
	uint32_t uid = popNumber(L);
	
	ScriptEnvironment* env = getScriptEnv();
	
	Thing* thing = env->getThingByUID(uid);
	Position pos(0, 0, 0);
	uint32_t stackpos = 0;
	if(thing)
	{
		pos = thing->getPosition();
		if(Tile* tile = thing->getTile())
			stackpos = tile->__getIndexOfThing(thing);
	}
	else
		reportErrorFunc(getErrorDesc(LUA_ERROR_THING_NOT_FOUND));
	
	pushPosition(L, pos, stackpos);
	return 1;
}

int32_t LuaScriptInterface::luaCreateCombatObject(lua_State* L)
{
	//createCombatObject()
	ScriptEnvironment* env = getScriptEnv();

	if(env->getScriptId() != EVENT_ID_LOADING)
	{
		reportErrorFunc("This function can only be used while loading the script.");
		lua_pushboolean(L, false);
		return 1;
	}

	Combat* combat = new Combat;

	if(!combat)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_COMBAT_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}
	
	uint32_t newCombatId = env->addCombatObject(combat);
	lua_pushnumber(L, newCombatId);
	return 1;
}

bool LuaScriptInterface::getArea(lua_State* L, std::list<uint32_t>& list, uint32_t& rows)
{
	rows = 0;
	uint32_t i = 0, j = 0;
	lua_pushnil(L); // first key //

	while(lua_next(L, -2) != 0){
		lua_pushnil(L);
		while(lua_next(L, -2) != 0){
			list.push_back((uint32_t)lua_tonumber(L, -1));
			
			lua_pop(L, 1); // removes `value'; keeps `key' for next iteration //
			j++;
		}

		++rows;
		
		j = 0;
		lua_pop(L, 1); // removes `value'; keeps `key' for next iteration //
		i++;
	}
	
	lua_pop(L, 1);
	return (rows != 0);
}

int32_t LuaScriptInterface::luaCreateCombatArea(lua_State* L)
{
	//createCombatArea( {area}, <optional> {extArea} )
	ScriptEnvironment* env = getScriptEnv();

	if(env->getScriptId() != EVENT_ID_LOADING)
	{
		reportErrorFunc("This function can only be used while loading the script.");
		lua_pushboolean(L, false);
		return 1;
	}

	int32_t parameters = lua_gettop(L);

	AreaCombat* area = new AreaCombat;

	if(parameters > 1)
	{
		//has extra parameter with diagonal area information

		uint32_t rowsExtArea;
		std::list<uint32_t> listExtArea;
		getArea(L, listExtArea, rowsExtArea);
		
		/*setup all possible rotations*/
		area->setupExtArea(listExtArea, rowsExtArea);
	}

	uint32_t rowsArea = 0;
	std::list<uint32_t> listArea;
	getArea(L, listArea, rowsArea);

	/*setup all possible rotations*/
	area->setupArea(listArea, rowsArea);

	uint32_t newAreaId = env->addCombatArea(area);

	lua_pushnumber(L, newAreaId);
	return 1;
}

int32_t LuaScriptInterface::luaCreateConditionObject(lua_State* L)
{
	//createConditionObject(type)
	ScriptEnvironment* env = getScriptEnv();

	if(env->getScriptId() != EVENT_ID_LOADING)
	{
		reportErrorFunc("This function can only be used while loading the script.");
		lua_pushboolean(L, false);
		return 1;
	}

	ConditionType_t type = (ConditionType_t)popNumber(L);

	Condition* condition = Condition::createCondition(CONDITIONID_COMBAT, type, 0, 0);
	if(condition){
		uint32_t newConditionId = env->addConditionObject(condition);
		lua_pushnumber(L, newConditionId);
	}
	else{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CONDITION_NOT_FOUND));
		lua_pushboolean(L, false);
	}

	return 1;
}

int32_t LuaScriptInterface::luaSetCombatArea(lua_State* L)
{
	//setCombatArea(combat, area)
	uint32_t areaId = popNumber(L);
	uint32_t combatId = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	if(env->getScriptId() != EVENT_ID_LOADING){
		reportError(__FUNCTION__, "This function can only be used while loading the script.");
		lua_pushboolean(L, false);
		return 1;
	}

	Combat* combat = env->getCombatObject(combatId);

	if(!combat){
		reportErrorFunc(getErrorDesc(LUA_ERROR_COMBAT_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	const AreaCombat* area = env->getCombatArea(areaId);
	if(!area){
		reportErrorFunc(getErrorDesc(LUA_ERROR_AREA_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	combat->setArea(new AreaCombat(*area));

	lua_pushboolean(L, true);
	return 1;
}

int32_t LuaScriptInterface::luaSetCombatCondition(lua_State* L)
{
	//setCombatCondition(combat, condition)
	uint32_t conditionId = popNumber(L);
	uint32_t combatId = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	if(env->getScriptId() != EVENT_ID_LOADING){
		reportError(__FUNCTION__, "This function can only be used while loading the script.");
		lua_pushboolean(L, false);
		return 1;
	}

	Combat* combat = env->getCombatObject(combatId);

	if(!combat){
		reportErrorFunc(getErrorDesc(LUA_ERROR_COMBAT_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	const Condition* condition = env->getConditionObject(conditionId);
	if(!condition){
		reportErrorFunc(getErrorDesc(LUA_ERROR_CONDITION_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	combat->setCondition(condition->clone());

	lua_pushboolean(L, true);
	return 1;
}

int32_t LuaScriptInterface::luaSetCombatParam(lua_State* L)
{
	//setCombatParam(combat, key, value)
	uint32_t value = popNumber(L);
	CombatParam_t key = (CombatParam_t)popNumber(L);
	uint32_t combatId = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	if(env->getScriptId() != EVENT_ID_LOADING){
		reportError(__FUNCTION__, "This function can only be used while loading the script.");
		lua_pushboolean(L, false);
		return 1;
	}

	Combat* combat = env->getCombatObject(combatId);

	if(combat){
		combat->setParam(key, value);
		lua_pushboolean(L, true);
	}
	else{
		reportErrorFunc(getErrorDesc(LUA_ERROR_COMBAT_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaSetConditionParam(lua_State* L)
{
	//setConditionParam(condition, key, value)
	int32_t value = (int32_t)popNumber(L);
	ConditionParam_t key = (ConditionParam_t)popNumber(L);
	uint32_t conditionId = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	if(env->getScriptId() != EVENT_ID_LOADING){
		reportError(__FUNCTION__, "This function can only be used while loading the script.");
		lua_pushboolean(L, false);
		return 1;
	}

	Condition* condition = env->getConditionObject(conditionId);

	if(condition){
		condition->setParam(key, value);
		lua_pushboolean(L, true);
	}
	else{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CONDITION_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaAddDamageCondition(lua_State* L)
{
	//addDamageCondition(condition, rounds, time, value)
	int32_t value = (int32_t)popNumber(L);
	int32_t time = (int32_t)popNumber(L);
	int32_t rounds = (int32_t)popNumber(L);
	uint32_t conditionId = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	if(env->getScriptId() != EVENT_ID_LOADING){
		reportError(__FUNCTION__, "This function can only be used while loading the script.");
		lua_pushboolean(L, false);
		return 1;
	}

	ConditionDamage* condition = dynamic_cast<ConditionDamage*>(env->getConditionObject(conditionId));

	if(condition){
		condition->addDamage(rounds, time, value);
		lua_pushboolean(L, true);
	}
	else{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CONDITION_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaAddOutfitCondition(lua_State* L)
{
	//addOutfitCondition(condition, lookTypeEx, lookType, lookHead, lookBody, lookLegs, lookFeet)
	Outfit_t outfit;
	outfit.lookFeet = popNumber(L);
	outfit.lookLegs = popNumber(L);
	outfit.lookBody = popNumber(L);
	outfit.lookHead = popNumber(L);
	outfit.lookType = popNumber(L);
	outfit.lookTypeEx = popNumber(L);
	uint32_t conditionId = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	if(env->getScriptId() != EVENT_ID_LOADING){
		reportError(__FUNCTION__, "This function can only be used while loading the script.");
		lua_pushboolean(L, false);
		return 1;
	}

	ConditionOutfit* condition = dynamic_cast<ConditionOutfit*>(env->getConditionObject(conditionId));

	if(condition){
		condition->addOutfit(outfit);
		lua_pushboolean(L, true);
	}
	else{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CONDITION_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaSetCombatCallBack(lua_State* L)
{
	//setCombatCallBack(combat, key, function_name)
	std::string function = popString(L);
	CallBackParam_t key = (CallBackParam_t)popNumber(L);
	uint32_t combatId = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();
	
	if(env->getScriptId() != EVENT_ID_LOADING)
	{
		reportErrorFunc("This function can only be used while loading the script.");
		lua_pushboolean(L, false);
		return 1;
	}
	
	Combat* combat = env->getCombatObject(combatId);

	if(!combat){
		reportErrorFunc(getErrorDesc(LUA_ERROR_COMBAT_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}
	
	LuaScriptInterface* scriptInterface = env->getScriptInterface();
	
	combat->setCallback(key);
	CallBack* callback = combat->getCallback(key);
	if(!callback)
	{
		std::ostringstream ss;
		ss << key << " is not a valid callback key.";
		reportErrorFunc(ss.str());
		lua_pushboolean(L, false);
		return 1;
	}
	
	if(!callback->loadCallBack(scriptInterface, function))
	{
		reportErrorFunc("Can not load callback");
		lua_pushboolean(L, false);
		return 1;
	}

	lua_pushboolean(L, true);
	return 1;
}

int32_t LuaScriptInterface::luaSetCombatFormula(lua_State* L)
{
	//setCombatFormula(combat, type, mina, minb, maxa, maxb)
	ScriptEnvironment* env = getScriptEnv();

	if(env->getScriptId() != EVENT_ID_LOADING){
		reportError(__FUNCTION__, "This function can only be used while loading the script.");
		lua_pushboolean(L, false);
		return 1;
	}

	double maxb = popFloatNumber(L);
	double maxa = popFloatNumber(L);
	double minb = popFloatNumber(L);
	double mina = popFloatNumber(L);

	formulaType_t type = (formulaType_t)popNumber(L);
	uint32_t combatId = popNumber(L);

	Combat* combat = env->getCombatObject(combatId);

	if(combat){
		combat->setPlayerCombatValues(type, mina, minb, maxa, maxb);
		
		lua_pushboolean(L, true);
	}
	else{
		reportErrorFunc(getErrorDesc(LUA_ERROR_COMBAT_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaSetConditionFormula(lua_State* L)
{
	//setConditionFormula(condition, mina, minb, maxa, maxb)
	ScriptEnvironment* env = getScriptEnv();

	if(env->getScriptId() != EVENT_ID_LOADING){
		reportError(__FUNCTION__, "This function can only be used while loading the script.");
		lua_pushboolean(L, false);
		return 1;
	}

	double maxb = popFloatNumber(L);
	double maxa = popFloatNumber(L);
	double minb = popFloatNumber(L);
	double mina = popFloatNumber(L);

	uint32_t conditionId = popNumber(L);

	ConditionSpeed* condition = dynamic_cast<ConditionSpeed*>(env->getConditionObject(conditionId));
	
	if(condition){
		condition->setFormulaVars(mina, minb, maxa, maxb);
		lua_pushboolean(L, true);
	}
	else{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CONDITION_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoCombat(lua_State* L)
{
	//doCombat(cid, combat, param)
	ScriptEnvironment* env = getScriptEnv();
	
	LuaVariant var = popVariant(L);
	uint32_t combatId = (uint32_t)popNumber(L);
	uint32_t cid = (uint32_t)popNumber(L);
	
	Creature* creature = NULL;
	
	if(cid != 0)
	{
		creature = env->getCreatureByUID(cid);

		if(!creature)
		{
			reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
			lua_pushboolean(L, false);
			return 1;
		}
	}

	const Combat* combat = env->getCombatObject(combatId);

	if(!combat)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_COMBAT_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	if(var.type == VARIANT_NONE)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_VARIANT_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	switch(var.type)
	{
		case VARIANT_NUMBER:
		{
			Creature* target = g_game.getCreatureByID(var.number);

			if(!target)
			{
				lua_pushboolean(L, false);
				return 1;
			}

			if(combat->hasArea())
			{
				combat->doCombat(creature, target->getPosition());
				//std::clog << "Combat->hasArea()" << std::endl;
			}
			else
				combat->doCombat(creature, target);

			break;
		}

		case VARIANT_POSITION:
		{
			combat->doCombat(creature, var.pos);
			break;
		}

		case VARIANT_TARGETPOSITION:
		{
			if(combat->hasArea())
				combat->doCombat(creature, var.pos);
			else
			{
				combat->postCombatEffects(creature, var.pos);
				g_game.addMagicEffect(var.pos, NM_ME_POFF);
			}
			break;
		}

		case VARIANT_STRING:
		{
			Player* target = g_game.getPlayerByName(var.text);
			if(!target)
			{
				lua_pushboolean(L, false);
				return 1;
			}

			combat->doCombat(creature, target);
			break;
		}

		default:
		{
			reportErrorFunc(getErrorDesc(LUA_ERROR_VARIANT_UNKNOWN));
			lua_pushboolean(L, false);
			return 1;
			break;
		}
	}

	lua_pushboolean(L, true);
	return 1;
}

int32_t LuaScriptInterface::luaDoAreaCombatHealth(lua_State* L)
{
	//doAreaCombatHealth(cid, type, pos, area, min, max, effect)
	uint8_t effect = (uint8_t)popNumber(L);
	int32_t maxChange = (int32_t)popNumber(L);
	int32_t minChange = (int32_t)popNumber(L);
	uint32_t areaId = popNumber(L);
	
	PositionEx pos;
	popPosition(L, pos);

	CombatType_t combatType = (CombatType_t)popNumber(L);
	uint32_t cid = (uint32_t)popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	Creature* creature = NULL;
	
	if(cid != 0){
		creature = env->getCreatureByUID(cid);

		if(!creature){
			reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
			lua_pushboolean(L, false);
			return 1;
		}
	}

	const AreaCombat* area = env->getCombatArea(areaId);
	if(area || areaId == 0){
		CombatParams params;
		params.combatType = combatType;
		params.impactEffect = effect;
		Combat::doCombatHealth(creature, pos, area, minChange, maxChange, params);

		lua_pushboolean(L, true);
	}
	else{
		reportErrorFunc(getErrorDesc(LUA_ERROR_AREA_NOT_FOUND));
		lua_pushboolean(L, false);
	}

	return 1;
}

int32_t LuaScriptInterface::luaDoTargetCombatHealth(lua_State* L)
{
	//doTargetCombatHealth(cid, target, type, min, max, effect)
	uint8_t effect = (uint8_t)popNumber(L);
	int32_t maxChange = (int32_t)popNumber(L);
	int32_t minChange = (int32_t)popNumber(L);
	CombatType_t combatType = (CombatType_t)popNumber(L);
	uint32_t targetCid = popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	Creature* creature = NULL;
	
	if(cid != 0){
		creature = env->getCreatureByUID(cid);

		if(!creature){
			reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
			lua_pushboolean(L, false);
			return 1;
		}
	}

	Creature* target = env->getCreatureByUID(targetCid);
	if(target){
		CombatParams params;
		params.combatType = combatType;
		params.impactEffect = effect;
		Combat::doCombatHealth(creature, target, minChange, maxChange, params);

		lua_pushboolean(L, true);
	}
	else{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoAreaCombatMana(lua_State* L)
{
	//doAreaCombatMana(cid, pos, area, min, max, effect)
	uint8_t effect = (uint8_t)popNumber(L);
	int32_t maxChange = (int32_t)popNumber(L);
	int32_t minChange = (int32_t)popNumber(L);
	uint32_t areaId = popNumber(L);

	PositionEx pos;
	popPosition(L, pos);

	uint32_t cid = (uint32_t)popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	Creature* creature = NULL;
	
	if(cid != 0){
		creature = env->getCreatureByUID(cid);

		if(!creature){
			reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
			lua_pushboolean(L, false);
			return 1;
		}
	}

	const AreaCombat* area = env->getCombatArea(areaId);
	if(area || areaId == 0){
		CombatParams params;
		params.impactEffect = effect;
		Combat::doCombatMana(creature, pos, area, minChange, maxChange, params);
		
		lua_pushboolean(L, true);
	}
	else{
		reportErrorFunc(getErrorDesc(LUA_ERROR_AREA_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoTargetCombatMana(lua_State* L)
{
	//doTargetCombatMana(cid, target, min, max, effect)
	uint8_t effect = (uint8_t)popNumber(L);
	int32_t maxChange = (int32_t)popNumber(L);
	int32_t minChange = (int32_t)popNumber(L);
	uint32_t targetCid = popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	Creature* creature = NULL;
	
	if(cid != 0){
		creature = env->getCreatureByUID(cid);

		if(!creature){
			reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
			lua_pushboolean(L, false);
			return 1;
		}
	}

	Creature* target = env->getCreatureByUID(targetCid);
	if(target){
		CombatParams params;
		params.impactEffect = effect;
		Combat::doCombatMana(creature, target, minChange, maxChange, params);
		
		lua_pushboolean(L, true);
	}
	else{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoAreaCombatCondition(lua_State* L)
{
	//doAreaCombatCondition(cid, pos, area, condition, effect)
	uint8_t effect = (uint8_t)popNumber(L);
	uint32_t conditionId = popNumber(L);
	uint32_t areaId = popNumber(L);
	PositionEx pos;
	popPosition(L, pos);
	uint32_t cid = (uint32_t)popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	Creature* creature = NULL;
	
	if(cid != 0){
		creature = env->getCreatureByUID(cid);
		if(!creature){
			reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
			lua_pushboolean(L, false);
			return 1;
		}
	}

	const Condition* condition = env->getConditionObject(conditionId);
	if(condition){
		const AreaCombat* area = env->getCombatArea(areaId);
		
		if(area || areaId == 0){
			CombatParams params;
			params.impactEffect = effect;
			params.condition = condition;
			Combat::doCombatCondition(creature, pos, area, params);
			
			lua_pushboolean(L, true);
		}
		else{
			reportErrorFunc(getErrorDesc(LUA_ERROR_AREA_NOT_FOUND));
			lua_pushboolean(L, false);
		}
	}
	else{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CONDITION_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoTargetCombatCondition(lua_State* L)
{
	//doTargetCombatCondition(cid, target, condition, effect)
	uint8_t effect = (uint8_t)popNumber(L);
	uint32_t conditionId = popNumber(L);
	uint32_t targetCid = popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	Creature* creature = NULL;

	if(cid != 0){
		creature = env->getCreatureByUID(cid);

		if(!creature){
			reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
			lua_pushboolean(L, false);
			return 1;
		}
	}

	Creature* target = env->getCreatureByUID(targetCid);
	if(target){
		const Condition* condition = env->getConditionObject(conditionId);
		if(condition){
			CombatParams params;
			params.impactEffect = effect;
			params.condition = condition;
			Combat::doCombatCondition(creature, target, params);
			
			lua_pushboolean(L, true);
		}
		else{
			reportErrorFunc(getErrorDesc(LUA_ERROR_CONDITION_NOT_FOUND));
			lua_pushboolean(L, false);
		}
	}
	else{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoAreaCombatDispel(lua_State* L)
{
	//doAreaCombatDispel(cid, pos, area, type, effect)
	uint8_t effect = (uint8_t)popNumber(L);
	ConditionType_t dispelType = (ConditionType_t)popNumber(L);
	uint32_t areaId = popNumber(L);
	PositionEx pos;
	popPosition(L, pos);
	uint32_t cid = (uint32_t)popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	Creature* creature = NULL;
	
	if(cid != 0){
		creature = env->getCreatureByUID(cid);

		if(!creature){
			reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
			lua_pushboolean(L, false);
			return 1;
		}
	}

	const AreaCombat* area = env->getCombatArea(areaId);
	if(area || areaId == 0){
		CombatParams params;
		params.impactEffect = effect;
		params.dispelType = dispelType;
		Combat::doCombatDispel(creature, pos, area, params);

		lua_pushboolean(L, true);
	}
	else{
		reportErrorFunc(getErrorDesc(LUA_ERROR_AREA_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoTargetCombatDispel(lua_State* L)
{
	//doTargetCombatDispel(cid, target, type, effect)
	uint8_t effect = (uint8_t)popNumber(L);
	ConditionType_t dispelType = (ConditionType_t)popNumber(L);
	uint32_t targetCid = popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	Creature* creature = NULL;

	if(cid != 0){
		creature = env->getCreatureByUID(cid);

		if(!creature){
			reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
			lua_pushboolean(L, false);
			return 1;
		}
	}

	Creature* target = env->getCreatureByUID(targetCid);
	if(target){
		CombatParams params;
		params.impactEffect = effect;
		params.dispelType = dispelType;
		Combat::doCombatDispel(creature, target, params);

		lua_pushboolean(L, true);
	}
	else{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}

	return 1;
}

int32_t LuaScriptInterface::luaDoChallengeCreature(lua_State* L)
{
	//doChallengeCreature(cid, target)
	uint32_t targetCid = popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	Creature* creature = env->getCreatureByUID(cid);

	if(!creature){
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	Creature* target = env->getCreatureByUID(targetCid);
	if(!target)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	target->challengeCreature(creature);
	lua_pushboolean(L, true);
	return 1;
}

int32_t LuaScriptInterface::luaDoConvinceCreature(lua_State* L)
{
	//doConvinceCreature(cid, target)
	uint32_t targetCid = popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	Creature* creature = env->getCreatureByUID(cid);

	if(!creature){
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	Creature* target = env->getCreatureByUID(targetCid);
	if(!target)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	target->convinceCreature(creature);
	lua_pushboolean(L, true);
	return 1;
}

int32_t LuaScriptInterface::luaGetMonsterTargetList(lua_State* L)
{
	//getMonsterTargetList(cid)
	uint32_t cid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	Creature* creature = env->getCreatureByUID(cid);
	if(!creature)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	Monster* monster = creature->getMonster();
	if(!monster)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	lua_newtable(L);
	uint32_t i = 0;
	const CreatureList& targetList = monster->getTargetList();
	for(CreatureList::const_iterator it = targetList.begin(); it != targetList.end(); ++it)
	{
		if(monster->isTarget(*it))
		{
			uint32_t targetCid = env->addThing(*it);
			lua_pushnumber(L, i);
			lua_pushnumber(L, targetCid);
			lua_settable(L, -3);
			++i;
		}
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetMonsterFriendList(lua_State* L)
{
	//getMonsterFriendList(cid)

	uint32_t cid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	Creature* creature = env->getCreatureByUID(cid);
	if(!creature)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	Monster* monster = creature->getMonster();
	if(!monster)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	lua_newtable(L);
	uint32_t i = 0;
	Creature* friendCreature;
	const CreatureList& friendList = monster->getFriendList();
	for(CreatureList::const_iterator it = friendList.begin(); it != friendList.end(); ++it)
	{
		friendCreature = *it;
		if(!friendCreature->isRemoved() && friendCreature->getPosition().z == monster->getPosition().z)
		{
			uint32_t friendCid = env->addThing(*it);
			lua_pushnumber(L, i);
			lua_pushnumber(L, friendCid);
			lua_settable(L, -3);
			++i;
		}
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoSetMonsterTarget(lua_State* L)
{
	//doSetMonsterTarget(cid, target)
	uint32_t targetCid = popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	Creature* creature = env->getCreatureByUID(cid);
	if(!creature)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	Monster* monster = creature->getMonster();
	if(!monster)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	Creature* target = env->getCreatureByUID(targetCid);
	if(!target)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	if(!monster->isSummon())
		monster->selectTarget(target);

	lua_pushboolean(L, true);
	return 1;
}

int32_t LuaScriptInterface::luaDoMonsterChangeTarget(lua_State* L)
{
	//doMonsterChangeTarget(cid)
	uint32_t cid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	Creature* creature = env->getCreatureByUID(cid);
	if(!creature)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	Monster* monster = creature->getMonster();
	if(!monster)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	if(!monster->isSummon())
		monster->searchTarget(TARGETSEARCH_RANDOM);

	lua_pushboolean(L, true);
	return 1;
}

int32_t LuaScriptInterface::luaDoAddCondition(lua_State* L)
{
	//doAddCondition(cid, condition)

	uint32_t conditionId = popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	Creature* creature = env->getCreatureByUID(cid);
	if(!creature)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	Condition* condition = env->getConditionObject(conditionId);
	if(!condition)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CONDITION_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	creature->addCondition(condition->clone());
	lua_pushboolean(L, true);
	return 1;
}

int32_t LuaScriptInterface::luaDoRemoveCondition(lua_State* L)
{
	//doRemoveCondition(cid, type)

	ConditionType_t conditionType = (ConditionType_t)popNumber(L);
	uint32_t cid = popNumber(L);
	
	ScriptEnvironment* env = getScriptEnv();

	Creature* creature = env->getCreatureByUID(cid);
	if(!creature)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	Condition* condition = creature->getCondition(conditionType, CONDITIONID_COMBAT);
	if(!condition)
		condition = creature->getCondition(conditionType, CONDITIONID_DEFAULT);

	if(condition)
		creature->removeCondition(condition);

	lua_pushboolean(L, true);
	return 1;
}

int32_t LuaScriptInterface::luaNumberToVariant(lua_State* L)
{
	//numberToVariant(number)
	LuaVariant var;
	var.type = VARIANT_NUMBER;
	var.number = popNumber(L);

	LuaScriptInterface::pushVariant(L, var);
	return 1;
}

int32_t LuaScriptInterface::luaStringToVariant(lua_State* L)
{
	//stringToVariant(string)
	LuaVariant var;
	var.type = VARIANT_STRING;
	var.text = popString(L);

	LuaScriptInterface::pushVariant(L, var);
	return 1;
}

int32_t LuaScriptInterface::luaPositionToVariant(lua_State* L)
{
	//positionToVariant(pos)
	LuaVariant var;
	var.type = VARIANT_POSITION;
	popPosition(L, var.pos);

	LuaScriptInterface::pushVariant(L, var);
	return 1;
}

int32_t LuaScriptInterface::luaTargetPositionToVariant(lua_State* L)
{
	//targetPositionToVariant(pos)
	LuaVariant var;
	var.type = VARIANT_TARGETPOSITION;
	popPosition(L, var.pos);

	LuaScriptInterface::pushVariant(L, var);
	return 1;
}

int32_t LuaScriptInterface::luaVariantToNumber(lua_State* L)
{
	//variantToNumber(var)
	LuaVariant var = popVariant(L);

	uint32_t number = 0;
	if(var.type == VARIANT_NUMBER){
		number = var.number;
	}

	lua_pushnumber(L, number);
	return 1;
}

int32_t LuaScriptInterface::luaVariantToString(lua_State* L)
{
	//variantToString(var)
	LuaVariant var = popVariant(L);

	std::string text = "";
	if(var.type == VARIANT_STRING){
		text = var.text;
	}

	lua_pushstring(L, text.c_str());
	return 1;
}

int32_t LuaScriptInterface::luaVariantToPosition(lua_State* L)
{
	//luaVariantToPosition(var)
	LuaVariant var = popVariant(L);

	PositionEx pos(0, 0, 0, 0);
	if(var.type == VARIANT_POSITION || var.type == VARIANT_TARGETPOSITION){
		pos = var.pos;
	}

	pushPosition(L, pos, pos.stackpos);
	return 1;
}

int32_t LuaScriptInterface::luaDoChangeSpeed(lua_State* L)
{
	//doChangeSpeed(cid, delta)
	int32_t delta = (int32_t)popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	Creature* creature = env->getCreatureByUID(cid);

	if(creature){
		g_game.changeSpeed(creature, delta);
		lua_pushboolean(L, true);
	}
	else{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaSetCreatureOutfit(lua_State* L)
{
	//doSetCreatureOutfit(cid, outfit, time)
	int32_t time = (int32_t)popNumber(L);
	Outfit_t outfit;
	outfit.lookType = getField(L, "lookType");
	outfit.lookHead = getField(L, "lookHead");
	outfit.lookBody = getField(L, "lookBody");
	outfit.lookLegs = getField(L, "lookLegs");
	outfit.lookFeet = getField(L, "lookFeet");
	outfit.lookAddons = getField(L, "lookAddons");
	lua_pop(L, 1);

	uint32_t cid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	Creature* creature = env->getCreatureByUID(cid);

	if(creature){
		ReturnValue ret = Spell::CreateIllusion(creature, outfit, time);
		if(ret == RET_NOERROR){
			lua_pushboolean(L, true);
		}
		else{
			lua_pushboolean(L, false);
		}
	}
	else{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetCreatureOutfit(lua_State* L)
{
	//getCreatureOutfit(cid)
	uint32_t cid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	Creature* creature = env->getCreatureByUID(cid);

	if(creature){
		const Outfit_t outfit = creature->getCurrentOutfit();

		lua_newtable(L);
		setField(L, "lookType", outfit.lookType);
		setField(L, "lookHead", outfit.lookHead);
		setField(L, "lookBody", outfit.lookBody);
		setField(L, "lookLegs", outfit.lookLegs);
		setField(L, "lookFeet", outfit.lookFeet);
		setField(L, "lookAddons", outfit.lookAddons);

		return 1;
	}
	else{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}

	return 1;
}

int32_t LuaScriptInterface::luaSetMonsterOutfit(lua_State* L)
{
	//doSetMonsterOutfit(cid, name, time)
	int32_t time = (int32_t)popNumber(L);
	std::string name = popString(L);
	uint32_t cid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	Creature* creature = env->getCreatureByUID(cid);

	if(creature){
		ReturnValue ret = Spell::CreateIllusion(creature, name, time);
		if(ret == RET_NOERROR){
			lua_pushboolean(L, true);
		}
		else{
			lua_pushboolean(L, false);
		}
	}
	else{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaSetItemOutfit(lua_State* L)
{
	//doSetItemOutfit(cid, item, time)
	int32_t time = (int32_t)popNumber(L);
	uint32_t item = (uint32_t)popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	Creature* creature = env->getCreatureByUID(cid);

	if(creature){
		ReturnValue ret = Spell::CreateIllusion(creature, item, time);
		if(ret == RET_NOERROR){
			lua_pushboolean(L, true);
		}
		else{
			lua_pushboolean(L, false);
		}
	}
	else{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetGlobalStorageValue(lua_State* L)
{
	//getGlobalStorageValue(valueid)
	uint32_t key = popNumber(L);
	
	ScriptEnvironment* env = getScriptEnv();
	
	int32_t value;
	if(env->getGlobalStorageValue(key, value)){
		lua_pushnumber(L, value);
	}
	else{
		lua_pushnumber(L, -1);
	}
	return 1;
}

int32_t LuaScriptInterface::luaSetGlobalStorageValue(lua_State* L)
{
	//setGlobalStorageValue(valueid, newvalue)
	int32_t value = (int32_t)popNumber(L);
	uint32_t key = popNumber(L);
	
	ScriptEnvironment* env = getScriptEnv();
	env->addGlobalStorageValue(key,value);
	lua_pushnumber(L,0);
	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerDepotItems(lua_State* L)
{
	//getPlayerDepotItems(cid, depotid)
	uint32_t depotid = popNumber(L);
	uint32_t cid = popNumber(L);
	
	ScriptEnvironment* env = getScriptEnv();
	Player* player = env->getPlayerByUID(cid);
	if(player){
		const Depot* depot = player->getDepot(depotid, true);
		if(depot){
			lua_pushnumber(L, depot->getItemHoldingCount());
		}
		else{
			reportErrorFunc("Depot not found");
			lua_pushboolean(L, false);
		}
	}
	else{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerSetGuildRank(lua_State* L)
{
	//doPlayerSetGuildRank(cid, rank)
	std::string rank = popString(L);
	uint32_t cid = popNumber(L);
	
	ScriptEnvironment* env = getScriptEnv();
	Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		player->setGuildRank(rank);
		lua_pushboolean(L, true);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerSetGuildNick(lua_State* L)
{
	//doPlayerSetGuildNick(cid, nick)
	std::string nick = popString(L);
	uint32_t cid = popNumber(L);
	
	ScriptEnvironment* env = getScriptEnv();
	Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		player->setGuildNick(nick);
		lua_pushboolean(L, true);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetGuildId(lua_State* L)
{
	//getGuildId(guild_name)
	std::string name = popString(L);
		
	uint32_t guildId;
	if(IOGuild::getInstance()->getGuildIdByName(guildId, name))
	{
		lua_pushnumber(L, guildId);
	}
	else
	{
		reportErrorFunc("Guild not found");
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoMoveCreature(lua_State* L)
{
	//doMoveCreature(cid, direction)
	uint32_t direction = popNumber(L);
	uint32_t cid = popNumber(L);
	
	switch(direction)
	{
		case NORTH:
		case SOUTH:
		case WEST:
		case EAST:
		case SOUTHWEST:
		case NORTHWEST:
		case NORTHEAST:
		case SOUTHEAST:
			break;
		default:
			reportErrorFunc("No valid direction");
			lua_pushboolean(L, false);
			return 1;
			break;
	}
	
	ScriptEnvironment* env = getScriptEnv();
	
	Creature* creature = env->getCreatureByUID(cid);
	if(creature)
	{
		ReturnValue ret = g_game.internalMoveCreature(creature, (Direction)direction, FLAG_NOLIMIT);
		lua_pushnumber(L, ret);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaIsPlayer(lua_State* L)
{
	//isPlayer(cid)
	uint32_t cid = popNumber(L);
	
	ScriptEnvironment* env = getScriptEnv();
	
	if(env->getPlayerByUID(cid))
		lua_pushboolean(L, true);
	else
		lua_pushboolean(L, false);

	return 1;
}

int32_t LuaScriptInterface::luaIsCreature(lua_State* L)
{
	//isCreature(cid)
	uint32_t cid = popNumber(L);
	
	ScriptEnvironment* env = getScriptEnv();
	
	if(env->getCreatureByUID(cid))
		lua_pushboolean(L, true);
	else
		lua_pushboolean(L, false);

	return 1;
}

int32_t LuaScriptInterface::luaIsContainer(lua_State* L)
{
	//isContainer(uid)
	uint32_t uid = popNumber(L);
	
	ScriptEnvironment* env = getScriptEnv();
	
	if(env->getContainerByUID(uid))
		lua_pushboolean(L, true);
	else
		lua_pushboolean(L, false);

	return 1;
}

int32_t LuaScriptInterface::luaIsCorpse(lua_State* L)
{
	//isCorpse(uid)
	uint32_t uid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	Item* item = env->getItemByUID(uid);
	if(item)
	{
		const ItemType& it = Item::items[item->getID()];
		lua_pushboolean(L, (it.corpseType != RACE_NONE ? true : false));
	}
	else
		lua_pushboolean(L, false);
	return 1;
}

int32_t LuaScriptInterface::luaIsMoveable(lua_State* L)
{
	//isMoveable(uid)
	//isMovable(uid)
	uint32_t uid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	Thing* thing = env->getThingByUID(uid);

	if(thing && thing->isPushable())
		lua_pushboolean(L, true);
	else
		lua_pushboolean(L, false);
	return 1;
}

int32_t LuaScriptInterface::luaGetPlayersByAccountNumber(lua_State* L)
{
	//getPlayersByAccountNumber(accountNumber)
	uint32_t accno = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	lua_newtable(L);
	PlayerVector players = g_game.getPlayersByAccount(accno);
	int index = 0;
	PlayerVector::iterator iter = players.begin();
	while(iter != players.end())
	{
		uint32_t cid = env->addThing(*iter);

		lua_pushnumber(L, index);
		lua_pushnumber(L, cid);
		lua_settable(L, -3);

		++iter, ++index;
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetIPByPlayerName(lua_State* L)
{
	//getIPByPlayerName(playerName)
	std::string name = popString(L);
	if(Player* player = g_game.getPlayerByName(name))
		lua_pushnumber(L, player->getIP());
	else
		lua_pushboolean(L, false);

	return 1;
}

int32_t LuaScriptInterface::luaGetPlayersByIPAddress(lua_State* L)
{
	//getPlayersByIPAddress(ip[, mask])
	int parameters = lua_gettop(L);

	uint32_t mask = 0xFFFFFFFF;
	if(parameters > 1)
		mask = (uint32_t)popNumber(L);
	uint32_t ip = (uint32_t)popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	lua_newtable(L);
	PlayerVector players = g_game.getPlayersByIP(ip);
	int index = 0;
	PlayerVector::iterator iter = players.begin();
	while(iter != players.end())
	{
		uint32_t cid = env->addThing(*iter);

		lua_pushnumber(L, index);
		lua_pushnumber(L, cid);
		lua_settable(L, -3);

		++iter, ++index;
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetAccountNumberByPlayerName(lua_State* L)
{
	//getAccountNumberByPlayerName(name)
	std::string name = popString(L);

	Player* player = g_game.getPlayerByName(name);
	uint32_t value = 0;

	if(player)
		value = player->getAccount();
	else
		value = IOLoginData::getInstance()->getAccountNumberByName(name);

	lua_pushnumber(L, value);
	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerGUIDByName(lua_State* L)
{
	//getPlayerGUIDByName(name)
	std::string name = popString(L);

	Player* player = g_game.getPlayerByName(name);
	uint32_t value = 0;

	if(player)
		value = player->getGUID();
	else
	{
		uint32_t guid;
		std::string strName(name);
		if(IOLoginData::getInstance()->getGuidByName(guid, strName))
			value = guid;
	}
	lua_pushnumber(L, value);
	return 1;
}

int32_t LuaScriptInterface::luaRegisterCreatureEvent(lua_State* L)
{
	//registerCreatureEvent(cid, name)
	std::string name = popString(L);
	uint32_t cid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	Creature* creature = env->getCreatureByUID(cid);
	if(creature)
	{
		creature->registerCreatureEvent(name);
		lua_pushboolean(L, true);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetContainerSize(lua_State* L)
{
	//getContainerSize(uid)
	uint32_t uid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	if(Container* container = env->getContainerByUID(uid))
		lua_pushnumber(L, container->size());
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CONTAINER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetContainerCap(lua_State* L)
{
	//getContainerCap(uid)
	uint32_t uid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	if(Container* container = env->getContainerByUID(uid))
		lua_pushnumber(L, container->capacity());
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CONTAINER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetContainerItem(lua_State* L)
{
	//getContainerItem(uid, slot)
	uint32_t slot = popNumber(L);
	uint32_t uid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	if(Container* container = env->getContainerByUID(uid))
	{
		Item* item = container->getItem(slot);
		if(item)
		{
			uint32_t uid = env->addThing(item);
			pushThing(L, item, uid);
		}
		else
			pushThing(L, NULL, 0);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CONTAINER_NOT_FOUND));
		pushThing(L, NULL, 0);
	}
	return 1;
	
}

int32_t LuaScriptInterface::luaDoAddContainerItem(lua_State* L)
{
	//doAddContainerItem(uid, itemid, <optional> count/subtype)
	int32_t parameters = lua_gettop(L);

	uint32_t count = 1;
	if(parameters > 2)
		count = popNumber(L);

	uint16_t itemId = (uint16_t)popNumber(L);
	uint32_t uid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();
	Container* container = env->getContainerByUID(uid);
	if(!container)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CONTAINER_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	const ItemType& it = Item::items[itemId];

	int32_t itemCount = 1;
	int32_t subType = 1;
	if(it.hasSubType())
	{
		if(it.stackable)
			itemCount = (int32_t)std::ceil((float)count / 100);

		subType = count;
	}
	else
		itemCount = std::max<int32_t>((int32_t)1, (int32_t)count);

	while(itemCount > 0)
	{
		int32_t stackCount = std::min<int32_t>((int32_t)100, (int32_t)subType);
		Item* newItem = Item::CreateItem(itemId, stackCount);
		if(!newItem)
		{
			reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
			lua_pushboolean(L, false);
			return 1;
		}

		if(it.stackable)
			subType -= stackCount;

		ReturnValue ret = g_game.internalAddItem(container, newItem);
		if(ret != RET_NOERROR)
		{
			delete newItem;
			lua_pushboolean(L, false);
			return 1;
		}

		--itemCount;
		if(itemCount == 0)
		{
			if(newItem->getParent())
			{
				uint32_t uid = env->addThing(newItem);
				lua_pushnumber(L, uid);
				return 1;
			}
			else
			{
				//stackable item stacked with existing object, newItem will be released
				lua_pushboolean(L, false);
				return 1;
			}
		}
	}
	lua_pushboolean(L, false);
	return 1;
}

int32_t LuaScriptInterface::luaGetDepotId(lua_State* L)
{
	//getDepotId(uid)
	uint32_t uid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();
	Container* container = env->getContainerByUID(uid);
	if(container)
	{
		Depot* depot = container->getDepot();
		if(depot)
			lua_pushnumber(L, depot->getDepotId());
		else
		{
			reportErrorFunc("Depot not found.");
			lua_pushboolean(L, false);
		}
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CONTAINER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetFluidSourceType(lua_State* L)
{
	//getFluidSourceType(type)
	uint32_t type = popNumber(L);

	const ItemType& it = Item::items[type];
	if(it.id != 0)
		lua_pushnumber(L, it.fluidSource);
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaIsInArray(lua_State* L)
{
	//isInArray(array, value)
	int32_t value = (int32_t)popNumber(L);
	if(lua_istable(L, -1) == 0)
	{
		lua_pop(L, 1);
		lua_pushboolean(L, false);
		return 1;
	}

	int32_t i = 1;
	while(true)
	{
		lua_pushnumber(L, i);
		lua_gettable(L, -2);
		if(lua_isnil(L, -1) == 1)
		{
			lua_pop(L, 2);
			lua_pushboolean(L, false);
			return 1;
		}
		else if(lua_isnumber(L, -1) == 1)
		{
			int32_t array_value = (int32_t)popNumber(L);
			if(array_value == value)
			{
				lua_pop(L, 1);
				lua_pushboolean(L, true);
				return 1;
			}
		}
		else
		{
			lua_pop(L, 2);
			lua_pushboolean(L, false);
			return 1;
		}
		++i;
	}
}

int32_t LuaScriptInterface::luaDoPlayerAddOutfit(lua_State* L)
{
	//doPlayerAddOutfit(cid, looktype, addon)
	uint32_t addon = popNumber(L);
	uint32_t looktype = popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();
	Player* player = env->getPlayerByUID(cid);
	if(player){
		player->addOutfit(looktype, addon);
		lua_pushboolean(L, true);
	}
	else{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerRemOutfit(lua_State* L)
{
	//doPlayerRemOutfit(cid, looktype, addon)
	uint32_t addon = popNumber(L);
	uint32_t looktype = popNumber(L);
	uint32_t cid = popNumber(L);
	
	ScriptEnvironment* env = getScriptEnv();
	Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		player->remOutfit(looktype, addon);
		lua_pushboolean(L, true);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaCanPlayerWearOutfit(lua_State* L)
{
	//canPlayerWearOutfit(cid, looktype, addon)
	uint32_t addon = popNumber(L);
	uint32_t looktype = popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();
	Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		lua_pushboolean(L, player->canWear(looktype, addon));
		return 1;
	}

	reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
	lua_pushboolean(L, false);
	return 1;
}

int32_t LuaScriptInterface::luaDoCreatureChangeOutfit(lua_State* L)
{
	//doCreatureChangeOutfit(cid, outfit)
	Outfit_t outfit;
	outfit.lookType = getField(L, "lookType");
	outfit.lookHead = getField(L, "lookHead");
	outfit.lookBody = getField(L, "lookBody");
	outfit.lookLegs = getField(L, "lookLegs");
	outfit.lookFeet = getField(L, "lookFeet");
	outfit.lookAddons = getField(L, "lookAddons");
	lua_pop(L, 1);

	uint32_t cid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();
	Creature* creature = env->getCreatureByUID(cid);
	if(creature)
	{
		creature->defaultOutfit = outfit;
		g_game.internalCreatureChangeOutfit(creature, outfit);
		lua_pushboolean(L, true);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoSetCreatureLight(lua_State* L)
{
	//doSetCreatureLight(cid, lightLevel, lightColor, time)
	uint32_t time = popNumber(L);
	uint8_t color = (uint8_t)popNumber(L);
	uint8_t level = (uint8_t)popNumber(L);
	uint32_t cid = popNumber(L);
	
	ScriptEnvironment* env = getScriptEnv();
	Creature* creature = env->getCreatureByUID(cid);
	if(creature)
	{
		Condition* condition = Condition::createCondition(CONDITIONID_COMBAT, CONDITION_LIGHT, time, level | (color << 8));	
		creature->addCondition(condition);
		lua_pushboolean(L, true);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerPopupFYI(lua_State* L)
{
	//doPlayerPopupFYI(cid, message)
	const std::string message = popString(L);
	uint32_t cid = (uint32_t)popNumber(L);
	ScriptEnvironment* env = getScriptEnv();
	Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		player->sendFYIBox(message);
		lua_pushboolean(L, true);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaMayNotMove(lua_State* L)
{
	//mayNotMove(cid, value)
	bool boolValue = popNumber(L) == 1;
	uint32_t cid = (uint32_t)popNumber(L);
	ScriptEnvironment* env = getScriptEnv();
	Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		player->mayNotMove = boolValue;
		if(player->mayNotMove)
			player->onWalkAborted();
		lua_pushboolean(L, true);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerAddPremiumDays(lua_State* L)
{
	//doPlayerAddPremiumDays(cid, days)
	uint32_t days = popNumber(L);
	uint32_t cid = popNumber(L);
	ScriptEnvironment* env = getScriptEnv();
	if(Player* player = env->getPlayerByUID(cid))
	{
		Account account = IOLoginData::getInstance()->loadAccount(player->getAccount());
		if(player->premiumDays < 65535)
		{
			if(player->premiumDays <= 50000 && days <= 10000)
			{
				account.premiumDays += days;
				player->premiumDays += days;
			}
			IOLoginData::getInstance()->saveAccount(account);
		}
		lua_pushboolean(L, true);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerRemovePremiumDays(lua_State* L)
{
	//doPlayerRemovePremiumDays(cid, days)
	int32_t days = popNumber(L);
	uint32_t cid = popNumber(L);
	ScriptEnvironment* env = getScriptEnv();
	if(Player* player = env->getPlayerByUID(cid))
	{
		Account account = IOLoginData::getInstance()->loadAccount(player->getAccount());
		if(player->premiumDays >= days && player->premiumDays < 65535)
		{
			account.premiumDays -= days;
			player->premiumDays -= days;
			IOLoginData::getInstance()->saveAccount(account);
		}
		lua_pushboolean(L, true);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetItemName(lua_State* L)
{
	//getItemName(itemid)
	uint32_t itemid = popNumber(L);
	const ItemType& it = Item::items[itemid];
	lua_pushstring(L, it.name.c_str());
	return 1;
}

int32_t LuaScriptInterface::luaGetItemDescriptions(lua_State* L)
{
	//getItemDescriptions(itemid)
	//returns the name, the article and the plural name of the item
	uint32_t itemid = popNumber(L);
	const ItemType& it = Item::items[itemid];

	lua_newtable(L);
	setField(L, "name", it.name.c_str());
	setField(L, "article", it.article.c_str());
	setField(L, "plural", it.getPluralName().c_str());
	setField(L, "description", it.description.c_str());
	return 1;
}

int32_t LuaScriptInterface::luaGetItemWeight(lua_State* L)
{
	//getItemWeight(itemid, count, <optional: default: 1> precise)
	int32_t parameters = lua_gettop(L);

	bool precise = true;
	if(parameters > 2)
		precise = popBoolean(L);

	int32_t count = popNumber(L);
	uint32_t itemid = popNumber(L);

	const ItemType& it = Item::items[itemid];
	double weight = it.weight * std::max<double>(1, count);
	if(precise)
	{
		std::stringstream ws;
		ws << std::fixed << std::setprecision(2) << weight;
		weight = atof(ws.str().c_str());
	}

	lua_pushnumber(L, weight);
	return 1;
}

int32_t LuaScriptInterface::luaHasProperty(lua_State* L)
{
	//hasProperty(uid, prop)
	uint32_t prop = popNumber(L);
	uint32_t uid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	Item* item = env->getItemByUID(uid);
	if(!item)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	//Check if the item is a tile, so we can get more accurate properties
	bool hasProp = item->hasProperty((ITEMPROPERTY)prop);
	if(item->getTile() && item->getTile()->ground == item)
		hasProp = item->getTile()->hasProperty((ITEMPROPERTY)prop);

	lua_pushboolean(L, hasProp);
	return 1;
}

int32_t LuaScriptInterface::luaGetCreatureMaster(lua_State* L)
{
	//getCreatureMaster(cid)
	//returns the creature's master or itself if the creature isn't a summon
	uint32_t cid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	Creature* creature = env->getCreatureByUID(cid);
	if(!creature)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	Creature* master = creature->getMaster();
	if(!master)
	{
		lua_pushnumber(L, cid);
		return 1;
	}

	uint32_t masterId = env->addThing(master);
	lua_pushnumber(L, masterId);
	return 1;
}

int32_t LuaScriptInterface::luaGetCreatureSummons(lua_State* L)
{
	//getCreatureSummons(cid)
	//returns a table with all the summons of the creature
	uint32_t cid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	Creature* creature = env->getCreatureByUID(cid);
	if(!creature)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	lua_newtable(L);
	const std::list<Creature*>& summons = creature->getSummons();
	std::list<Creature*>::const_iterator it = summons.begin();
	for(uint32_t i = 1; it != summons.end(); ++it, ++i)
	{
		uint32_t summonCid = env->addThing(*it);
		lua_pushnumber(L, i);
		lua_pushnumber(L, summonCid);
		lua_settable(L, -3);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetSpectators(lua_State* L)
{
	//getSpectators(centerPos, rangex, rangey, multifloor)

	bool multifloor = popNumber(L) == 1;
	uint32_t rangey = popNumber(L);
	uint32_t rangex = popNumber(L);

	PositionEx centerPos;
	popPosition(L, centerPos);

	SpectatorVec list;
	g_game.getSpectators(list, centerPos, false, multifloor, rangex, rangex, rangey, rangey);
	if(list.empty())
	{
		lua_pushnil(L);
		return 1;
	}

	lua_newtable(L);
	SpectatorVec::const_iterator it = list.begin();
	for(uint32_t i = 1; it != list.end(); ++it, ++i)
	{
		lua_pushnumber(L, i);
		lua_pushnumber(L, (*it)->getID());
		lua_settable(L, -3);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetItemIdByName(lua_State* L)
{
	//getItemIdByName(name)
	std::string name = popString(L);

	int32_t itemId = Item::items.getItemIdByName(name);
	if(itemId == -1)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));

		lua_pushboolean(L, false);
	}
	else
		lua_pushnumber(L, itemId);

	return 1;
}

int32_t LuaScriptInterface::luaIsSightClear(lua_State* L)
{
	//isSightClear(fromPos, toPos, floorCheck)
	PositionEx fromPos, toPos;
	bool floorCheck = (popNumber(L) == 1);
	popPosition(L, toPos);
	popPosition(L, fromPos);

	lua_pushboolean(L, g_game.isSightClear(fromPos, toPos, floorCheck));
	return 1;
}

int32_t LuaScriptInterface::luaGetCreaturePosition(lua_State* L)
{
	//getCreaturePosition(cid)
	uint32_t cid = popNumber(L);
	
	ScriptEnvironment* env = getScriptEnv();
	
	Creature* creature = env->getCreatureByUID(cid);
	if(creature){
		Position pos = creature->getPosition();
		uint32_t stackpos = 0;
		if(Tile* tile = creature->getTile())
			stackpos = tile->__getIndexOfThing(creature);
		pushPosition(L, pos, stackpos);
	}
	else{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetCreatureName(lua_State* L)
{
	//getCreatureName(cid)
	uint32_t cid = popNumber(L);
	
	ScriptEnvironment* env = getScriptEnv();
	
	Creature* creature = env->getCreatureByUID(cid);
	if(creature){
		lua_pushstring(L, creature->getName().c_str());
	}
	else{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetCreatureSpeed(lua_State* L)
{
	//getCreatureSpeed(cid)
	uint32_t cid = popNumber(L);
	
	ScriptEnvironment* env = getScriptEnv();
	
	Creature* creature = env->getCreatureByUID(cid);
	if(creature)
		lua_pushnumber(L, creature->getSpeed());
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetCreatureBaseSpeed(lua_State* L)
{
	//getCreatureBaseSpeed(cid)
	uint32_t cid = popNumber(L);
	
	ScriptEnvironment* env = getScriptEnv();
	
	Creature* creature = env->getCreatureByUID(cid);
	if(creature){
		lua_pushnumber(L, creature->getBaseSpeed());
	}
	else{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetCreatureTarget(lua_State* L)
{
	//getCreatureTarget(cid)
	uint32_t cid = popNumber(L);
	
	ScriptEnvironment* env = getScriptEnv();
	
	Creature* creature = env->getCreatureByUID(cid);
	if(creature){
		Creature* target = creature->getAttackedCreature();
		if(target){
			uint32_t targetCid = env->addThing(target);
			lua_pushnumber(L, targetCid);
		}
		else{
			lua_pushnumber(L, 0);
		}
	}
	else{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaIsItemStackable(lua_State* L)
{
	//isItemStackable(itemid)
	uint32_t itemid = popNumber(L);
	const ItemType& it = Item::items[itemid];
	if(it.stackable)
		lua_pushboolean(L, true);
	else
		lua_pushboolean(L, false);
	return 1;
}

int32_t LuaScriptInterface::luaIsItemRune(lua_State* L)
{
	//isItemRune(itemid)
	uint32_t itemid = popNumber(L);
	const ItemType& it = Item::items[itemid];
	if(it.isRune())
		lua_pushboolean(L, true);
	else
		lua_pushboolean(L, false);
	return 1;
}

int32_t LuaScriptInterface::luaIsItemDoor(lua_State* L)
{
	//isItemDoor(itemid)
	uint32_t itemid = popNumber(L);
	const ItemType& it = Item::items[itemid];
	if(it.isDoor())
		lua_pushboolean(L, true);
	else
		lua_pushboolean(L, false);
	return 1;
}

int32_t LuaScriptInterface::luaIsItemContainer(lua_State* L)
{
	//isItemContainer(itemid)
	uint32_t itemid = popNumber(L);
	const ItemType& it = Item::items[itemid];
	if(it.isContainer())
		lua_pushboolean(L, true);
	else
		lua_pushboolean(L, false);
	return 1;
}

int32_t LuaScriptInterface::luaIsItemFluidContainer(lua_State* L)
{
	//isItemFluidContainer(itemid)
	uint32_t itemid = popNumber(L);
	const ItemType& it = Item::items[itemid];
	if(it.isFluidContainer())
		lua_pushboolean(L, true);
	else
		lua_pushboolean(L, false);
	return 1;
}

int32_t LuaScriptInterface::luaIsItemMoveable(lua_State* L)
{
	uint32_t itemid = popNumber(L);
	const ItemType& it = Item::items[itemid];
	if(it.moveable)
		lua_pushboolean(L, true);
	else
		lua_pushboolean(L, false);
	return 1;
}

int32_t LuaScriptInterface::luaAddEvent(lua_State* L)
{
	//addEvent(callback, delay, ...)
	ScriptEnvironment* env = getScriptEnv();

	LuaScriptInterface* script_interface = env->getScriptInterface();
	if(!script_interface)
	{
		reportErrorFunc("No valid script interface!");
		lua_pushboolean(L, false);
		return 1;
	}

	int32_t parameters = lua_gettop(L);
	if(lua_isfunction(L, -parameters) == 0) //-parameters means the first parameter from left to right
	{
		reportErrorFunc("callback parameter should be a function.");
		lua_pushboolean(L, false);
		return 1;
	}

	LuaTimerEventDesc eventDesc;
	std::list<int32_t> params;
	for(int32_t i = 0; i < parameters-2; ++i) //-2 because addEvent needs at least two parameters
		params.push_back(luaL_ref(L, LUA_REGISTRYINDEX));

	eventDesc.parameters = params;

	uint32_t delay = std::max<uint32_t>((uint32_t)100, popNumber(L));
	eventDesc.function = luaL_ref(L, LUA_REGISTRYINDEX);

	eventDesc.scriptId = env->getScriptId();

	script_interface->m_lastEventTimerId++;

	eventDesc.eventId = Scheduler::getScheduler().addEvent(createSchedulerTask(delay, boost::bind(&LuaScriptInterface::executeTimerEvent,
		script_interface, script_interface->m_lastEventTimerId)));
	script_interface->m_timerEvents[script_interface->m_lastEventTimerId] = eventDesc;
	lua_pushnumber(L, script_interface->m_lastEventTimerId);
	return 1;
}

int32_t LuaScriptInterface::luaStopEvent(lua_State* L)
{
	//stopEvent(eventid)
	uint32_t eventId = popNumber(L);
	ScriptEnvironment* env = getScriptEnv();

	LuaScriptInterface* script_interface = env->getScriptInterface();
	if(!script_interface)
	{
		reportErrorFunc("No valid script interface!");
		lua_pushboolean(L, false);
		return 1;
	}

	LuaTimerEvents::iterator it = script_interface->m_timerEvents.find(eventId);
	if(it != script_interface->m_timerEvents.end())
	{
		LuaTimerEventDesc& timerEventDesc = it->second;
		Scheduler::getScheduler().stopEvent(timerEventDesc.eventId);
		for(std::list<int32_t>::iterator lt = timerEventDesc.parameters.begin(), end = timerEventDesc.parameters.end(); lt != end; ++lt)
			luaL_unref(script_interface->m_luaState, LUA_REGISTRYINDEX, *lt);

		timerEventDesc.parameters.clear();

		luaL_unref(script_interface->m_luaState, LUA_REGISTRYINDEX, timerEventDesc.function);
		script_interface->m_timerEvents.erase(it);
		lua_pushboolean(L, true);
	}
	else
		lua_pushboolean(L, false);

	return 1;
}

int32_t LuaScriptInterface::luaGetPromotedVocation(lua_State* L)
{
	int32_t vocationId = (int32_t)popNumber(L);
	lua_pushnumber(L, g_vocations.getPromotedVocation(vocationId));
	return 1;
}

int32_t LuaScriptInterface::luaGetCreatureCondition(lua_State* L)
{
	uint32_t condition = popNumber(L);
	uint32_t cid = popNumber(L);
	ScriptEnvironment* env = getScriptEnv();
	Creature* creature = env->getCreatureByUID(cid);
	if(creature)
	{
		if(creature->hasCondition((ConditionType_t)condition))
			lua_pushboolean(L, true);
		else
			lua_pushboolean(L, false);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerBlessing(lua_State* L)
{
	int16_t blessing = popNumber(L) - 1;
	uint32_t cid = popNumber(L);
	ScriptEnvironment* env = getScriptEnv();
	Player* player = env->getPlayerByUID(cid);
	if(player)
		lua_pushboolean(L, player->hasBlessing(blessing));
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerAddBlessing(lua_State* L)
{
	int32_t blessing = popNumber(L);
	uint32_t cid = popNumber(L);
	ScriptEnvironment* env = getScriptEnv();
	Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		blessing = 1 << (blessing - 1);
		if(!player->hasBlessing(blessing))
		{
			player->addBlessing(player->blessings + blessing);
			lua_pushboolean(L, true);
		}
		else
			lua_pushboolean(L, false);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaSetPlayerGroupId(lua_State* L)
{
	uint32_t newGroupId = popNumber(L);
	uint32_t cid = popNumber(L);
	ScriptEnvironment* env = getScriptEnv();
	Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		player->setGroupId(newGroupId);
		lua_pushboolean(L, true);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetCreatureMana(lua_State* L)
{
	//getCreatureMana(cid)
	uint32_t cid = popNumber(L);
	ScriptEnvironment* env = getScriptEnv();
	Creature* creature = env->getCreatureByUID(cid);
	if(creature)
		lua_pushnumber(L, creature->getMana());
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetCreatureMaxMana(lua_State* L)
{
	//getCreatureMaxMana(cid)
	uint32_t cid = popNumber(L);
	ScriptEnvironment* env = getScriptEnv();
	Creature* creature = env->getCreatureByUID(cid);
	if(creature)
		lua_pushnumber(L, creature->getMaxMana());
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetCreatureHealth(lua_State* L)
{
	uint32_t cid = popNumber(L);
	ScriptEnvironment* env = getScriptEnv();
	Creature* creature = env->getCreatureByUID(cid);
	if(creature)
		lua_pushnumber(L, creature->getHealth());
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetCreatureMaxHealth(lua_State* L)
{
	uint32_t cid = popNumber(L);
	ScriptEnvironment* env = getScriptEnv();
	Creature* creature = env->getCreatureByUID(cid);
	if(creature)
	{
		Player* player = creature->getPlayer();
		if(player)
			lua_pushnumber(L, player->getMaxHealth());
		else
			lua_pushnumber(L, creature->getMaxHealth());
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoSaveServer(lua_State* L)
{
	Dispatcher::getDispatcher().addTask(createTask(boost::bind(&Game::saveGameState, &g_game)));
	return 1;
}

int32_t LuaScriptInterface::luaGetOnlinePlayers(lua_State* L)
{
	//getOnlinePlayers()
	int32_t i = 0;
	lua_newtable(L);
	for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it)
	{
		lua_pushnumber(L, ++i);
		lua_pushstring(L, (*it).second->getName().c_str());
		lua_settable(L, -3);
	}
	return 1;
}

const luaL_Reg LuaScriptInterface::luaBitReg[] =
{
	//{"cast", LuaScriptInterface::luaBitCast},
	{"bnot", LuaScriptInterface::luaBitNot},
	{"band", LuaScriptInterface::luaBitAnd},
	{"bor", LuaScriptInterface::luaBitOr},
	{"bxor", LuaScriptInterface::luaBitXor},
	{"lshift", LuaScriptInterface::luaBitLeftShift},
	{"rshift", LuaScriptInterface::luaBitRightShift},
	// Unsigned
	{"ubnot", LuaScriptInterface::luaBitUNot},
	{"uband", LuaScriptInterface::luaBitUAnd},
	{"ubor", LuaScriptInterface::luaBitUOr},
	{"ubxor", LuaScriptInterface::luaBitUXor},
	{"ulshift", LuaScriptInterface::luaBitULeftShift},
	{"urshift", LuaScriptInterface::luaBitURightShift},
	//{"arshift", LuaScriptInterface::luaBitArithmeticalRightShift},
	{NULL,NULL}
};

int32_t LuaScriptInterface::luaBitNot(lua_State* L)
{
	int32_t number = (int32_t)popNumber(L);
	lua_pushnumber(L, ~number);
	return 1;
}

int32_t LuaScriptInterface::luaBitUNot(lua_State* L)
{
	uint32_t number = (uint32_t)popNumber(L);
	lua_pushnumber(L, ~number);
	return 1;
}

#define MULTIOP(type, name, op) \
	int32_t LuaScriptInterface::luaBit##name(lua_State* L) \
	{ \
		int32_t n = lua_gettop(L); \
		type w = (type)popNumber(L); \
		for(int32_t i = 2; i <= n; ++i) \
			w op popNumber(L); \
		lua_pushnumber(L, w); \
		return 1; \
	}

MULTIOP(int32_t, And, &=)
MULTIOP(int32_t, Or, |=)
MULTIOP(int32_t, Xor, ^=)
MULTIOP(uint32_t, UAnd, &=)
MULTIOP(uint32_t, UOr, |=)
MULTIOP(uint32_t, UXor, ^=)

#define SHIFTOP(type, name, op) \
	int32_t LuaScriptInterface::luaBit##name(lua_State* L) \
	{ \
		type n2 = (type)popNumber(L), n1 = (type)popNumber(L); \
		lua_pushnumber(L, (n1 op n2)); \
		return 1; \
	}

SHIFTOP(int32_t, LeftShift, <<)
SHIFTOP(int32_t, RightShift, >>)
SHIFTOP(uint32_t, ULeftShift, <<)
SHIFTOP(uint32_t, URightShift, >>)

const luaL_Reg LuaScriptInterface::luaDatabaseTable[] =
{
	{"query", LuaScriptInterface::luaDatabaseExecute},
	{"storeQuery", LuaScriptInterface::luaDatabaseStoreQuery},
	{"escapeString", LuaScriptInterface::luaDatabaseEscapeString},
	{"escapeBlob", LuaScriptInterface::luaDatabaseEscapeBlob},
	{"lastInsertId", LuaScriptInterface::luaDatabaseLastInsertId},
	{"stringComparer", LuaScriptInterface::luaDatabaseStringComparer},
	{"updateLimiter", LuaScriptInterface::luaDatabaseUpdateLimiter},
	{"connected", LuaScriptInterface::luaDatabaseConnected},
	{"tableExists", LuaScriptInterface::luaDatabaseTableExists},
	{NULL,NULL}
};

int32_t LuaScriptInterface::luaDatabaseExecute(lua_State* L)
{
	DBQuery query;
	lua_pushboolean(L, Database::getInstance()->executeQuery(popString(L)));
	return 1;
}

int32_t LuaScriptInterface::luaDatabaseStoreQuery(lua_State* L)
{
	ScriptEnvironment* env = getScriptEnv();

	DBQuery query;
	if(DBResult* res = Database::getInstance()->storeQuery(popString(L)))
		lua_pushnumber(L, env->addResult(res));
	else
		lua_pushboolean(L, false);

	return 1;
}

int32_t LuaScriptInterface::luaDatabaseEscapeString(lua_State* L)
{
	DBQuery query;
	lua_pushstring(L, Database::getInstance()->escapeString(popString(L)).c_str());
	return 1;
}

int32_t LuaScriptInterface::luaDatabaseEscapeBlob(lua_State* L)
{
	uint32_t length = popNumber(L);
	DBQuery query;

	lua_pushstring(L, Database::getInstance()->escapeBlob(popString(L).c_str(), length).c_str());
	return 1;
}

int32_t LuaScriptInterface::luaDatabaseLastInsertId(lua_State* L)
{
	DBQuery query;
	lua_pushnumber(L, Database::getInstance()->getLastInsertId());
	return 1;
}

int32_t LuaScriptInterface::luaDatabaseStringComparer(lua_State* L)
{
	lua_pushstring(L, Database::getInstance()->getStringComparer().c_str());
	return 1;
}

int32_t LuaScriptInterface::luaDatabaseUpdateLimiter(lua_State* L)
{
	lua_pushstring(L, Database::getInstance()->getUpdateLimiter().c_str());
	return 1;
}

int32_t LuaScriptInterface::luaDatabaseConnected(lua_State* L)
{
	lua_pushboolean(L, Database::getInstance()->isConnected());
	return 1;
}

const luaL_Reg LuaScriptInterface::luaResultTable[] =
{
	{"getDataInt", LuaScriptInterface::luaResultGetDataInt},
	{"getDataLong", LuaScriptInterface::luaResultGetDataLong},
	{"getDataString", LuaScriptInterface::luaResultGetDataString},
	{"getDataStream", LuaScriptInterface::luaResultGetDataStream},
	{"getAllData", LuaScriptInterface::luaResultGetAllData},
	{"next", LuaScriptInterface::luaResultNext},
	{"free", LuaScriptInterface::luaResultFree},
	{NULL, NULL}
};

int32_t LuaScriptInterface::luaResultGetDataInt(lua_State* L)
{
	const std::string& s = popString(L);
	ScriptEnvironment* env = getScriptEnv();

	DBResult* res = env->getResultByID(popNumber(L));
	if(!res)
	{
		lua_pushboolean(L, false);
		return 1;
	}

	lua_pushnumber(L, res->getDataInt(s));
	return 1;
}

int32_t LuaScriptInterface::luaResultGetDataLong(lua_State* L)
{
	const std::string& s = popString(L);
	ScriptEnvironment* env = getScriptEnv();

	DBResult* res = env->getResultByID(popNumber(L));
	if(!res)
	{
		lua_pushboolean(L, false);
		return 1;
	}

	lua_pushnumber(L, res->getDataLong(s));
	return 1;
}

int32_t LuaScriptInterface::luaResultGetDataString(lua_State* L)
{
	const std::string& s = popString(L);
	ScriptEnvironment* env = getScriptEnv();

	DBResult* res = env->getResultByID(popNumber(L));
	if(!res)
	{
		lua_pushboolean(L, false);
		return 1;
	}

	lua_pushstring(L, res->getDataString(s).c_str());
	return 1;
}

int32_t LuaScriptInterface::luaResultGetDataStream(lua_State* L)
{
	const std::string& s = popString(L);
	ScriptEnvironment* env = getScriptEnv();

	DBResult* res = env->getResultByID(popNumber(L));
	if(!res)
	{
		lua_pushboolean(L, false);
		return 1;
	}

	unsigned long length = 0;
	lua_pushstring(L, res->getDataStream(s, length));
	lua_pushnumber(L, length);
	return 2;
}

int32_t LuaScriptInterface::luaResultGetAllData(lua_State* L)
{
	ScriptEnvironment* env = getScriptEnv();

	DBResult* res = env->getResultByID(popNumber(L));
	if(!res)
	{
		lua_pushboolean(L, false);
		return 1;
	}

	lua_newtable(L);
	listNames_t listNames = res->getListNames();
	for(listNames_t::iterator it = listNames.begin(); it != listNames.end(); ++it)
		setField(L, it->first.c_str(), res->getDataString(it->first));

	return 1;
}

int32_t LuaScriptInterface::luaResultNext(lua_State* L)
{
	ScriptEnvironment* env = getScriptEnv();

	DBResult* res = env->getResultByID(popNumber(L));
	if(!res)
	{
		lua_pushboolean(L, false);
		return 1;
	}

	lua_pushboolean(L, res->next());
	return 1;
}

int32_t LuaScriptInterface::luaResultFree(lua_State* L)
{
	ScriptEnvironment* env = getScriptEnv();
	lua_pushboolean(L, env->removeResult(popNumber(L)));
	return 1;
}

int32_t LuaScriptInterface::luaDatabaseTableExists(lua_State* L)
{
	lua_pushboolean(L, DatabaseManager::getInstance()->tableExists(popString(L)));
	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerParty(lua_State* L)
{
	//getPlayerParty(cid)
	uint32_t cid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();
	if(Player* player = env->getPlayerByUID(cid))
	{
		if(Party* party = player->getParty())
			lua_pushnumber(L, env->addThing(party->getLeader()));
		else
			lua_pushnil(L);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}

	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerJoinParty(lua_State* L)
{
	//doPlayerJoinParty(cid, lid)
	ScriptEnvironment* env = getScriptEnv();

	uint32_t cid = popNumber(L);
	Player* leader = env->getPlayerByUID(cid);
	if(!leader)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}

	Player* player = env->getPlayerByUID(cid);
	if(!player)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}

	g_game.playerJoinParty(player->getID(), leader->getID());
	lua_pushboolean(L, true);
	return 1;
}

int32_t LuaScriptInterface::luaGetPartyMembers(lua_State* L)
{
	//getPartyMembers(leaderId)
	uint32_t cid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();
	Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		Party* party = player->getParty();
		if(party)
		{
			PlayerVector list = party->getMembers();
			list.push_back(party->getLeader());

			PlayerVector::const_iterator it = list.begin();
			lua_newtable(L);
			for(uint32_t i = 1; it != list.end(); ++it, ++i)
			{
				lua_pushnumber(L, i);
				lua_pushnumber(L, (*it)->getID());
				lua_settable(L, -3);
			}
			return 1;
		}
	}
	else
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));

	lua_pushboolean(L, false);
	return 1;
}

int32_t LuaScriptInterface::luaDoCleanMap(lua_State* L)
{
 	//doCleanMap()
	uint32_t count = g_game.getMap()->onRemoveTileItem();
	lua_pushnumber(L, count);
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerSetBankBalance(lua_State* L)
{
	//doPlayerSetBalance(cid, balance)
	uint64_t balance = popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();
	Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		player->setBankBalance(balance);
		lua_pushboolean(L, true);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerSetGuildId(lua_State* L)
{
	//doPlayerSetGuildId(cid, id)
	uint32_t id = popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();
	Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		if(player->guildId == 0)
		{
			if(IOGuild::getInstance()->guildExists(id))
				IOGuild::getInstance()->joinGuild(player, id);
		}
		lua_pushboolean(L, true);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetCreatureLookDirection(lua_State* L)
{
	//getCreatureLookDirection(cid)
	ScriptEnvironment* env = getScriptEnv();

	if(Creature* creature = env->getCreatureByUID(popNumber(L)))
		lua_pushnumber(L, creature->getDirection());
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaIsValidItemId(lua_State* L)
{
	//isValidItemId(itemid)
	uint32_t itemId = popNumber(L);
	const ItemType& it = Item::items[itemId];
	if(it.id == 0)
		lua_pushboolean(L, false);
	else
		lua_pushboolean(L, true);
	return 1;
}

int32_t LuaScriptInterface::luaGetGameState(lua_State* L)
{
	//getGameState()
	lua_pushnumber(L, g_game.getGameState());
	return 1;
}

int32_t LuaScriptInterface::luaDoSetGameState(lua_State* L)
{
	//doSetGameState(id)
	uint32_t id = popNumber(L);
	if(id >= GAME_STATE_FIRST && id <= GAME_STATE_LAST)
	{
		Dispatcher::getDispatcher().addTask(createTask(boost::bind(&Game::setGameState, &g_game, (GameState_t)id)));
		lua_pushboolean(L, true);
	}
	else
		lua_pushboolean(L, false);

	return 1;
}

int32_t LuaScriptInterface::luaGetPlayersOnline(lua_State* L)
{
	//getPlayersOnline()
	ScriptEnvironment* env = getScriptEnv();
	AutoList<Player>::listiterator it = Player::listPlayer.list.begin();

	lua_newtable(L);
	for(int32_t i = 1; it != Player::listPlayer.list.end(); ++it, ++i)
	{
		lua_pushnumber(L, i);
		lua_pushnumber(L, env->addThing(it->second));
		lua_settable(L, -3);
		//pushTable(L); TODO pushTable
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetClosestFreeTile(lua_State* L)
{
	//getClosestFreeTile(cid, targetPos[, extended = false[, ignoreHouse = true]])
	uint32_t params = lua_gettop(L);
	bool ignoreHouse = true, extended = false;
	if(params > 3)
		ignoreHouse = popNumber(L);

	if(params > 2)
		extended = popNumber(L);

	PositionEx pos;
	popPosition(L, pos);

	ScriptEnvironment* env = getScriptEnv();
	if(Creature* creature = env->getCreatureByUID(popNumber(L)))
	{
		Position newPos = g_game.getClosestFreeTile(creature, pos, extended, ignoreHouse);
		if(newPos.x != 0)
			pushPosition(L, newPos, 0);
		else
			lua_pushboolean(L, false);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetTownName(lua_State* L)
{
	//getTownName(townId)
	uint32_t townId = popNumber(L);
	if(Town* town = Towns::getInstance().getTown(townId))
		lua_pushstring(L, town->getName().c_str());
	else
		lua_pushboolean(L, false);

	return 1;
}

int32_t LuaScriptInterface::luaGetTownTemplePosition(lua_State* L)
{
	//getTownTemplePosition(townId)
	bool displayError = true;
	if(lua_gettop(L) >= 2)
		displayError = popNumber(L);

	uint32_t townId = popNumber(L);
	if(Town* town = Towns::getInstance().getTown(townId))
		pushPosition(L, town->getTemplePosition(), 255);
	else
		lua_pushboolean(L, false);

	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerByNameWildcard(lua_State* L)
{
	//getPlayerByNameWildcard(name~[, ret = false])
	Player* player = NULL;
	bool pushRet = false;
	if(lua_gettop(L) > 1)
		pushRet = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();
	ReturnValue ret = g_game.getPlayerByNameWildcard(popString(L), player);
	if(ret == RET_NOERROR)
		lua_pushnumber(L, env->addThing(player));
	else if(pushRet)
		lua_pushnumber(L, ret);
	else
		lua_pushnil(L);

	return 1;
}

int32_t LuaScriptInterface::luaGetTownId(lua_State* L)
{
	//getTownId(townName)
	std::string townName = popString(L);
	if(Town* town = Towns::getInstance().getTown(townName))
		lua_pushnumber(L, town->getTownID());
	else
		lua_pushboolean(L, false);

	return 1;
}

int32_t LuaScriptInterface::luaDoSummonMonster(lua_State* L)
{
	//doSummonMonster(cid, name)
	std::string name = popString(L);

	ScriptEnvironment* env = getScriptEnv();
	Creature* creature = env->getCreatureByUID(popNumber(L));
	if(!creature)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	lua_pushnumber(L, g_game.placeSummon(creature, name));
	return 1;
}

int32_t LuaScriptInterface::luaDoCreateMonster(lua_State* L)
{
	//doCreateMonster(name, pos[, displayError = true])
	bool displayError = true;
	if(lua_gettop(L) > 2)
		displayError = popNumber(L);

	PositionEx pos;
	popPosition(L, pos);

	std::string name = popString(L);
	Monster* monster = Monster::createMonster(name.c_str());
	if(!monster)
	{
		if(displayError)
			reportErrorFunc("Monster with name '" + name + "' not found");

		lua_pushboolean(L, false);
		return 1;
	}

	if(!g_game.placeCreature(monster, pos))
	{
		delete monster;
		if(displayError)
			reportErrorFunc("Cannot create monster: " + name);

		lua_pushboolean(L, true);
		return 1;
	}

	ScriptEnvironment* env = getScriptEnv();
	lua_pushnumber(L, env->addThing((Thing*)monster));
	return 1;
}

int32_t LuaScriptInterface::luaDoCreateNpc(lua_State* L)
{
	//doCreateNpc(name, pos[, displayError = true])
	bool displayError = true;
	if(lua_gettop(L) > 2)
		displayError = popNumber(L);

	PositionEx pos;
	popPosition(L, pos);

	std::string name = popString(L);
	Npc* npc = Npc::createNpc(name.c_str());
	if(!npc)
	{
		if(displayError)
			reportErrorFunc("Npc with name '" + name + "' not found");

		lua_pushboolean(L, false);
		return 1;
	}

	if(!g_game.placeCreature(npc, pos))
	{
		delete npc;
		if(displayError)
			reportErrorFunc("Cannot create npc: " + name);

		lua_pushboolean(L, true); //for scripting compatibility
		return 1;
	}

	ScriptEnvironment* env = getScriptEnv();
	lua_pushnumber(L, env->addThing((Thing*)npc));
	return 1;
}

int32_t LuaScriptInterface::luaGetExperienceStage(lua_State* L)
{
	//getExperienceStage(level)
	lua_pushnumber(L, g_game.getExperienceStage(popNumber(L)));
	return 1;
}

int32_t LuaScriptInterface::luaGetCreatureByName(lua_State* L)
{
	//getCreatureByName(name)
	ScriptEnvironment* env = getScriptEnv();
	if(Creature* creature = g_game.getCreatureByName(popString(L)))
		lua_pushnumber(L, env->addThing(creature));
	else
		lua_pushnil(L);

	return 1;
}

