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
#include "const.h"
#include "player.h"
#include "monster.h"
#include "npc.h"
#include "game.h"
#include "item.h"
#include "container.h"
#include "combat.h"
#include "depot.h"
#include "house.h"
#include "tasks.h"
#include "tools.h"
#include "spells.h"
#include "configmanager.h"
#include "beds.h"
#include "actions.h"

extern Game g_game;
extern Spells* g_spells;
extern Actions* g_actions;
extern ConfigManager g_config;

Actions::Actions() :
m_scriptInterface("Action Interface")
{
	m_scriptInterface.initState();
}

Actions::~Actions()
{
	clear();
}

inline void Actions::clearMap(ActionUseMap& map)
{
	ActionUseMap::iterator it = map.begin();
	while(it != map.end())
	{
		delete it->second;
		map.erase(it);
		it = map.begin();
	}
}

void Actions::clear()
{
	clearMap(useItemMap);
	clearMap(uniqueItemMap);
	clearMap(actionItemMap);

	m_scriptInterface.reInitState();
}

LuaScriptInterface& Actions::getScriptInterface()
{
	return m_scriptInterface;
}

std::string Actions::getScriptBaseName()
{
	return "actions";
}

Event* Actions::getEvent(const std::string& nodeName)
{
	if(asLowerCaseString(nodeName) == "action")
		return new Action(&m_scriptInterface);

	return NULL;
}

bool Actions::registerEvent(Event* event, const pugi::xml_node& node)
{
	Action* action = dynamic_cast<Action*>(event);
	if(!action)
		return false;

	pugi::xml_attribute attr;
	if((attr = node.attribute("itemid")))
	{
		uint16_t id = pugi::cast<uint16_t>(attr.value());
		if(useItemMap.find(id) != useItemMap.end())
		{
			std::clog << "[Warning - Actions::registerEvent] Duplicate registered item with id: " << id << std::endl;
			return false;
		}
		useItemMap[id] = action;
	}
	else if((attr = node.attribute("fromid")))
	{
		pugi::xml_attribute toIdAttribute = node.attribute("toid");
		if(toIdAttribute)
		{
			uint16_t fromId = pugi::cast<uint16_t>(attr.value());
			uint16_t iterId = fromId;
			uint16_t toId = pugi::cast<uint16_t>(toIdAttribute.value());

			bool success = false;
			if(useItemMap.find(iterId) != useItemMap.end())
			{
				std::clog << "[Warning - Actions::registerEvent] Duplicate registered item with id: " << iterId << " in fromid: " << fromId << ", toid: " << toId << std::endl;
			}
			else
			{
				useItemMap[iterId] = action;
				success = true;
			}

			while (++iterId <= toId)
			{
				if(useItemMap.find(iterId) != useItemMap.end())
				{
					std::clog << "[Warning - Actions::registerEvent] Duplicate registered item with id: " << iterId << " in fromid: " << fromId << ", toid: " << toId << std::endl;
					continue;
				}
				useItemMap[iterId] = action;
				success = true;
			}
			return success;
		}
		else
		{
			std::clog << "[Warning - Actions::registerEvent] Missing toid in fromid: " << attr.as_string() << std::endl;
			return false;
		}
	}
	else if((attr = node.attribute("uniqueid")))
	{
		uint16_t uid = pugi::cast<uint16_t>(attr.value());
		if(uniqueItemMap.find(uid) != uniqueItemMap.end())
		{
			std::clog << "[Warning - Actions::registerEvent] Duplicate registered item with uniqueid: " << uid << std::endl;
			return false;
		}
		uniqueItemMap[uid] = action;
	}
	else if((attr = node.attribute("fromuid")))
	{
		pugi::xml_attribute toUidAttribute = node.attribute("touid");
		if(toUidAttribute)
		{
			uint16_t fromUid = pugi::cast<uint16_t>(attr.value());
			uint16_t iterUid = fromUid;
			uint16_t toUid = pugi::cast<uint16_t>(toUidAttribute.value());

			bool success = false;
			if(uniqueItemMap.find(iterUid) != uniqueItemMap.end())
				std::clog << "[Warning - Actions::registerEvent] Duplicate registered item with unique id: " << iterUid << " in fromuid: " << fromUid << ", touid: " << toUid << std::endl;
			else
			{
				uniqueItemMap[iterUid] = action;
				success = true;
			}

			while (++iterUid <= toUid)
			{
				if(uniqueItemMap.find(iterUid) != uniqueItemMap.end())
				{
					std::clog << "[Warning - Actions::registerEvent] Duplicate registered item with unique id: " << iterUid << " in fromuid: " << fromUid << ", touid: " << toUid << std::endl;
					continue;
				}
				uniqueItemMap[iterUid] = action;
				success = true;
			}
			return success;
		}
		else
		{
			std::clog << "[Warning - Actions::registerEvent] Missing touid in fromuid: " << attr.as_string() << std::endl;
			return false;
		}
	}
	else if((attr = node.attribute("actionid")))
	{
		uint16_t aid = pugi::cast<uint16_t>(attr.value());
		if(actionItemMap.find(aid) != actionItemMap.end())
		{
			std::clog << "[Warning - Actions::registerEvent] Duplicate registered item with actionid: " << aid << std::endl;
			return false;
		}
		actionItemMap[aid] = action;
	}
	else if((attr = node.attribute("fromaid")))
	{
		pugi::xml_attribute toAidAttribute = node.attribute("toaid");
		if(toAidAttribute)
		{
			uint16_t fromAid = pugi::cast<uint16_t>(attr.value());
			uint16_t iterAid = fromAid;
			uint16_t toAid = pugi::cast<uint16_t>(toAidAttribute.value());

			bool success = false;
			if(actionItemMap.find(iterAid) != actionItemMap.end())
				std::clog << "[Warning - Actions::registerEvent] Duplicate registered item with action id: " << iterAid << " in fromaid: " << fromAid << ", toaid: " << toAid << std::endl;
			else
			{
				actionItemMap[iterAid] = action;
				success = true;
			}

			while (++iterAid <= toAid)
			{
				if(actionItemMap.find(iterAid) != actionItemMap.end())
				{
					std::clog << "[Warning - Actions::registerEvent] Duplicate registered item with action id: " << iterAid << " in fromaid: " << fromAid << ", toaid: " << toAid << std::endl;
					continue;
				}
				actionItemMap[iterAid] = action;
				success = true;
			}
			return success;
		}
		else
		{
			std::clog << "[Warning - Actions::registerEvent] Missing toaid in fromaid: " << attr.as_string() << std::endl;
			return false;
		}
	}
	else
		return false;

	return true;
}

