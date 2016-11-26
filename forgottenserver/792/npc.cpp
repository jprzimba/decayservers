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

#include "definitions.h"
#include "npc.h"
#include "game.h"
#include "tools.h"
#include "configmanager.h"
#include "position.h"
#include "spells.h"

#include <algorithm>
#include <functional>
#include <string>
#include <sstream>
#include <fstream>

#include "luascript.h"

extern ConfigManager g_config;
extern Game g_game;
extern Spells* g_spells;

AutoList<Npc> Npc::listNpc;

NpcScriptInterface* Npc::m_scriptInterface = nullptr;

void Npcs::reload()
{
	delete Npc::m_scriptInterface;
	Npc::m_scriptInterface = nullptr;

	for(AutoList<Npc>::listiterator it = Npc::listNpc.list.begin(); it != Npc::listNpc.list.end(); ++it)
		it->second->reload();
}

Npc* Npc::createNpc(const std::string& name)
{
	Npc* npc = new Npc(name);
	if(!npc)
		return nullptr;

	if(!npc->load())
	{
		delete npc;
		return nullptr;
	}
	return npc;
}

Npc::Npc(const std::string& _name) :
	Creature()
{
	m_filename = "data/npc/" + _name + ".xml";
	loaded = false;

	m_npcEventHandler = nullptr;
	reset();
}

Npc::~Npc()
{
	Scheduler::getScheduler().stopEvent(npcTalkEvent);
	reset();
}

bool Npc::load()
{
	if(isLoaded())
		return true;

	reset();

	if(!m_scriptInterface)
	{
		m_scriptInterface = new NpcScriptInterface();
		m_scriptInterface->loadNpcLib("data/npc/lib/npc.lua");
	}

	loaded = loadFromXml(m_filename);
	return isLoaded();
}

void Npc::reset()
{
	loaded = false;
	walkTicks = 1500;
	floorChange = false;
	attackable = false;
	focusCreature = 0;
	isIdle = true;
	talkRadius = 2;
	idleTime = 30;

	delete m_npcEventHandler;
	m_npcEventHandler = nullptr;

	for(StateList::iterator it = stateList.begin(); it != stateList.end(); ++it)
		delete *it;

	stateList.clear();
	queueList.clear();
	m_parameters.clear();
	itemListMap.clear();
}

void Npc::reload()
{
	reset();
	load();

	//Simulate that the creature is placed on the map again.
	if(m_npcEventHandler)
		m_npcEventHandler->onCreatureAppear(this);

	if(walkTicks > 0)
		addEventWalk();
}

