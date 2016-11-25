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

#include "ban.h"
#include "chat.h"
#include "creature.h"
#include "player.h"
#include "talkaction.h"
#include "tools.h"
#include "game.h"
#include "configmanager.h"
#include "house.h"

extern ConfigManager g_config;
extern Chat g_chat;
extern Game g_game;

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

	return nullptr;
}

bool TalkActions::registerEvent(Event* event, const pugi::xml_node& node)
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
			if(talkAction->getAccess() > player->getAccessLevel())
			{
				if(player->getAccessLevel() > 0)
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
	m_function = nullptr;
}

TalkAction::~TalkAction()
{
	//
}

bool TalkAction::configureEvent(const pugi::xml_node& node)
{
	pugi::xml_attribute attr;
	if(attr = node.attribute("words"))
		m_words = attr.as_string();
	else
	{
		std::cout << "[Error - TalkAction::configureEvent] No words for TalkAction." << std::endl;
		return false;
	}

	if((attr = node.attribute("access")))
		m_access = pugi::cast<int32_t>(attr.value());

	if((attr = node.attribute("filter")))
	{
		std::string tmpStrValue = asLowerCaseString(attr.as_string());
		if(tmpStrValue == "quotation")
			m_filter = TALKFILTER_QUOTATION;
		else if(tmpStrValue == "word")
			m_filter = TALKFILTER_WORD;
		else if(tmpStrValue == "word-spaced")
			m_filter = TALKFILTER_WORD_SPACED;
		else
			std::cout << "[Warning - TalkAction::configureEvent] Unknown filter for TalkAction: " << tmpStrValue << ", using default." << std::endl;
	}
	
	if((attr = node.attribute("registerlog")) || (attr = node.attribute("log")) || (attr = node.attribute("logged")))
		m_logged = booleanString(attr.as_string());

	if((attr = node.attribute("case-sensitive")) || (attr = node.attribute("sensitive")))
		m_sensitive = booleanString(attr.as_string());
	
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
	else
	{
		std::cout << "[Warning - TalkAction::loadFunction] Function \"" << functionName << "\" does not exist." << std::endl;
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
	
		#ifdef __DEBUG_LUASCRIPTS__
		std::stringstream desc;
		desc << creature->getName() << " - " << words << " " << param;
		env->setEventDesc(desc.str());
		#endif
	
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
		std::cout << "[Error] Call stack overflow. TalkAction::executeSay" << std::endl;
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
			IOBan::getInstance()->addIpBan(ip, (time(nullptr) + 86400), 0);

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
