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

#include <string>
#include <fstream>
#include <utility>

#include "commands.h"
#include "player.h"
#include "npc.h"
#include "monsters.h"
#include "game.h"
#include "actions.h"
#include "house.h"
#include "iologindata.h"
#include "tools.h"
#include "ban.h"
#include "configmanager.h"
#include "town.h"
#include "spells.h"
#include "talkaction.h"
#include "movement.h"
#include "spells.h"
#include "weapons.h"
#include "raids.h"
#include "chat.h"
#include "status.h"
#include "globalevent.h"

extern ConfigManager g_config;
extern Actions* g_actions;
extern Monsters g_monsters;
extern Npcs g_npcs;
extern TalkActions* g_talkActions;
extern MoveEvents* g_moveEvents;
extern Spells* g_spells;
extern Weapons* g_weapons;
extern Game g_game;
extern Chat g_chat;
extern CreatureEvents* g_creatureEvents;
extern GlobalEvents* g_globalEvents;

#define ipText(a) (unsigned int)a[0] << "." << (unsigned int)a[1] << "." << (unsigned int)a[2] << "." << (unsigned int)a[3]

s_defcommands Commands::defined_commands[] =
{
	//admin commands
	{"/s", &Commands::placeNpc},
	{"/m", &Commands::placeMonster},
	{"/summon", &Commands::placeSummon},
	{"/B", &Commands::broadcastMessage},
	{"/b", &Commands::banPlayer},
	{"/t", &Commands::teleportMasterPos},
	{"/c", &Commands::teleportHere},
	{"/q", &Commands::subtractMoney},
	{"/reload", &Commands::reloadInfo},
	{"/goto", &Commands::teleportTo},
	{"/info", &Commands::getInfo},
	{"/a", &Commands::teleportNTiles},
	{"/kick", &Commands::kickPlayer},
	{"/owner", &Commands::setHouseOwner},
	{"/gethouse", &Commands::getHouse},
	{"/town", &Commands::teleportToTown},
	{"/up", &Commands::changeFloor},
	{"/down", &Commands::changeFloor},
	{"/pos", &Commands::showPosition},
	{"/r", &Commands::removeThing},
	{"/newtype", &Commands::newType},
	{"/raid", &Commands::forceRaid},
	{"/addskill", &Commands::addSkill},
	{"/unban", &Commands::unban},
	{"/ghost", &Commands::ghost},

	//player commands - TODO: make them talkactions
	{"!buyhouse", &Commands::buyHouse},
 	{"!sellhouse", &Commands::sellHouse},
	{"!serverinfo", &Commands::serverInfo},
	{"!kills", &Commands::playerKills},
 	{"!createguild", &Commands::createGuild},
 	{"!joinguild", &Commands::joinGuild}
};

Commands::Commands()
{
	loaded = false;

	//setup command map
	for(uint32_t i = 0; i < sizeof(defined_commands) / sizeof(defined_commands[0]); i++)
	{
		Command* cmd = new Command;
		cmd->loadedGroupId = false;
		cmd->loadedAccountType = false;
		cmd->groupId = 1;
		cmd->f = defined_commands[i].f;
		std::string key = defined_commands[i].name;
		commandMap[key] = cmd;
	}
}

bool Commands::loadFromXml()
{
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file("data/XML/commands.xml");
	if (!result) {
		std::cout << "[Error - Commands::loadFromXml] Failed to load data/XML/commands.xml: " << result.description() << std::endl;
		return false;
	}

	loaded = true;

	for (pugi::xml_node commandNode = doc.child("commands").first_child(); commandNode; commandNode = commandNode.next_sibling()) {
		pugi::xml_attribute cmdAttribute = commandNode.attribute("cmd");
		if (!cmdAttribute) {
			std::cout << "[Warning - Commands::loadFromXml] Missing cmd" << std::endl;
			continue;
		}

		auto it = commandMap.find(cmdAttribute.as_string());
		if (it == commandMap.end()) {
			std::cout << "[Warning - Commands::loadFromXml] Unknown command " << cmdAttribute.as_string() << std::endl;
			continue;
		}

		Command* command = it->second;

		pugi::xml_attribute groupAttribute = commandNode.attribute("group");
		if (groupAttribute) {
			if (!command->loadedGroupId) {
				command->groupId = pugi::cast<uint32_t>(groupAttribute.value());
				command->loadedGroupId = true;
			} else {
				std::cout << "[Notice - Commands::loadFromXml] Duplicate command: " << it->first << std::endl;
			}
		}

		pugi::xml_attribute acctypeAttribute = commandNode.attribute("acctype");
		if (acctypeAttribute) {
			if (!command->loadedAccountType) {
				command->accountType = (AccountType_t)pugi::cast<uint32_t>(acctypeAttribute.value());
				command->loadedAccountType = true;
			} else {
				std::cout << "[Notice - Commands::loadFromXml] Duplicate command: " << it->first << std::endl;
			}
		}
	}

	for (const auto& it : commandMap) {
		Command* command = it.second;
		if (!command->loadedGroupId) {
			std::cout << "[Warning - Commands::loadFromXml] Missing group id for command " << it.first << std::endl;
		}

		if (!command->loadedAccountType) {
			std::cout << "[Warning - Commands::loadFromXml] Missing acctype level for command " << it.first << std::endl;
		}

		g_game.addCommandTag(it.first[0]);
	}
	return loaded;
}