bool Npc::loadFromXml(const std::string& filename)
{
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(filename.c_str());
	if(!result) {
		std::cout << "[Error - Npc::loadFromXml] Failed to load " << filename << ": " << result.description() << std::endl;
		return false;
	}

	pugi::xml_node npcNode = doc.child("npc");
	if(!npcNode) {
		std::cout << "[Error - Npc::loadFromXml] Missing npc tag in " << filename << std::endl;
		return false;
	}

	name = npcNode.attribute("name").as_string();
	attackable = npcNode.attribute("attackable").as_bool();
	floorChange = npcNode.attribute("floorchange").as_bool();

	pugi::xml_attribute attr;
	if((attr = npcNode.attribute("speed"))) {
		baseSpeed = pugi::cast<uint32_t>(attr.value());
	} else {
		baseSpeed = 100;
	}

	if((attr = npcNode.attribute("walkinterval"))) {
		walkTicks = pugi::cast<uint32_t>(attr.value());
	}

	if((attr = npcNode.attribute("walkradius"))) {
		masterRadius = pugi::cast<int32_t>(attr.value());
	}

	pugi::xml_node healthNode = npcNode.child("health");
	if(healthNode) {
		if((attr = healthNode.attribute("now"))) {
			health = pugi::cast<int32_t>(attr.value());
		} else {
			health = 100;
		}

		if((attr = healthNode.attribute("max"))) {
			healthMax = pugi::cast<int32_t>(attr.value());
		} else {
			healthMax = 100;
		}
	}

	pugi::xml_node lookNode = npcNode.child("look");
	if(lookNode) {
		pugi::xml_attribute lookTypeAttribute = lookNode.attribute("type");
		if(lookTypeAttribute) {
			defaultOutfit.lookType = pugi::cast<uint16_t>(lookTypeAttribute.value());
			defaultOutfit.lookHead = pugi::cast<uint16_t>(lookNode.attribute("head").value());
			defaultOutfit.lookBody = pugi::cast<uint16_t>(lookNode.attribute("body").value());
			defaultOutfit.lookLegs = pugi::cast<uint16_t>(lookNode.attribute("legs").value());
			defaultOutfit.lookFeet = pugi::cast<uint16_t>(lookNode.attribute("feet").value());
			defaultOutfit.lookAddons = pugi::cast<uint16_t>(lookNode.attribute("addons").value());
		} else if((attr = lookNode.attribute("typeex"))) {
			defaultOutfit.lookTypeEx = pugi::cast<uint16_t>(attr.value());
		}

		currentOutfit = defaultOutfit;
	}

	for(pugi::xml_node parameterNode = npcNode.child("parameters").first_child(); parameterNode; parameterNode = parameterNode.next_sibling()) {
		m_parameters[parameterNode.attribute("key").as_string()] = parameterNode.attribute("value").as_string();
	}

	pugi::xml_node interactionNode = npcNode.child("interaction");
	if(healthNode) {
		if((attr = interactionNode.attribute("talkradius"))) {
			talkRadius = pugi::cast<int32_t>(attr.value());
		}

		if((attr = interactionNode.attribute("idletime"))) {
			idleTime = pugi::cast<int32_t>(attr.value());
		}
	}
	
	pugi::xml_attribute scriptFile = npcNode.attribute("script");
	if(scriptFile) {
		m_npcEventHandler = new NpcScript(scriptFile.as_string(), this);
		if(!m_npcEventHandler->isLoaded()) {
			return false;
		}
	}
	return true;
}

NpcState* Npc::getState(const Player* player, bool makeNew /*= true*/)
{
	for(StateList::iterator it = stateList.begin(); it != stateList.end(); ++it)
	{
		if((*it)->respondToCreature == player->getID())
			return *it;
	}

	if(!makeNew)
		return nullptr;

	NpcState* state = new NpcState;
	state->prevInteraction = 0;
	state->price = 0;
	state->sellPrice = 0;
	state->buyPrice = 0;
	state->amount = 1;
	state->itemId = 0;
	state->subType = -1;
	state->spellName = "";
	state->listName = "";
	state->listPluralName = "";
	state->level = -1;
	state->topic = -1;
	state->isIdle = true;
	state->isQueued = false;
	state->respondToText = "";
	state->respondToCreature = 0;
	state->prevRespondToText = "";
	stateList.push_back(state);
	return state;
}

bool Npc::canSee(const Position& pos) const
{
	if(pos.z != getPosition().z)
		return false;

	return Creature::canSee(getPosition(), pos, Map::maxClientViewportX, Map::maxClientViewportY);
}

std::string Npc::getDescription(int32_t lookDistance) const
{
	std::stringstream s;
	s << name << ".";
	return s.str();
}

void Npc::onAddTileItem(const Tile* tile, const Position& pos, const Item* item)
{
	Creature::onAddTileItem(tile, pos, item);
}

void Npc::onUpdateTileItem(const Tile* tile, const Position& pos, uint32_t stackpos,
	const Item* oldItem, const ItemType& oldType, const Item* newItem, const ItemType& newType)
{
	Creature::onUpdateTileItem(tile, pos, stackpos, oldItem, oldType, newItem, newType);
}

void Npc::onRemoveTileItem(const Tile* tile, const Position& pos, uint32_t stackpos,
	const ItemType& iType, const Item* item)
{
	Creature::onRemoveTileItem(tile, pos, stackpos, iType, item);
}

void Npc::onUpdateTile(const Tile* tile, const Position& pos)
{
	Creature::onUpdateTile(tile, pos);
}

void Npc::onCreatureAppear(const Creature* creature, bool isLogin)
{
	Creature::onCreatureAppear(creature, isLogin);

	if(creature == this && walkTicks > 0)
		addEventWalk();

	if(creature == this)
	{
		if(m_npcEventHandler)
			m_npcEventHandler->onCreatureAppear(creature);
	}
	//only players for script events
	else if(Player* player = const_cast<Player*>(creature->getPlayer()))
	{
		if(m_npcEventHandler)
			m_npcEventHandler->onCreatureAppear(creature);

		NpcState* npcState = getState(player);
		if(npcState)
		{
			if(canSee(player->getPosition()))
			{
				npcState->respondToCreature = player->getID();
				onPlayerEnter(player, npcState);
			}
		}
	}
}

