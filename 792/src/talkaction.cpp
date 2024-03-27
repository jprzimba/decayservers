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

#include "actions.h"
#include "ban.h"
#include "chat.h"
#include "creature.h"
#include "creatureevent.h"
#include "player.h"
#include "talkaction.h"
#include "tools.h"
#include "game.h"
#include "globalevent.h"
#include "configmanager.h"
#include "house.h"
#include "monster.h"
#include "npc.h"
#include "movement.h"
#include "spells.h"
#include "weapons.h"
#include "raids.h"
#include "status.h"

extern Actions* g_actions;
extern Chat g_chat;
extern ConfigManager g_config;
extern CreatureEvents* g_creatureEvents;
extern Game g_game;
extern GlobalEvents* g_globalEvents;
extern Monsters g_monsters;
extern Npcs g_npcs;
extern TalkActions* g_talkActions;
extern MoveEvents* g_moveEvents;
extern Spells* g_spells;
extern Weapons* g_weapons;

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

bool TalkActions::registerEvent(Event* event, xmlNodePtr p)
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

	std::string cmdstring[TALKFILTER_LAST] = words, paramstring[TALKFILTER_LAST] = "";
	size_t loc = words.find('"', 0);
	if(loc != std::string::npos && loc >= 0)
	{
		cmdstring[TALKFILTER_QUOTATION] = std::string(words, 0, loc);
		paramstring[TALKFILTER_QUOTATION] = std::string(words, (loc + 1), (words.size() - (loc - 1)));
		trimString(cmdstring[TALKFILTER_QUOTATION]);
	}

	loc = words.find(" ", 0);
	if(loc != std::string::npos && loc >= 0)
	{
		cmdstring[TALKFILTER_WORD] = std::string(words, 0, loc);
		paramstring[TALKFILTER_WORD] = std::string(words, (loc + 1), (words.size() - (loc - 1)));

		size_t sloc = words.find(" ", ++loc);
		if(sloc != std::string::npos && sloc >= 0)
		{
			cmdstring[TALKFILTER_WORD_SPACED] = std::string(words, 0, sloc);
			paramstring[TALKFILTER_WORD_SPACED] = std::string(words, (sloc + 1), (words.size() - (sloc - 1)));
		}
	}

	TalkActionList::iterator it;
	for(it = wordsMap.begin(); it != wordsMap.end(); ++it)
	{
		if(it->first == cmdstring[it->second->getFilter()] || !it->second->isSensitive() && !strcasecmp(it->first.c_str(), cmdstring[it->second->getFilter()].c_str()))
		{
			TalkAction* talkAction = it->second;
			if(talkAction->getAccess() > player->getAccess())
			{
				if(player->hasFlag(PlayerFlag_GamemasterPrivileges))
				{
					player->sendCancel("You are not able to execute this action.");
					g_game.addMagicEffect(player->getPosition(), NM_ME_POFF);
					return TALKACTION_FAILED;
				}
				else
					return TALKACTION_CONTINUE;
			}
			
			if(player && talkAction->isLogged())
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

			player->sendTextMessage(MSG_STATUS_CONSOLE_RED, words.c_str());
			bool ret = false;
			if(talkAction->isScripted())
				ret = talkAction->executeSay(player, cmdstring[talkAction->getFilter()], paramstring[talkAction->getFilter()]);
			else
			{
				if(TalkActionFunction* function = talkAction->getFunction())
				{
					function(player, cmdstring[talkAction->getFilter()], paramstring[talkAction->getFilter()]);
					ret = false;
				}
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
Event(_interface)
{
	m_filter = TALKFILTER_WORD;
	m_logged = false;
	m_sensitive = false;
	m_access = 0;
	m_function = NULL;
}

TalkAction::~TalkAction()
{
	//
}

bool TalkAction::configureEvent(xmlNodePtr p)
{
	std::string strValue;
	if(readXMLString(p, "words", strValue))
		m_words = strValue;
	else
	{
		std::clog << "[Error - TalkAction::configureEvent] No words for TalkAction." << std::endl;
		return false;
	}

	if(readXMLString(p, "filter", strValue))
	{
		std::string tmpStrValue = asLowerCaseString(strValue);
		if(tmpStrValue == "quotation")
			m_filter = TALKFILTER_QUOTATION;
		else if(tmpStrValue == "word")
			m_filter = TALKFILTER_WORD;
		else if(tmpStrValue == "word-spaced")
			m_filter = TALKFILTER_WORD_SPACED;
		else
			std::clog << "[Warning - TalkAction::configureEvent] Unknown filter for TalkAction: " << strValue << ", using default." << std::endl;
	}

	int32_t intValue;
	if(readXMLInteger(p, "access", intValue))
		m_access = intValue;

	if(readXMLString(p, "log", strValue) || readXMLString(p, "logged", strValue))
		m_logged = booleanString(strValue);

	if(readXMLString(p, "case-sensitive", strValue) || readXMLString(p, "casesensitive", strValue) || readXMLString(p, "sensitive", strValue))
		m_sensitive = booleanString(strValue);

	return true;	
}

bool TalkAction::loadFunction(const std::string& functionName)
{
	std::string tmpFunctionName = asLowerCaseString(functionName);
	if(tmpFunctionName == "banplayer")
		m_function = banPlayer;
	else if(tmpFunctionName == "addskill")
		m_function = addSkill;
	else if(tmpFunctionName == "createguild")
		m_function = createGuild;
	else if(tmpFunctionName == "joinguild")
		m_function = joinGuild;
	else if(tmpFunctionName == "unban")
		m_function = unBan;
	else if(tmpFunctionName == "ghost")
		m_function = ghost;
	else if(tmpFunctionName == "frags")
		m_function = frags;
	else if(tmpFunctionName == "serverinfo")
		m_function = serverInfo;
	else if(tmpFunctionName == "sellhouse")
		m_function = sellHouse;
	else if(tmpFunctionName == "buyhouse")
		m_function = buyHouse;
	else if(tmpFunctionName == "reloadinfo")
		m_function = reloadInfo;
	else if(tmpFunctionName == "getinfo")
		m_function = getInfo;
	else if(tmpFunctionName == "forceraid")
		m_function = forceRaid;
	else if(tmpFunctionName == "gethouse")
		m_function = getHouse;
	else if(tmpFunctionName == "sethouseowner")
		m_function = setHouseOwner;
	else if(tmpFunctionName == "removething")
		m_function = removeThing;
	else
	{
		std::clog << "[Warning - TalkAction::loadFunction] Function \"" << functionName << "\" does not exist." << std::endl;
		return false;
	}

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
		std::clog << "[Error] Call stack overflow. TalkAction::executeSay" << std::endl;
		return 0;
	}
}

bool TalkAction::banPlayer(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(!player)
		return false;

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

bool TalkAction::addSkill(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(!player)
		return false;

	boost::char_separator<char> sep(",");
	tokenizer cmdtokens(param, sep);
	tokenizer::iterator cmdit = cmdtokens.begin();
	std::string param1, param2;
	param1 = parseParams(cmdit, cmdtokens.end());
	param2 = parseParams(cmdit, cmdtokens.end());
	trimString(param1);
	trimString(param2);

	if(!player)
		return false;

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

	return false;
}

bool TalkAction::createGuild(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(!player)
		return false;

	if(player->guildId != 0)
	{
		player->sendCancel("You are already in a guild.");
		return false;
	}

	trimString((std::string&)param);
	if(param.length() < (uint32_t)g_config.getNumber(ConfigManager::MIN_GUILD_NAME))
	{
		std::ostringstream ss;
		ss << "That guild name is too short, it has to be at least " << g_config.getNumber(ConfigManager::MIN_GUILD_NAME) << " characters.";
		player->sendCancel(ss.str());
		return false;
	}
	else if(param.length() > (uint32_t)g_config.getNumber(ConfigManager::MAX_GUILD_NAME))
	{
		std::ostringstream ss;
		ss << "That guild name is too long, it can not be longer than " << g_config.getNumber(ConfigManager::MAX_GUILD_NAME) << " characters.";
		player->sendCancel(ss.str());
		return false;
	}

	if(!isValidName(param))
	{
		player->sendCancel("Invalid guild name format.");
		return false;
	}

	uint32_t guildId;
	if(IOGuild::getInstance()->getGuildIdByName(guildId, param))
	{
		player->sendCancel("There is already a guild with that name.");
		return false;
	}

	if(player->level < (uint32_t)g_config.getNumber(ConfigManager::LEVEL_TO_CREATE_GUILD))
	{
		std::ostringstream ss;
		ss << "You have to be atleast Level " << g_config.getNumber(ConfigManager::LEVEL_TO_CREATE_GUILD) << " to form a guild.";
		player->sendCancel(ss.str());
		return false;
	}

	if(!player->isPremium())
	{
		player->sendCancelMessage(RET_YOUNEEDPREMIUMACCOUNT);
		return false;
	}

	std::ostringstream ss;
	ss << "You have formed the guild: " << param << "!";
	player->sendTextMessage(MSG_INFO_DESCR, ss.str());
	player->setGuildName(param);

	IOGuild::getInstance()->createGuild(player);
	return false;
}

bool TalkAction::joinGuild(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(!player)
		return false;

	if(player->guildId != 0)
	{
		player->sendCancel("You are already in a guild.");
		return false;
	}

	trimString((std::string&)param);
	uint32_t guildId;
	if(!IOGuild::getInstance()->getGuildIdByName(guildId, param))
	{
		player->sendCancel("There's no guild with that name.");
		return false;
	}

	if(!player->isInvitedToGuild(guildId))
	{
		player->sendCancel("You are not invited to that guild.");
		return false;
	}

	player->sendTextMessage(MSG_INFO_DESCR, "You have joined the guild.");
	IOGuild::getInstance()->joinGuild(player, guildId);

	ChatChannel* guildChannel = g_chat.getChannel(player, CHANNEL_GUILD);
	if(guildChannel)
	{
		std::ostringstream ss;
		ss << player->getName() << " has joined the guild.";
		guildChannel->sendToAll(ss.str(), SPEAK_CHANNEL_R1);
	}

	return false;
}

bool TalkAction::unBan(Creature* creature, const std::string& cmd, const std::string& param)
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

bool TalkAction::ghost(Creature* creature, const std::string& cmd, const std::string& param)
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

	return false;
}

bool TalkAction::frags(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(!player)
		return false;

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

	return false;
}

bool TalkAction::serverInfo(Creature* creature, const std::string& cmd, const std::string& param)
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
	return false;
}

bool TalkAction::sellHouse(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(!player)
		return false;

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
	
	int16_t maxHouses = g_config.getNumber(ConfigManager::HOUSES_PER_ACCOUNT);
	if(maxHouses != -1 && player->getAccountHousesCount() >= maxHouses)
	{
		char buffer[50];
		sprintf(buffer, "You may own only %u %s per account.", maxHouses, (maxHouses == 1 ? "house" : "houses"));
		player->sendCancel(buffer);
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

	return false;
}

bool TalkAction::buyHouse(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(!player)
		return false;

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

	return false;
}

bool TalkAction::reloadInfo(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(!player)
		return false;

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

	return false;
}

bool TalkAction::getInfo(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(!player)
		return false;

	Player* paramPlayer = g_game.getPlayerByName(param);
	if(paramPlayer)
	{
		if(player != paramPlayer && paramPlayer->getAccess() >= player->getAccess())
		{
			player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "You can not get info about this player.");
			return false;
		}
		uint8_t ip[4];
		*(uint32_t*)&ip = paramPlayer->lastIP;
		std::stringstream info;
		info << "name:    " << paramPlayer->name << std::endl <<
			"access:  " << paramPlayer->getAccess() << std::endl <<
			"level:   " << paramPlayer->level << std::endl <<
			"maglvl:  " << paramPlayer->magLevel << std::endl <<
			"speed:   " << paramPlayer->getSpeed() <<std::endl <<
			"position " << paramPlayer->getPosition() << std::endl;
		player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, info.str().c_str());
	}
	else
		player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Player not found.");

	return false;
}

bool TalkAction::forceRaid(Creature* creature, const std::string& cmd, const std::string& param)
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
	return false;
}

bool TalkAction::getHouse(Creature* creature, const std::string& cmd, const std::string& param)
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

bool TalkAction::setHouseOwner(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(!player)
		return false;

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
	return false;
}

bool TalkAction::removeThing(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(!player)
		return false;

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
	return false;
}