bool Commands::reload()
{
	loaded = false;
	for(CommandMap::iterator it = commandMap.begin(); it != commandMap.end(); ++it)
	{
		it->second->groupId = 1;
		it->second->accountType = ACCOUNT_TYPE_GOD;
		it->second->loadedGroupId = false;
		it->second->loadedAccountType = false;
	}
	g_game.resetCommandTag();
	return loadFromXml();
}

bool Commands::exeCommand(Creature* creature, const std::string& cmd)
{
	std::string str_command;
	std::string str_param;
	
	std::string::size_type loc = cmd.find( ' ', 0 );
	if(loc != std::string::npos && loc >= 0)
	{
		str_command = std::string(cmd, 0, loc);
		str_param = std::string(cmd, (loc + 1), cmd.size() - loc - 1);
	}
	else
	{
		str_command = cmd;
		str_param = std::string(""); 
	}
	
	//find command
	CommandMap::iterator it = commandMap.find(str_command);
	if(it == commandMap.end())
		return false;

	Player* player = creature->getPlayer();
	if(player && it->second->groupId > player->groupId || it->second->accountType > player->accountType || player->name == "Account Manager")
	{
		if(player->getAccessLevel() > 0)
			player->sendTextMessage(MSG_STATUS_SMALL, "You can not execute this command.");
		return false;
	}

	//execute command
	CommandFunc cfunc = it->second->f;
	(this->*cfunc)(creature, str_command, str_param);
	if(player)
	{
		if(player->getAccessLevel() > 0)
		{
			player->sendTextMessage(MSG_STATUS_CONSOLE_RED, cmd.c_str());
			time_t ticks = time(NULL);
			const tm* now = localtime(&ticks);
			char buf[32], buffer[85];
			strftime(buf, sizeof(buf), "%d/%m/%Y %H:%M", now);
			sprintf(buffer, "data/logs/%s commands.log", player->name.c_str());
			std::ofstream out(buffer, std::ios::app);
			out << "[" << buf << "] " << cmd << std::endl;
			out.close();
		}
	}
	return true;
}

bool Commands::placeNpc(Creature* creature, const std::string& cmd, const std::string& param)
{
	Npc* npc = Npc::createNpc(param);
	if(!npc)
		return false;

	// Place the npc
	if(g_game.placeCreature(npc, creature->getPosition()))
	{
		g_game.addMagicEffect(creature->getPosition(), NM_ME_MAGIC_BLOOD);
		return true;
	}
	else
	{
		delete npc;
		Player* player = creature->getPlayer();
		if(player)
		{
			player->sendCancelMessage(RET_NOTENOUGHROOM);
			g_game.addMagicEffect(creature->getPosition(), NM_ME_POFF);
		}
		return true;
	}
	return false;
}

bool Commands::placeMonster(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();

	Monster* monster = Monster::createMonster(param);
	if(!monster)
	{
		if(player)
		{
			player->sendCancelMessage(RET_NOTPOSSIBLE);
			g_game.addMagicEffect(player->getPosition(), NM_ME_POFF);
		}
		return false;
	}

	// Place the monster
	if(g_game.placeCreature(monster, creature->getPosition()))
	{
		g_game.addMagicEffect(creature->getPosition(), NM_ME_MAGIC_BLOOD);
		return true;
	}
	else
	{
		delete monster;
		if(player)
		{
			player->sendCancelMessage(RET_NOTENOUGHROOM);
			g_game.addMagicEffect(player->getPosition(), NM_ME_POFF);
		}
	}
	return false;
}

ReturnValue Commands::placeSummon(Creature* creature, const std::string& name)
{
	Monster* monster = Monster::createMonster(name);
	if(!monster)
		return RET_NOTPOSSIBLE;
	
	// Place the monster
	creature->addSummon(monster);
	if(!g_game.placeCreature(monster, creature->getPosition()))
	{
		creature->removeSummon(monster);
		return RET_NOTENOUGHROOM;
	}
	return RET_NOERROR;
}