void Npc::onCreatureDisappear(const Creature* creature, uint32_t stackpos, bool isLogout)
{
	Creature::onCreatureDisappear(creature, stackpos, isLogout);

	if(creature == this)
	{
		/*
		Can't use this yet because Jiddo's scriptsystem isn't able to handle it.
		if(m_npcEventHandler){
			m_npcEventHandler->onCreatureDisappear(creature);
		}
		*/
	}
	else if(Player* player = const_cast<Player*>(creature->getPlayer()))
	{
		if(m_npcEventHandler)
			m_npcEventHandler->onCreatureDisappear(creature);

		NpcState* npcState = getState(player);
		if(npcState)
		{
			npcState->respondToCreature = player->getID();
			onPlayerLeave(player, npcState);
		}
	}
}

void Npc::onCreatureMove(const Creature* creature, const Tile* newTile, const Position& newPos,
		const Tile* oldTile, const Position& oldPos, uint32_t oldStackPos, bool teleport)
{
	Creature::onCreatureMove(creature, newTile, newPos, oldTile, oldPos, oldStackPos, teleport);

	if(creature == this)
	{
		if(m_npcEventHandler)
			m_npcEventHandler->onCreatureMove(creature, oldPos, newPos);
	}
	else if(Player* player = const_cast<Player*>(creature->getPlayer()))
	{
		if(m_npcEventHandler)
			m_npcEventHandler->onCreatureMove(creature, oldPos, newPos);
	}
}

void Npc::onCreatureTurn(const Creature* creature, uint32_t stackpos)
{
	Creature::onCreatureTurn(creature, stackpos);
}

void Npc::onCreatureSay(const Creature* creature, SpeakClasses type, const std::string& text)
{
	if(creature->getID() == this->getID())
		return;

	//only players for script events
	if(const Player* player = creature->getPlayer())
	{
		if(m_npcEventHandler)
			m_npcEventHandler->onCreatureSay(player, type, text);

		if(type == SPEAK_SAY)
		{
			const Position& myPos = getPosition();
			const Position& pos = creature->getPosition();
			if(canSee(myPos))
			{
				if((pos.x >= myPos.x - talkRadius) && (pos.x <= myPos.x + talkRadius) &&
					(pos.y >= myPos.y - talkRadius) && (pos.y <= myPos.y + talkRadius))
				{
					NpcState* npcState = getState(player);
					npcState->respondToText = text;
					npcState->respondToCreature = player->getID();
				}
			}
		}
	}
}

void Npc::onCreatureChangeOutfit(const Creature* creature, const Outfit_t& outfit)
{
	#ifdef __DEBUG_NPC__
	std::cout << "Npc::onCreatureChangeOutfit" << std::endl;
	#endif
}

void Npc::onPlayerEnter(Player* player, NpcState* state)
{

}

void Npc::onPlayerLeave(Player* player, NpcState* state)
{

}

void Npc::onThink(uint32_t interval)
{
	Creature::onThink(interval);
	if(m_npcEventHandler)
		m_npcEventHandler->onThink();
}

void Npc::doSay(std::string msg)
{
	if(npcTalkEvent)
		Scheduler::getScheduler().stopEvent(npcTalkEvent);
	npcTalkEvent = Scheduler::getScheduler().addEvent(createSchedulerTask(500, boost::bind(&Game::internalCreatureSay, &g_game, this, SPEAK_SAY, msg)));
}

void Npc::doMove(Direction dir)
{
	g_game.internalMoveCreature(this, dir);
}

void Npc::doTurn(Direction dir)
{
	g_game.internalCreatureTurn(this, dir);
}

bool Npc::getNextStep(Direction& dir)
{
	if(Creature::getNextStep(dir))
		return true;

	if(walkTicks <= 0)
		return false;

	if(!isIdle || focusCreature != 0)
		return false;

	if(getTimeSinceLastMove() < walkTicks)
		return false;

	return getRandomStep(dir);
}