ReturnValue Actions::canUse(const Player* player, const Position& pos)
{
	const Position& playerPos = player->getPosition();
	if(pos.x != 0xFFFF)
	{
		if(playerPos.z > pos.z)
			return RET_FIRSTGOUPSTAIRS;
		else if(playerPos.z < pos.z)
			return RET_FIRSTGODOWNSTAIRS;
		else if(!Position::areInRange<1,1,0>(playerPos, pos))
			return RET_TOOFARAWAY;
	}
	return RET_NOERROR;
}

ReturnValue Actions::canUse(const Player* player, const Position& pos, const Item* item)
{
	Action* action = getAction(item);
	if(action)
		return action->canExecuteAction(player, pos);

	return RET_NOERROR;
}

ReturnValue Actions::canUseFar(const Creature* creature, const Position& toPos, bool checkLineOfSight)
{
	if(toPos.x == 0xFFFF)
		return RET_NOERROR;

	const Position& creaturePos = creature->getPosition();

	if(creaturePos.z > toPos.z)
		return RET_FIRSTGOUPSTAIRS;
	else if(creaturePos.z < toPos.z)
		return RET_FIRSTGODOWNSTAIRS;
	else if(!Position::areInRange<7,5,0>(toPos, creaturePos))
		return RET_TOOFARAWAY;
	
	if(checkLineOfSight && !g_game.canThrowObjectTo(creaturePos, toPos))
		return RET_CANNOTTHROW;

	return RET_NOERROR;
}

Action* Actions::getAction(const Item* item)
{
	if(item->getUniqueId() != 0)
	{
		ActionUseMap::iterator it = uniqueItemMap.find(item->getUniqueId());
		if(it != uniqueItemMap.end())
			return it->second;
	}
	
	if(item->getActionId() != 0)
	{
		ActionUseMap::iterator it = actionItemMap.find(item->getActionId());
		if(it != actionItemMap.end())
			return it->second;
	}
	
	ActionUseMap::iterator it = useItemMap.find(item->getID());
	if(it != useItemMap.end())
		return it->second;

	//rune items
	Action* runeSpell = g_spells->getRuneSpell(item->getID());
	if(runeSpell)
		return runeSpell;
	
	return NULL;
}