bool Commands::placeSummon(Creature* creature, const std::string& cmd, const std::string& param)
{
	ReturnValue ret = placeSummon(creature, param);
	if(ret != RET_NOERROR)
	{
		if(Player* player = creature->getPlayer())
		{
			player->sendCancelMessage(ret);
			g_game.addMagicEffect(player->getPosition(), NM_ME_POFF);
		}
	}
	return (ret == RET_NOERROR);
}

bool Commands::broadcastMessage(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(!player)
		return false;

	return g_game.playerBroadcastMessage(player, param, SPEAK_BROADCAST);
}

bool Commands::banPlayer(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* playerBan = g_game.getPlayerByName(param);
	if(playerBan)
	{
		if(playerBan->hasFlag(PlayerFlag_CannotBeBanned))
		{
			if(Player* player = creature->getPlayer())
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

bool Commands::teleportMasterPos(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(player)
	{
		Player* targetPlayer = player;
		if(!param.empty())
		{
			if(Player* paramPlayer = g_game.getPlayerByName(param))
				targetPlayer = paramPlayer;
			else
			{
				player->sendCancel("Player not found.");
				return false;
			}
		}

		Position oldPosition = player->getPosition();
		Position destPos = player->masterPos;
		Position newPosition = g_game.getClosestFreeTile(player, 0, destPos, true);
		if(player->getPosition() != destPos)
		{
			if(newPosition.x == 0)
				player->sendCancel("You can not teleport there.");
			else if(g_game.internalTeleport(creature, newPosition, true) == RET_NOERROR)
			{
				g_game.addMagicEffect(oldPosition, NM_ME_POFF, targetPlayer->isInGhostMode());
				g_game.addMagicEffect(newPosition, NM_ME_TELEPORT, targetPlayer->isInGhostMode());
				return true;
			}
		}
	}
	return false;
}

bool Commands::teleportHere(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(player)
	{
		Creature* paramCreature = g_game.getCreatureByName(param);
		if(paramCreature)
		{
			Position oldPosition = paramCreature->getPosition();
			Position destPos = paramCreature->getPosition();
			Position newPosition = g_game.getClosestFreeTile(player, paramCreature, player->getPosition(), false);
			if(newPosition.x == 0)
			{
				char buffer[80];
				sprintf(buffer, "You can not teleport %s to you.", paramCreature->getName().c_str());
				player->sendCancel(buffer);
			}
			else if(g_game.internalTeleport(paramCreature, newPosition, true) == RET_NOERROR)
			{
				g_game.addMagicEffect(oldPosition, NM_ME_POFF, paramCreature->isInGhostMode());
				g_game.addMagicEffect(newPosition, NM_ME_TELEPORT, paramCreature->isInGhostMode());
				return true;
			}
		}
		else
			player->sendCancel("A creature with that name could not be found.");
	}
	return false;
}

bool Commands::subtractMoney(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(!player)
		return false;
				
	int32_t count = atoi(param.c_str());
	uint32_t money = g_game.getMoney(player);
	if(!count)
	{
		char info[35];
		sprintf(info, "You have %u gold.", money);
		player->sendCancel(info);
		return true;
	}
	else if(count > (int32_t)money)
	{
		char info[65];
		sprintf(info, "You have %u gold and is not sufficient.", money);
		player->sendCancel(info);
		return true;
	}

	if(!g_game.removeMoney(player, count))
		player->sendCancel("Can not subtract money!");

	return true;
}

bool Commands::reloadInfo(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(player)
	{
		std::string tmpParam = asLowerCaseString(param);
		if(tmpParam == "action" || tmpParam == "actions")
		{
			g_actions->reload();
			player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Reloaded actions.");
		}
		else if(tmpParam == "config" || tmpParam == "configuration")
		{
			g_config.reload();
			player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Reloaded config.");
		}
		else if(tmpParam == "command" || tmpParam == "commands")
		{
			reload();
			player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Reloaded commands.");
		}
		else if(tmpParam == "creaturescript" || tmpParam == "creaturescripts")
		{
			g_creatureEvents->reload();
			player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Reloaded creaturescripts.");
		}
		else if(tmpParam == "globalevent" || tmpParam == "globalevents")
		{
			g_globalEvents->reload();
			player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Reloaded global events.");
		}
		else if(tmpParam == "highscore" || tmpParam == "highscores")
		{
			g_game.reloadHighscores();
			player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Reloaded highscores.");
		}
		else if(tmpParam == "monster" || tmpParam == "monsters")
		{
			g_monsters.reload();
			player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Reloaded monsters.");
		}
		else if(tmpParam == "move" || tmpParam == "movement" || tmpParam == "movements"
			|| tmpParam == "moveevents" || tmpParam == "moveevent")
		{
			g_moveEvents->reload();
			player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Reloaded movements.");
		}
		else if(tmpParam == "npc" || tmpParam == "npcs")
		{
			g_npcs.reload();
			player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Reloaded npcs.");
		}
		else if(tmpParam == "raid" || tmpParam == "raids")
		{
			Raids::getInstance()->reload();
			Raids::getInstance()->startup();
			player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Reloaded raids.");
		}
		else if(tmpParam == "spell" || tmpParam == "spells")
		{
			g_spells->reload();
			g_monsters.reload();
			player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Reloaded spells.");
		}
		else if(tmpParam == "talk" || tmpParam == "talkaction" || tmpParam == "talkactions")
		{
			g_talkActions->reload();
			player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Reloaded talkactions.");
		}
		else
			player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Reload type not found.");
	}
	return true;
}

bool Commands::teleportToTown(Creature* creature, const std::string& cmd, const std::string& param)
{
	std::string tmp = param;
	Player* player = creature->getPlayer();
	if(!player)
		return false;

	Town* town = Towns::getInstance().getTown(tmp);
	if(town)
	{
		Position oldPosition = player->getPosition();
		Position newPosition = g_game.getClosestFreeTile(player, 0, town->getTemplePosition(), true);
		if(player->getPosition() != town->getTemplePosition())
		{
			if(newPosition.x == 0)
				player->sendCancel("You can not teleport there.");
			else if(g_game.internalTeleport(player, newPosition, true) == RET_NOERROR)
			{
				g_game.addMagicEffect(oldPosition, NM_ME_POFF, player->isInGhostMode());
				g_game.addMagicEffect(newPosition, NM_ME_TELEPORT, player->isInGhostMode());
				return true;
			}
		}
	}
	else
		player->sendCancel("Could not find the town.");
	return false;
}

bool Commands::teleportTo(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(!player)
		return true;
	
	Creature* targetCreature = g_game.getCreatureByName(param);
	if(targetCreature)
	{
		Position oldPosition = player->getPosition();
		Position newPosition = g_game.getClosestFreeTile(player, 0, targetCreature->getPosition(), true);
		if(newPosition.x > 0)
		{
			if(g_game.internalTeleport(player, newPosition, true) == RET_NOERROR)
			{
				bool ghostMode = false;
				if(player->isInGhostMode() || targetCreature->isInGhostMode())
					ghostMode = true;

				g_game.addMagicEffect(oldPosition, NM_ME_POFF, ghostMode);
				g_game.addMagicEffect(newPosition, NM_ME_TELEPORT, ghostMode);
				return true;
			}
		}
		else
		{
			char buffer[75];
			sprintf(buffer, "You can not teleport to %s.", targetCreature->getName().c_str());
			player->sendCancel(buffer);
		}
	}
	return false;
}

bool Commands::getInfo(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(!player)
		return true;
	
	Player* paramPlayer = g_game.getPlayerByName(param);
	if(paramPlayer)
	{
		if(player != paramPlayer && paramPlayer->getAccessLevel() >= player->getAccessLevel())
		{
			player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "You can not get info about this player.");
			return true;
		}
		uint8_t ip[4];
		*(uint32_t*)&ip = paramPlayer->lastIP;
		std::stringstream info;
		info << "name:    " << paramPlayer->name << std::endl <<
			"access:  " << paramPlayer->accessLevel << std::endl <<
			"level:   " << paramPlayer->level << std::endl <<
			"maglvl:  " << paramPlayer->magLevel << std::endl <<
			"speed:   " << paramPlayer->getSpeed() <<std::endl <<
			"position " << paramPlayer->getPosition() << std::endl << 
			"ip:      " << ipText(ip);
		player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, info.str().c_str());
	}
	else
		player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Player not found.");

	return true;
}

bool Commands::teleportNTiles(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(player)
	{
		int32_t ntiles = atoi(param.c_str());
		if(ntiles != 0)
		{
			Position oldPosition = player->getPosition();
			Position newPos = player->getPosition();
			switch(player->direction)
			{
				case NORTH: newPos.y -= ntiles; break;
				case SOUTH: newPos.y += ntiles; break;
				case EAST: newPos.x += ntiles; break;
				case WEST: newPos.x -= ntiles; break;
				default: break;
			}

			Position newPosition = g_game.getClosestFreeTile(player, 0, newPos, true);
			if(newPosition.x == 0)
				player->sendCancel("You can not teleport there.");
			else if(g_game.internalTeleport(player, newPosition, true) == RET_NOERROR)
			{
				if(ntiles != 1)
				{
					g_game.addMagicEffect(oldPosition, NM_ME_POFF, player->isInGhostMode());
					g_game.addMagicEffect(newPosition, NM_ME_TELEPORT, player->isInGhostMode());
				}
			}
		}
	}
	return true;
}

bool Commands::kickPlayer(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* playerKick = g_game.getPlayerByName(param);
	if(playerKick)
	{
		Player* player = creature->getPlayer();
		if(player && playerKick->accessLevel)
		{
			player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "You cannot kick this player.");
			return false;
		}

		playerKick->kickPlayer(true);
		return true;
	}
	return false;
}