bool Npc::canWalkTo(const Position& fromPos, Direction dir)
{
	Position toPos = fromPos;
	toPos = getNextPosition(dir, toPos);

	bool result = Spawns::getInstance()->isInZone(masterPos, masterRadius, toPos);
	if(!result)
		return false;

	Tile* tile = g_game.getTile(toPos.x, toPos.y, toPos.z);
	if(!tile || tile->__queryAdd(0, this, 1, 0) != RET_NOERROR)
		return false;

	if(!floorChange && (tile->floorChange() || tile->getTeleportItem()))
		return false;

	return true;
}

bool Npc::getRandomStep(Direction& dir)
{
	std::vector<Direction> dirList;
	const Position& creaturePos = getPosition();

	if(canWalkTo(creaturePos, NORTH))
		dirList.push_back(NORTH);

	if(canWalkTo(creaturePos, SOUTH))
		dirList.push_back(SOUTH);

	if(canWalkTo(creaturePos, EAST))
		dirList.push_back(EAST);

	if(canWalkTo(creaturePos, WEST))
		dirList.push_back(WEST);

	if(!dirList.empty())
	{
		std::random_shuffle(dirList.begin(), dirList.end());
		dir = dirList[random_range(0, dirList.size() - 1)];
		return true;
	}
	return false;
}

void Npc::doMoveTo(Position target)
{
	std::list<Direction> listDir;
	if(!g_game.getPathToEx(this, target, listDir, 1, 1, true, true))
		return;

	startAutoWalk(listDir);
}

void Npc::setCreatureFocus(Creature* creature)
{
	if(creature)
	{
		focusCreature = creature->getID();
		const Position& creaturePos = creature->getPosition();
		const Position& myPos = getPosition();
		int32_t dx = myPos.x - creaturePos.x;
		int32_t dy = myPos.y - creaturePos.y;

		Direction dir = SOUTH;
		float tan = 0;

		if(dx != 0)
			tan = dy/dx;
		else
			tan = 10;

		if(std::abs(tan) < 1)
		{
			if(dx > 0)
				dir = WEST;
			else
				dir = EAST;
		}
		else
		{
			if(dy > 0)
				dir = NORTH;
			else
				dir = SOUTH;
		}
		g_game.internalCreatureTurn(this, dir);
	}
	else
		focusCreature = 0;
}

NpcScriptInterface* Npc::getScriptInterface()
{
	return m_scriptInterface;
}

NpcScriptInterface::NpcScriptInterface() :
	LuaScriptInterface("Npc interface")
{
	m_libLoaded = false;
	initState();
}


NpcScriptInterface::~NpcScriptInterface()
{
	//
}

bool NpcScriptInterface::initState()
{
	return LuaScriptInterface::initState();
}

bool NpcScriptInterface::closeState()
{
	m_libLoaded = false;
	return LuaScriptInterface::closeState();
}

bool NpcScriptInterface::loadNpcLib(std::string file)
{
	if(m_libLoaded)
		return true;

	if(loadFile(file) == -1)
	{
		std::cout << "Warning: [NpcScriptInterface::loadNpcLib] Can not load " << file << std::endl;
		return false;
	}

	m_libLoaded = true;
	return true;
}

void NpcScriptInterface::registerFunctions()
{
	LuaScriptInterface::registerFunctions();

	//npc exclusive functions
	lua_register(m_luaState, "selfSay", NpcScriptInterface::luaActionSay);
	lua_register(m_luaState, "selfMove", NpcScriptInterface::luaActionMove);
	lua_register(m_luaState, "selfMoveTo", NpcScriptInterface::luaActionMoveTo);
	lua_register(m_luaState, "selfTurn", NpcScriptInterface::luaActionTurn);
	lua_register(m_luaState, "selfFollow", NpcScriptInterface::luaActionFollow);
	lua_register(m_luaState, "selfGetPosition", NpcScriptInterface::luaSelfGetPos);
	lua_register(m_luaState, "creatureGetName", NpcScriptInterface::luaCreatureGetName);
	lua_register(m_luaState, "creatureGetPosition", NpcScriptInterface::luaCreatureGetPos);
	lua_register(m_luaState, "getDistanceTo", NpcScriptInterface::luagetDistanceTo);
	lua_register(m_luaState, "doNpcSetCreatureFocus", NpcScriptInterface::luaSetNpcFocus);
	lua_register(m_luaState, "getNpcCid", NpcScriptInterface::luaGetNpcCid);
	lua_register(m_luaState, "getNpcPos", NpcScriptInterface::luaGetNpcPos);
	lua_register(m_luaState, "getNpcState", NpcScriptInterface::luaGetNpcState);
	lua_register(m_luaState, "getNpcName", NpcScriptInterface::luaGetNpcName);
	lua_register(m_luaState, "getNpcParameter", NpcScriptInterface::luaGetNpcParameter);
}