ReturnValue Actions::internalUseItem(Player* player, const Position& pos,
	uint8_t index, Item* item, uint32_t creatureId)
{
	bool executedAction = false;

	if(Door* door = item->getDoor())
	{
		if(!door->canUse(player))
			return RET_CANNOTUSETHISOBJECT;
	}

	if(BedItem* bed = item->getBed())
	{
		if(!bed->canUse(player))
			return RET_CANNOTUSETHISOBJECT;

		bed->sleep(player);
		executedAction = true;
	}

	Action* action = getAction(item);
	if(action)
	{
		int32_t stack = item->getParent()->__getIndexOfThing(item);
		PositionEx posEx(pos, stack);
		if(action->isScripted())
		{
			if(action->executeUse(player, item, posEx, posEx, false, creatureId))
				return RET_NOERROR;
		}
		else
		{
			if(action->function)
			{
				if(action->function(player, item, posEx, posEx, false, creatureId))
					return RET_NOERROR;
			}
		}
	}

	if(item->isReadable())
	{
		if(item->canWriteText())
		{
			player->setWriteItem(item, item->getMaxWriteLength());
			player->sendTextWindow(item, item->getMaxWriteLength(), true);
		}
		else
		{
			player->setWriteItem(NULL);
			player->sendTextWindow(item, 0, false);
		}
		return RET_NOERROR;
	}

	if(Container* container = item->getContainer())
	{
		if(openContainer(player, container, index))
			return RET_NOERROR;
	}

	if(executedAction)
		return RET_NOERROR;

	return RET_CANNOTUSETHISOBJECT;
}

bool Actions::useItem(Player* player, const Position& pos, uint8_t index, Item* item, bool isHotkey)
{
	if(!player->canDoAction())
		return false;

	player->stopWalk();

	int32_t itemId = 0;
	uint32_t itemCount = 0;

	if(isHotkey)
	{
		int32_t subType = -1;
		if(item->hasSubType() && !item->hasCharges())
			subType = item->getSubType();

		itemCount = player->__getItemTypeCount(item->getID(), subType, false);
		itemId = item->getID();
	}

	ReturnValue ret = internalUseItem(player, pos, index, item, 0);
	if(ret != RET_NOERROR)
	{
		player->sendCancelMessage(ret);
		return false;
	}

	if(isHotkey)
		showUseHotkeyMessage(player, itemId, itemCount);

	player->setNextAction(OTSYS_TIME() + g_config.getNumber(ConfigManager::ACTIONS_DELAY_INTERVAL));
	return true;
}

bool Actions::useItemEx(Player* player, const Position& fromPos, const Position& toPos,
	uint8_t toStackPos, Item* item, bool isHotkey, uint32_t creatureId /* = 0*/)
{
	if(!player->canDoAction())
		return false;

	player->stopWalk();

	Action* action = getAction(item);
	if(!action)
	{
		player->sendCancelMessage(RET_CANNOTUSETHISOBJECT);
		return false;
	}

	ReturnValue ret = action->canExecuteAction(player, toPos);
	if(ret != RET_NOERROR)
	{
		player->sendCancelMessage(ret);
		return false;
	}

	int32_t itemId = 0;
	uint32_t itemCount = 0;

	if(isHotkey)
	{
		int32_t subType = -1;
		if(item->hasSubType() && !item->hasCharges())
			subType = item->getSubType();

		itemCount = player->__getItemTypeCount(item->getID(), subType, false);
		itemId = item->getID();
	}

	int32_t fromStackPos = item->getParent()->__getIndexOfThing(item);
	PositionEx fromPosEx(fromPos, fromStackPos);
	PositionEx toPosEx(toPos, toStackPos);

	if(!action->executeUse(player, item, fromPosEx, toPosEx, true, creatureId))
	{
		if(!action->hasOwnErrorHandler())
			player->sendCancelMessage(RET_CANNOTUSETHISOBJECT);
		return false;
	}

	if(isHotkey)
		showUseHotkeyMessage(player, itemId, itemCount);

	player->setNextAction(OTSYS_TIME() + g_config.getNumber(ConfigManager::EX_ACTIONS_DELAY_INTERVAL));
	return true;
}

void Actions::showUseHotkeyMessage(Player* player, int32_t id, uint32_t count)
{
	const ItemType& it = Item::items[id];
	std::ostringstream ss;
	if(!it.showCount)
		ss << "Using one of " << it.name << "...";
	else if(count == 1)
		ss << "Using the last " << it.name << "...";
	else
		ss << "Using one of " << count << " " << it.getPluralName() << "...";

	player->sendTextMessage(MSG_INFO_DESCR, ss.str());
}

bool Actions::openContainer(Player* player, Container* container, const uint8_t index)
{
	Container* openContainer = NULL;

	//depot container
	if(Depot* depot = container->getDepot())
	{
		Depot* myDepot = player->getDepot(depot->getDepotId(), true);
		myDepot->setParent(depot->getParent());
		openContainer = myDepot;
	}
	else
		openContainer = container;
	
	if(container->getCorpseOwner() != 0)
	{
		if(!player->canOpenCorpse(container->getCorpseOwner()))
		{
			player->sendCancel("You are not the owner.");
			return true;
		}
	}

	//open/close container
	int32_t oldcid = player->getContainerID(openContainer);
	if(oldcid != -1)
	{
		player->onCloseContainer(openContainer);
		player->closeContainer(oldcid);
	}
	else
	{
		player->addContainer(index, openContainer);
		player->onSendContainer(openContainer);
	}

	return true;
}