bool Commands::setHouseOwner(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(player)
	{
		if(player->getTile()->hasFlag(TILESTATE_HOUSE))
		{
			HouseTile* houseTile = dynamic_cast<HouseTile*>(player->getTile());
			if(houseTile)
			{
				std::string real_name = param;
				uint32_t guid;
				if(param == "none")
					houseTile->getHouse()->setHouseOwner(0);
				else if(IOLoginData::getInstance()->getGuidByName(guid, real_name))
					houseTile->getHouse()->setHouseOwner(guid);
				else
					player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Player not found.");
				return true;
			}
		}
	}
	return false;
}

bool Commands::sellHouse(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(player)
	{
		House* house = Houses::getInstance().getHouseByPlayerId(player->guid);
		if(!house)
		{
			player->sendCancel("You do not own any house.");
			return false;
		}

		Player* tradePartner = g_game.getPlayerByName(param);
		if(!(tradePartner && tradePartner != player))
		{
			player->sendCancel("Trade player not found.");
			return false;
		}

		if(tradePartner->level < 1)
		{
			player->sendCancel("Trade player level is too low.");
			return false;
		}

		if(Houses::getInstance().getHouseByPlayerId(tradePartner->guid))
		{
			player->sendCancel("Trade player already owns a house.");
			return false;
		}

		if(!Position::areInRange<2,2,0>(tradePartner->getPosition(), player->getPosition()))
		{
			player->sendCancel("Trade player is too far away.");
			return false;
		}

		if(!tradePartner->isPremium())
		{
			player->sendCancel("Trade player does not have a premium account.");
			return false;
		}

		Item* transferItem = house->getTransferItem();
		if(!transferItem)
		{
			player->sendCancel("You can not trade this house.");
			return false;
		}

		transferItem->getParent()->setParent(player);
		if(g_game.internalStartTrade(player, tradePartner, transferItem))
			return true;
		else
			house->resetTransferItem();
	}
	return false;
}