int32_t NpcScriptInterface::luaCreatureGetName(lua_State* L)
{
	//creatureGetName(cid)
	popNumber(L);
	reportErrorFunc("Deprecated function, use getCreatureName.");
	lua_pushstring(L, "");
	return 1;
}

int32_t NpcScriptInterface::luaCreatureGetPos(lua_State* L)
{
	//creatureGetPosition(cid)
	popNumber(L);
	reportErrorFunc("Deprecated function, use getCreaturePosition.");
	lua_pushnil(L);
	lua_pushnil(L);
	lua_pushnil(L);
	return 3;
}

int32_t NpcScriptInterface::luaSelfGetPos(lua_State* L)
{
	//selfGetPosition()
	ScriptEnvironment* env = getScriptEnv();
	Npc* npc = env->getNpc();
	if(npc)
	{
		Position pos = npc->getPosition();
		lua_pushnumber(L, pos.x);
		lua_pushnumber(L, pos.y);
		lua_pushnumber(L, pos.z);
	}
	else
	{
		lua_pushnil(L);
		lua_pushnil(L);
		lua_pushnil(L);
	}
	return 3;
}

int32_t NpcScriptInterface::luaActionSay(lua_State* L)
{
    //selfSay(words)
	int32_t parameters = lua_gettop(L);

	std::string msg(popString(L));
	
	ScriptEnvironment* env = getScriptEnv();

	Npc* npc = env->getNpc();
	if(npc)
		npc->doSay(msg);

	return 0;
}

int32_t NpcScriptInterface::luaActionMove(lua_State* L)
{
	//selfMove(direction)
	Direction dir = (Direction)popNumber(L);
	ScriptEnvironment* env = getScriptEnv();

	Npc* npc = env->getNpc();
	if(npc)
		npc->doMove(dir);

	return 0;
}

int32_t NpcScriptInterface::luaActionMoveTo(lua_State* L)
{
	//selfMoveTo(x,y,z)
	Position target;
	target.z = (int)popNumber(L);
	target.y = (int)popNumber(L);
	target.x = (int)popNumber(L);

	ScriptEnvironment* env = getScriptEnv();
	Npc* npc = env->getNpc();
	if(npc)
		npc->doMoveTo(target);

	return 0;
}

int32_t NpcScriptInterface::luaActionTurn(lua_State* L)
{
	//selfTurn(direction)
	Direction dir = (Direction)popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	Npc* npc = env->getNpc();
	if(npc)
		npc->doTurn(dir);

	return 0;
}

int32_t NpcScriptInterface::luaActionFollow(lua_State* L)
{
	//selfFollow(cid)
	uint32_t cid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	Player* player = env->getPlayerByUID(cid);
	if(cid != 0 && !player)
	{
		lua_pushboolean(L, false);
		return 1;
	}

	Npc* npc = env->getNpc();
	if(!npc)
	{
		lua_pushboolean(L, false);
		return 1;
	}

	bool result = npc->setFollowCreature(player, true);
	lua_pushboolean(L, result);
	return 1;
}

int32_t NpcScriptInterface::luagetDistanceTo(lua_State* L)
{
	//getDistanceTo(uid)
	uint32_t uid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	Npc* npc = env->getNpc();
	Thing* thing = env->getThingByUID(uid);
	if(thing && npc)
	{
		Position thing_pos = thing->getPosition();
		Position npc_pos = npc->getPosition();
		if(npc_pos.z != thing_pos.z)
			lua_pushnumber(L, -1);
		else
		{
			int32_t dist = std::max<int32_t>(std::abs(npc_pos.x - thing_pos.x), std::abs(npc_pos.y - thing_pos.y));
			lua_pushnumber(L, dist);
		}
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_THING_NOT_FOUND));
		lua_pushnil(L);
	}

	return 1;
}