Action::Action(LuaScriptInterface* _interface) :
	Event(_interface)
{
	allowFarUse = false;
	checkLineOfSight = true;
}

Action::Action(const Action *copy) :
	Event(copy)
{
	allowFarUse = copy->allowFarUse;
	checkLineOfSight = copy->checkLineOfSight;
}

Action::~Action()
{
	//
}

bool Action::configureEvent(const pugi::xml_node& node)
{
	pugi::xml_attribute allowFarUseAttr = node.attribute("allowfaruse");
	if(allowFarUseAttr)
		setAllowFarUse(allowFarUseAttr.as_bool());

	pugi::xml_attribute blockWallsAttr = node.attribute("blockwalls");
	if(blockWallsAttr)
		setCheckLineOfSight(blockWallsAttr.as_bool());

	return true;
}

bool Action::loadFunction(const std::string& functionName)
{
	std::string tmpFunctionName = asLowerCaseString(functionName);
	if(tmpFunctionName == "increaseitemid")
		function = increaseItemId;
	else if(tmpFunctionName == "decreaseitemid")
		function = decreaseItemId;
	else if(tmpFunctionName == "highscorebook")
		function = highscoreBook;
	else
	{
		std::clog << "[Warning - Action::loadFunction] Function \"" << functionName << "\" does not exist." << std::endl;
		return false;
	}

	m_scripted = false;
	return true;
}

bool Action::highscoreBook(Player* player, Item* item, const PositionEx& posFrom, const PositionEx& posTo, bool extendedUse, uint32_t creatureId)
{
	if(player)
	{
		if(item)
		{
			if(item->getActionId() >= 150 && item->getActionId() <= 158)
			{
				std::string highscoreString = g_game.getHighscoreString(item->getActionId() - 150);
				item->setText(highscoreString);
				player->sendTextWindow(item, highscoreString.size(), false);
				return true;
			}
		}
	}
	return false;
}

bool Action::increaseItemId(Player* player, Item* item, const PositionEx& posFrom, const PositionEx& posTo, bool extendedUse, uint32_t creatureId)
{
	if(player)
	{
		if(item)
		{
			g_game.transformItem(item, item->getID() + 1);
			return true;
		}
	}
	return false;
}

bool Action::decreaseItemId(Player* player, Item* item, const PositionEx& posFrom, const PositionEx& posTo, bool extendedUse, uint32_t creatureId)
{
	if(player)
	{
		if(item)
		{
			g_game.transformItem(item, item->getID() - 1);
			return true;
		}
	}
	return false;
}

std::string Action::getScriptEventName()
{
	return "onUse";
}

ReturnValue Action::canExecuteAction(const Player* player, const Position& toPos)
{
	ReturnValue ret = RET_NOERROR;
	if(!getAllowFarUse())
		ret = g_actions->canUse(player, toPos);
	else
		ret = g_actions->canUseFar(player, toPos, getCheckLineOfSight());
	return ret;
}

bool Action::executeUse(Player* player, Item* item, const PositionEx& fromPos, const PositionEx& toPos, bool extendedUse, uint32_t creatureId)
{
	//onUse(cid, item, fromPosition, itemEx, toPosition)
	if(m_scriptInterface->reserveScriptEnv())
	{
		ScriptEnvironment* env = m_scriptInterface->getScriptEnv();
	
		env->setScriptId(m_scriptId, m_scriptInterface);
		env->setRealPos(player->getPosition());
	
		uint32_t cid = env->addThing(player);
		uint32_t itemid1 = env->addThing(item);
	
		lua_State* L = m_scriptInterface->getLuaState();
	
		m_scriptInterface->pushFunction(m_scriptId);
		lua_pushnumber(L, cid);
		LuaScriptInterface::pushThing(L, item, itemid1);
		LuaScriptInterface::pushPosition(L, fromPos, fromPos.stackpos);
		//std::clog << "posTo" <<  (Position)posTo << " stack" << (int32_t)posTo.stackpos <<std::endl;
		Thing* thing = g_game.internalGetThing(player, toPos, toPos.stackpos);
		if(thing && (!extendedUse || thing != item))
		{
			uint32_t thingId2 = env->addThing(thing);
			LuaScriptInterface::pushThing(L, thing, thingId2);
			LuaScriptInterface::pushPosition(L, toPos, toPos.stackpos);
		}
		else
		{
			LuaScriptInterface::pushThing(L, NULL, 0);
			Position posEx;
			LuaScriptInterface::pushPosition(L, posEx, 0);
		}
	
		bool result = m_scriptInterface->callFunction(5);
		m_scriptInterface->releaseScriptEnv();
		return result;
	}
	else
	{
		std::clog << "[Error] Call stack overflow. Action::executeUse" << std::endl;
		return false;
	}
}