bool Commands::getHouse(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(!player)
		return false;
	
	std::string real_name = param;
	uint32_t guid;
	if(IOLoginData::getInstance()->getGuidByName(guid, real_name))
	{
		House* house = Houses::getInstance().getHouseByPlayerId(guid);
		std::stringstream str;
		str << real_name;
		if(house)
			str << " owns house: " << house->getName() << ".";
		else
			str << " does not own any house.";

		player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, str.str().c_str());
	}
	return false;
}

bool Commands::serverInfo(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(!player)
		return false;
	
	std::stringstream text;
	text << "Server Info:";
	text << "\nExp Rate: " << g_game.getExperienceStage(player->level);
	text << "\nSkill Rate: " << g_config.getNumber(ConfigManager::RATE_SKILL);
	text << "\nMagic Rate: " << g_config.getNumber(ConfigManager::RATE_MAGIC);
	text << "\nLoot Rate: " << g_config.getNumber(ConfigManager::RATE_LOOT);
	player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, text.str().c_str());
	return true;
}

bool Commands::buyHouse(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(player)
	{
		Position pos = player->getPosition();
		pos = getNextPosition(player->direction, pos);
		for(HouseMap::iterator it = Houses::getInstance().getHouseBegin(); it != Houses::getInstance().getHouseEnd(); it++)
		{
			if(it->second->getHouseOwner() == player->guid)
			{
				player->sendCancel("You are already the owner of a house.");
				return false;
			}
		}
		if(Tile* tile = g_game.getTile(pos.x, pos.y, pos.z))
		{
			if(HouseTile* houseTile = dynamic_cast<HouseTile*>(tile))
			{
				if(House* house = houseTile->getHouse())
				{
					if(house->getDoorByPosition(pos))
					{
						if(!house->getHouseOwner())
						{
							if(player->isPremium())
							{
								uint32_t price = 0;
								for(HouseTileList::iterator it = house->getHouseTileBegin(); it != house->getHouseTileEnd(); it++)
									price += g_config.getNumber(ConfigManager::HOUSE_PRICE);
								if(price)
								{
									uint32_t money = g_game.getMoney(player);
									if(money >= price && g_game.removeMoney(player, price))
									{
										house->setHouseOwner(player->guid);
										player->sendTextMessage(MSG_INFO_DESCR, "You have successfully bought this house, be sure to have the money for the rent in your depot of this city.");
										return true;
									}
									else
										player->sendCancel("You do not have enough money.");
								}
								else
									player->sendCancel("That house doesn't contain any house tile.");
							}
							else
								player->sendCancelMessage(RET_YOUNEEDPREMIUMACCOUNT);
						}
						else
							player->sendCancel("This house alreadly has an owner.");
					}
					else
						player->sendCancel("You have to be looking at the door of the house you would like to buy.");
				}
				else
					player->sendCancel("You have to be looking at the door of the house you would like to buy.");
			}
			else
				player->sendCancel("You have to be looking at the door of the house you would like to buy.");
		}
		else
			player->sendCancel("You have to be looking at the door of the house you would like to buy.");
	}
	return false;
}