int32_t NpcScriptInterface::luaSetNpcFocus(lua_State* L)
{
	//doNpcSetCreatureFocus(cid)
	uint32_t cid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	Npc* npc = env->getNpc();
	if(npc)
	{
		Creature* creature = env->getCreatureByUID(cid);
		npc->setCreatureFocus(creature);
	}
	return 0;
}

int32_t NpcScriptInterface::luaGetNpcPos(lua_State* L)
{
	//getNpcPos()
	ScriptEnvironment* env = getScriptEnv();

	Position pos(0, 0, 0);
	uint32_t stackpos = 0;

	Npc* npc = env->getNpc();
	if(npc)
	{
		pos = npc->getPosition();
		stackpos = npc->getParent()->__getIndexOfThing(npc);
	}

	pushPosition(L, pos, stackpos);
	return 1;
}

int32_t NpcScriptInterface::luaGetNpcState(lua_State* L)
{
	//getNpcState(cid)
	uint32_t cid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();
	const Player* player = env->getPlayerByUID(cid);
	if(!player)
	{
		lua_pushnil(L);
		return 1;
	}

	Npc* npc = env->getNpc();
	if(!npc)
	{
		lua_pushnil(L);
		return 1;
	}

	NpcState* state = npc->getState(player);
	NpcScriptInterface::pushState(L, state);
	return 1;
}

int32_t NpcScriptInterface::luaSetNpcState(lua_State* L)
{
	//setNpcState(state, cid)
	uint32_t cid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();
	const Player* player = env->getPlayerByUID(cid);
	if(!player)
	{
		lua_pushnil(L);
		return 1;
	}

	Npc* npc = env->getNpc();
	if(!npc)
	{
		lua_pushboolean(L, false);
		return 1;
	}

	NpcState* state = npc->getState(player);
	NpcScriptInterface::popState(L, state);
	lua_pushboolean(L, true);
	return 1;
}

int32_t NpcScriptInterface::luaGetNpcCid(lua_State* L)
{
	//getNpcCid()
	ScriptEnvironment* env = getScriptEnv();

	Npc* npc = env->getNpc();
	if(npc)
	{
		uint32_t cid = env->addThing(npc);
		lua_pushnumber(L, cid);
	}
	else
		lua_pushnil(L);

	return 1;
}

int32_t NpcScriptInterface::luaGetNpcName(lua_State* L)
{
	//getNpcName()
	ScriptEnvironment* env = getScriptEnv();

	Npc* npc = env->getNpc();
	if(npc)
		lua_pushstring(L, npc->getName().c_str());
	else
		lua_pushstring(L, "");

	return 1;
}

int32_t NpcScriptInterface::luaGetNpcParameter(lua_State* L)
{
	//getNpcParameter(paramKey)
	std::string paramKey = popString(L);

	ScriptEnvironment* env = getScriptEnv();

	Npc* npc = env->getNpc();
	if(npc)
	{
		Npc::ParametersMap::iterator it = npc->m_parameters.find(paramKey);
		if(it != npc->m_parameters.end())
			lua_pushstring(L, it->second.c_str());
		else
			lua_pushnil(L);
	}
	else
		lua_pushnil(L);

	return 1;
}

void NpcScriptInterface::pushState(lua_State* L, NpcState* state)
{
	lua_newtable(L);
	setField(L, "price", state->price);
	setField(L, "amount", state->amount);
	setField(L, "itemid", state->itemId);
	setField(L, "subtype", state->subType);
	setField(L, "topic", state->topic);
	setField(L, "level", state->level);
	setField(L, "spellname", state->spellName);
	setField(L, "listname", state->listName);
	setField(L, "listpname", state->listPluralName);
	setFieldBool(L, "isidle", state->isIdle);

	setField(L, "n1", state->scriptVars.n1);
	setField(L, "n2", state->scriptVars.n2);
	setField(L, "n3", state->scriptVars.n3);

	setFieldBool(L, "b1", state->scriptVars.b1);
	setFieldBool(L, "b2", state->scriptVars.b2);
	setFieldBool(L, "b3", state->scriptVars.b3);

	setField(L, "s1", state->scriptVars.s1);
	setField(L, "s2", state->scriptVars.s2);
	setField(L, "s3", state->scriptVars.s3);
}