bool Commands::changeFloor(Creature* creature, const std::string &cmd, const std::string &param)
{
	Player* player = creature->getPlayer();
	if(!player)
		return true;

	Position newPos = player->getPosition();
	if(cmd[1] == 'u')
		newPos.z--;
	else
		newPos.z++;
	Position newPosition = g_game.getClosestFreeTile(player, 0, newPos, true);
	if(newPosition.x != 0)
	{
		Position oldPosition = player->getPosition();
		if(g_game.internalTeleport(creature, newPosition, true) == RET_NOERROR)
		{
			g_game.addMagicEffect(oldPosition, NM_ME_POFF, player->isInGhostMode());
			g_game.addMagicEffect(newPosition, NM_ME_TELEPORT, player->isInGhostMode());
			return true;
		}
	}
	player->sendCancel("You can not teleport there.");
	return false;
}

bool Commands::showPosition(Creature* creature, const std::string &cmd, const std::string &param)
{
	Player* player = creature->getPlayer();
	if(player)
	{
		char buffer[75];
		sprintf(buffer, "Your current position is [X: %d | Y: %d | Z: %d].", player->getPosition().x, player->getPosition().y, player->getPosition().z);
		player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, buffer);
	}
	return true;
}

bool Commands::removeThing(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(player)
	{
		Position pos = player->getPosition();
		pos = getNextPosition(player->direction, pos);
		Tile *removeTile = g_game.getMap()->getTile(pos);
		if(removeTile != NULL)
		{
			Thing *thing = removeTile->getTopThing();
			if(thing)
			{
				if(Creature *creature = thing->getCreature())
					g_game.removeCreature(creature, true);
				else
				{
					Item *item = thing->getItem();
					if(item && !item->isGroundTile())
					{
						g_game.internalRemoveItem(item, 1);
						g_game.addMagicEffect(pos, NM_ME_MAGIC_BLOOD);
					}
					else if(item && item->isGroundTile())
					{
						player->sendTextMessage(MSG_STATUS_SMALL, "You may not remove a ground tile.");
						g_game.addMagicEffect(pos, NM_ME_POFF);
						return false;
					}
				}
			}
			else
			{
				player->sendTextMessage(MSG_STATUS_SMALL, "No object found.");
				g_game.addMagicEffect(pos, NM_ME_POFF);
				return false;
			}
		}
		else
		{
			player->sendTextMessage(MSG_STATUS_SMALL, "No tile found.");
			g_game.addMagicEffect(pos, NM_ME_POFF);
			return false;
		}
	}
	return true;
}

bool Commands::newType(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	int32_t lookType = atoi(param.c_str());
	if(player)
	{
		if(lookType < 0 || lookType == 1 || lookType == 135 || lookType > 160 && lookType < 192 || lookType > 247)
			player->sendTextMessage(MSG_STATUS_SMALL, "This looktype does not exist.");
		else
		{
			g_game.internalCreatureChangeOutfit(creature, (const Outfit_t&)lookType);
			return true;
		}
	}
	return false;
}