void NpcScriptInterface::popState(lua_State* L, NpcState* &state)
{
	state->price = getField(L, "price");
	state->amount = getField(L, "amount");
	state->itemId = getField(L, "itemid");
	state->subType = getField(L, "subtype");
	state->topic = getField(L, "topic");
	state->level = getField(L, "level");
	state->spellName = getFieldString(L, "spellname");
	state->listName = getFieldString(L, "listname");
	state->listPluralName = getFieldString(L, "listpname");
	state->isIdle = getFieldBool(L, "isidle");

	state->scriptVars.n1 = getField(L, "n1");
	state->scriptVars.n2 = getField(L, "n2");
	state->scriptVars.n3 = getField(L, "n3");

	state->scriptVars.b1 = getFieldBool(L, "b1");
	state->scriptVars.b2 = getFieldBool(L, "b2");
	state->scriptVars.n3 = getFieldBool(L, "b3");

	state->scriptVars.s1 = getFieldString(L, "s1");
	state->scriptVars.s2 = getFieldString(L, "s2");
	state->scriptVars.s3 = getFieldString(L, "s3");
}

NpcEventsHandler::NpcEventsHandler(Npc* npc)
{
	m_npc = npc;
	m_loaded = false;
}

NpcEventsHandler::~NpcEventsHandler()
{
	//
}

bool NpcEventsHandler::isLoaded()
{
	return m_loaded;
}

NpcScript::NpcScript(std::string file, Npc* npc) :
	NpcEventsHandler(npc)
{
	m_scriptInterface = npc->getScriptInterface();

	if(m_scriptInterface->loadFile(file, npc) == -1)
	{
		std::cout << "[Warning - NpcScript::NpcScript] Can not load script: " << file << std::endl;
		std::cout << m_scriptInterface->getLastLuaError() << std::endl;
		m_loaded = false;
		return;
	}

	m_onCreatureSay = m_scriptInterface->getEvent("onCreatureSay");
	m_onCreatureDisappear = m_scriptInterface->getEvent("onCreatureDisappear");
	m_onCreatureAppear = m_scriptInterface->getEvent("onCreatureAppear");
	m_onCreatureMove = m_scriptInterface->getEvent("onCreatureMove");
	m_onThink = m_scriptInterface->getEvent("onThink");
	m_loaded = true;
}

NpcScript::~NpcScript()
{
	//
}

void NpcScript::onCreatureAppear(const Creature* creature)
{
	if(m_onCreatureAppear == -1)
		return;

	//onCreatureAppear(creature)
	if(m_scriptInterface->reserveScriptEnv())
	{
		ScriptEnvironment* env = m_scriptInterface->getScriptEnv();

		#ifdef __DEBUG_LUASCRIPTS__
		std::stringstream desc;
		desc << "npc " << m_npc->getName();
		env->setEventDesc(desc.str());
		#endif

		lua_State* L = m_scriptInterface->getLuaState();

		env->setScriptId(m_onCreatureAppear, m_scriptInterface);
		env->setRealPos(m_npc->getPosition());
		env->setNpc(m_npc);

		uint32_t cid = env->addThing(const_cast<Creature*>(creature));

		m_scriptInterface->pushFunction(m_onCreatureAppear);
		lua_pushnumber(L, cid);
		m_scriptInterface->callFunction(1);
		m_scriptInterface->releaseScriptEnv();
	}
	else
		std::cout << "[Error - NpcScript::onCreatureAppear] NPC Name: " << m_npc->getName() << " - Call stack overflow" << std::endl;
}