bool Commands::forceRaid(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(!player)
		return false;

	Raid* raid = Raids::getInstance()->getRaidByName(param);
	if(!raid || !raid->isLoaded())
	{
		player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "No such raid exists.");
		return false;
	}

	if(Raids::getInstance()->getRunning())
	{
		player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Another raid is already being executed.");
		return false;
	}

	Raids::getInstance()->setRunning(raid);
	RaidEvent* event = raid->getNextRaidEvent();

	if(!event)
	{
		player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "The raid does not contain any data.");
		return false;
	}

	raid->setState(RAIDSTATE_EXECUTING);

	uint32_t ticks = event->getDelay();
	if(ticks > 0)
		Scheduler::getScheduler().addEvent(createSchedulerTask(ticks,
			boost::bind(&Raid::executeRaidEvent, raid, event)));
	else
		Dispatcher::getDispatcher().addTask(createTask(
			boost::bind(&Raid::executeRaidEvent, raid, event)));

	player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Raid started.");
	return true;
}

bool Commands::addSkill(Creature* creature, const std::string& cmd, const std::string& param)
{
	boost::char_separator<char> sep(",");
	tokenizer cmdtokens(param, sep);
	tokenizer::iterator cmdit = cmdtokens.begin();
	std::string param1, param2;
	param1 = parseParams(cmdit, cmdtokens.end());
	param2 = parseParams(cmdit, cmdtokens.end());
	trimString(param1);
	trimString(param2);
	Player* player = creature->getPlayer();
	if(player)
	{
		Player* paramPlayer = g_game.getPlayerByName(param1);
		if(paramPlayer)
		{
			if(param2[0] == 'l' || param2[0] == 'e')
				paramPlayer->addExperience(paramPlayer, Player::getExpForLevel(paramPlayer->getLevel() + 1) - paramPlayer->experience);
			else if(param2[0] == 'm')
				paramPlayer->addManaSpent(player->vocation->getReqMana(paramPlayer->getMagicLevel() + 1) - paramPlayer->manaSpent);
			else
				paramPlayer->addSkillAdvance(getSkillId(param2), paramPlayer->vocation->getReqSkillTries(getSkillId(param2), paramPlayer->getSkill(getSkillId(param2), SKILL_LEVEL) + 1));
			return true;
		}
		else
			player->sendTextMessage(MSG_STATUS_SMALL, "Couldn't find target.");
	}
	return false;
}

bool Commands::joinGuild(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(player)
	{
		if(player->guildId == 0)
		{
			trimString((std::string&)param);
			uint32_t guildId;
			if(IOGuild::getInstance()->getGuildIdByName(guildId, param))
			{
				if(player->isInvitedToGuild(guildId))
				{
					player->sendTextMessage(MSG_INFO_DESCR, "You have joined the guild.");
					IOGuild::getInstance()->joinGuild(player, guildId);
					ChatChannel* guildChannel = g_chat.getChannel(player, CHANNEL_GUILD);
					char buffer[55];
					sprintf(buffer, "%s has joined the guild.", player->name.c_str());
					if(guildChannel)
						guildChannel->talk(player, SPEAK_CHANNEL_R2, buffer);
					return true;
				}
				else
					player->sendCancel("You are not invited to that guild.");
			}
			else
				player->sendCancel("There's no guild with that name.");
		}
		else
			player->sendCancel("You are already in a guild.");
	}
	return false;
}

bool Commands::createGuild(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(player)
	{
		if(player->guildId == 0)
		{
			trimString((std::string&)param);
			if(param.length() > 3)
			{
				if(param.length() < 21)
				{
					uint32_t guildId;
					if(!IOGuild::getInstance()->getGuildIdByName(guildId, param))
					{
						if(player->level > 7)
						{
							if(player->isPremium())
							{
								char buffer[55];
								sprintf(buffer, "You have formed the guild: %s!", param.c_str());
								player->sendTextMessage(MSG_INFO_DESCR, buffer);
								player->setGuildName(param);

								IOGuild::getInstance()->createGuild(player);
								return true;
							}
							else
								player->sendCancelMessage(RET_YOUNEEDPREMIUMACCOUNT);
						}
						else
							player->sendCancel("You have to be atleast Level 8 to form a guild.");
					}
					else
						player->sendCancel("There is already a guild with that name.");
				}
				else
					player->sendCancel("That guild name is too long, please select a shorter name.");
			}
			else
				player->sendCancel("That guild name is too short, please select a longer name.");
		}
		else
			player->sendCancel("You are already in a guild.");
	}
	return false;
}