void NpcScript::onCreatureDisappear(const Creature* creature)
{
	if(m_onCreatureDisappear == -1)
		return;

	//onCreatureDisappear(id)
	if(m_scriptInterface->reserveScriptEnv())
	{
		ScriptEnvironment* env = m_scriptInterface->getScriptEnv();

		#ifdef __DEBUG_LUASCRIPTS__
		std::stringstream desc;
		desc << "npc " << m_npc->getName();
		env->setEventDesc(desc.str());
		#endif

		lua_State* L = m_scriptInterface->getLuaState();

		env->setScriptId(m_onCreatureDisappear, m_scriptInterface);
		env->setRealPos(m_npc->getPosition());
		env->setNpc(m_npc);

		uint32_t cid = env->addThing(const_cast<Creature*>(creature));

		m_scriptInterface->pushFunction(m_onCreatureDisappear);
		lua_pushnumber(L, cid);
		m_scriptInterface->callFunction(1);
		m_scriptInterface->releaseScriptEnv();
	}
	else
		std::cout << "[Error - NpcScript::onCreatureDisappear] NPC Name: " << m_npc->getName() << " - Call stack overflow" << std::endl;
}

void NpcScript::onCreatureMove(const Creature* creature, const Position& oldPos, const Position& newPos)
{
	if(m_onCreatureMove == -1)
		return;

	//onCreatureMove(creature, oldPos, newPos)
	if(m_scriptInterface->reserveScriptEnv())
	{
		ScriptEnvironment* env = m_scriptInterface->getScriptEnv();

		#ifdef __DEBUG_LUASCRIPTS__
		std::stringstream desc;
		desc << "npc " << m_npc->getName();
		env->setEventDesc(desc.str());
		#endif

		lua_State* L = m_scriptInterface->getLuaState();

		env->setScriptId(m_onCreatureMove, m_scriptInterface);
		env->setRealPos(m_npc->getPosition());
		env->setNpc(m_npc);

		uint32_t cid = env->addThing(const_cast<Creature*>(creature));

		m_scriptInterface->pushFunction(m_onCreatureMove);
		lua_pushnumber(L, cid);
		LuaScriptInterface::pushPosition(L, oldPos, 0);
		LuaScriptInterface::pushPosition(L, newPos, 0);
		m_scriptInterface->callFunction(3);
		m_scriptInterface->releaseScriptEnv();
	}
	else
		std::cout << "[Error - NpcScript::onCreatureMove] NPC Name: " << m_npc->getName() << " - Call stack overflow" << std::endl;
}

void NpcScript::onCreatureSay(const Creature* creature, SpeakClasses type, const std::string& text)
{
	if(m_onCreatureSay == -1)
		return;

	//onCreatureSay(cid, type, msg)
	if(m_scriptInterface->reserveScriptEnv())
	{
		ScriptEnvironment* env = m_scriptInterface->getScriptEnv();

		#ifdef __DEBUG_LUASCRIPTS__
		std::stringstream desc;
		desc << "npc " << m_npc->getName();
		env->setEventDesc(desc.str());
		#endif

		env->setScriptId(m_onCreatureSay, m_scriptInterface);
		env->setRealPos(m_npc->getPosition());
		env->setNpc(m_npc);

		uint32_t cid = env->addThing(const_cast<Creature*>(creature));

		lua_State* L = m_scriptInterface->getLuaState();
		m_scriptInterface->pushFunction(m_onCreatureSay);
		lua_pushnumber(L, cid);
		lua_pushnumber(L, type);
		lua_pushstring(L, text.c_str());
		m_scriptInterface->callFunction(3);
		m_scriptInterface->releaseScriptEnv();
	}
	else
		std::cout << "[Error - NpcScript::onCreatureSay] NPC Name: " << m_npc->getName() << " - Call stack overflow" << std::endl;
}

void NpcScript::onThink()
{
	if(m_onThink == -1)
		return;

	//onThink()
	if(m_scriptInterface->reserveScriptEnv())
	{
		ScriptEnvironment* env = m_scriptInterface->getScriptEnv();

		#ifdef __DEBUG_LUASCRIPTS__
		std::stringstream desc;
		desc << "npc " << m_npc->getName();
		env->setEventDesc(desc.str());
		#endif

		env->setScriptId(m_onThink, m_scriptInterface);
		env->setRealPos(m_npc->getPosition());
		env->setNpc(m_npc);

		m_scriptInterface->pushFunction(m_onThink);
		m_scriptInterface->callFunction(0);
		m_scriptInterface->releaseScriptEnv();
	}
	else
		std::cout << "[Error - NpcScript::onThink] NPC Name: " << m_npc->getName() << " - Call stack overflow" << std::endl;
}