bool Commands::unban(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(!player)
		return false;

	uint32_t accountNumber = atoi(param.c_str());
	bool removedIPBan = false;
	std::string name = param;
	bool playerExists = false;
	if(IOLoginData::getInstance()->playerExists(name))
	{
		playerExists = true;
		accountNumber = IOLoginData::getInstance()->getAccountNumberByName(name);

		uint32_t lastIP = IOLoginData::getInstance()->getLastIPByName(name);
		if(lastIP != 0 && IOBan::getInstance()->isIpBanished(lastIP))
			removedIPBan = IOBan::getInstance()->removeIPBan(lastIP);
	}

	bool banned = false;
	bool deleted = false;
	uint32_t bannedBy = 0, banTime = 0;
	int32_t reason = 0, action = 0;
	std::string comment = "";
	if(IOBan::getInstance()->getBanInformation(accountNumber, bannedBy, banTime, reason, action, comment, deleted))
	{
		if(!deleted)
			banned = true;
	}

	if(banned)
	{
		if(IOBan::getInstance()->removeAccountBan(accountNumber))
		{
			std::ostringstream ss;
			ss << name << " has been unbanned.";
			player->sendTextMessage(MSG_INFO_DESCR, ss.str());
		}
	}
	else if(deleted)
	{
		if(IOBan::getInstance()->removeAccountDeletion(accountNumber))
		{
			std::ostringstream ss;
			ss << name << " has been undeleted.";
			player->sendTextMessage(MSG_INFO_DESCR, ss.str());
		}
	}
	else if(removedIPBan)
	{
		std::ostringstream ss;
		ss << "The IP banishment on " << name << " has been lifted.";
		player->sendTextMessage(MSG_INFO_DESCR, ss.str());
	}
	else
	{
		bool removedNamelock = false;
		if(playerExists)
		{
			uint32_t guid = 0;
			if(IOLoginData::getInstance()->getGuidByName(guid, name) &&
				IOBan::getInstance()->isPlayerNamelocked(name) &&
				IOBan::getInstance()->removePlayerNamelock(guid))
			{
				std::ostringstream ss;
				ss << "Namelock on " << name << " has been lifted.";
				player->sendTextMessage(MSG_INFO_DESCR, ss.str());
				removedNamelock = true;
			}
		}

		if(!removedNamelock)
			player->sendCancel("That player or account is not banished or deleted.");
	}
	return false;
}

bool Commands::playerKills(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(player)
	{
		int32_t fragTime = g_config.getNumber(ConfigManager::FRAG_TIME);
		if(player->redSkullTicks && fragTime > 0)
		{
			int32_t frags = (player->redSkullTicks / fragTime) + 1;
			int32_t remainingTime = player->redSkullTicks - (fragTime * (frags - 1));
			int32_t hours = ((remainingTime / 1000) / 60) / 60;
			int32_t minutes = ((remainingTime / 1000) / 60) - (hours * 60);

			char buffer[140];
			sprintf(buffer, "You have %d unjustified frag%s. The amount of unjustified frags will decrease after: %s.", frags, (frags > 1 ? "s" : ""), formatTime(hours, minutes).c_str());
			player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, buffer);
		}
		else
			player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "You do not have any unjustified frag.");
	}
	return false;
}

bool Commands::ghost(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(!player)
		return false;

	player->switchGhostMode();
	Player* tmpPlayer;

	SpectatorVec list;
	g_game.getSpectators(list, player->getPosition(), true);
	SpectatorVec::const_iterator it;

	Cylinder* cylinder = player->getTopParent();
	int32_t index = cylinder->__getIndexOfThing(creature);

	for(it = list.begin(); it != list.end(); ++it)
	{
		if((tmpPlayer = (*it)->getPlayer()))
		{
			tmpPlayer->sendCreatureChangeVisible(player, !player->isInGhostMode());
			if(tmpPlayer != player && !tmpPlayer->canSeeGhost(player))
			{
				if(player->isInGhostMode())
					tmpPlayer->sendCreatureDisappear(player, index, true);
				else
					tmpPlayer->sendCreatureAppear(player, true);

				tmpPlayer->sendUpdateTile(player->getTile(), player->getPosition());
			}
		}
	}

	for(it = list.begin(); it != list.end(); ++it)
		(*it)->onUpdateTile(player->getTile(), player->getPosition());

	if(player->isInGhostMode())
	{
		for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it)
		{
			if(!it->second->canSeeGhost(player))
				it->second->notifyLogOut(player);
		}

		IOLoginData::getInstance()->updateOnlineStatus(player->getGUID(), false);
		player->sendTextMessage(MSG_INFO_DESCR, "You are now invisible.");
	}
	else
	{
		for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it)
		{
			if(!it->second->canSeeGhost(player))
				it->second->notifyLogIn(player);
		}

		IOLoginData::getInstance()->updateOnlineStatus(player->getGUID(), true);
		player->sendTextMessage(MSG_INFO_DESCR, "You are visible again.");
	}
	return true;
}
