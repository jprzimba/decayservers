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
#include "party.h"

#include "player.h"
#include "chat.h"
#include "game.h"
#include "configmanager.h"

extern Game g_game;
extern Chat g_chat;
extern ConfigManager g_config;

Party::Party(Player* _leader)
{
	if(_leader)
	{
		leader = _leader;
		leader->setParty(this);
		leader->sendPlayerPartyIcons(leader);
	}
}

void Party::disband()
{
	leader->sendClosePrivate(CHANNEL_PARTY);
	leader->setParty(NULL);
	leader->sendTextMessage(MSG_INFO_DESCR, "Your party has been disbanded.");

	leader->sendPlayerPartyIcons(leader);
	for(PlayerVector::iterator it = inviteList.begin(); it != inviteList.end(); ++it)
	{
		(*it)->removePartyInvitation(this);
		(*it)->sendPlayerPartyIcons(leader);
		(*it)->sendPlayerPartyIcons(*it);
		leader->sendPlayerPartyIcons(*it);
	}

	inviteList.clear();
	for(PlayerVector::iterator it = memberList.begin(); it != memberList.end(); ++it)
	{
		(*it)->sendClosePrivate(CHANNEL_PARTY);
		(*it)->setParty(NULL);
		(*it)->sendTextMessage(MSG_INFO_DESCR, "Your party has been disbanded.");

		(*it)->sendPlayerPartyIcons(*it);
		(*it)->sendPlayerPartyIcons(leader);
		leader->sendPlayerPartyIcons(*it);
	}

	memberList.clear();
	leader = NULL;
	delete this;
}

bool Party::leave(Player* player)
{
	if(!isPlayerMember(player) && player != leader)
		return false;

	bool missingLeader = false;
	if(leader == player)
	{
		if(!memberList.empty())
		{
			if(memberList.size() == 1 && inviteList.empty())
				missingLeader = true;
			else
				passLeadership(memberList.front());
		}
		else
			missingLeader = true;
	}

	//since we already passed the leadership, we remove the player from the list
	PlayerVector::iterator it = std::find(memberList.begin(), memberList.end(), player);
	if(it != memberList.end())
		memberList.erase(it);

	it = std::find(inviteList.begin(), inviteList.end(), player);
	if(it != inviteList.end())
		inviteList.erase(it);

	player->setParty(NULL);
	player->sendClosePrivate(CHANNEL_PARTY);

	player->sendTextMessage(MSG_INFO_DESCR, "You have left the party.");
	player->sendPlayerPartyIcons(player);

	updateIcons(player);
	clearPlayerPoints(player);

	char buffer[105];
	sprintf(buffer, "%s has left the party.", player->getName().c_str());

	broadcastMessage(MSG_INFO_DESCR, buffer);
	if(missingLeader || canDisband())
		disband();

	return true;
}

bool Party::passLeadership(Player* player)
{
	if(!isPlayerMember(player) || player == leader)
		return false;

	//Remove it before to broadcast the message correctly
	PlayerVector::iterator it = std::find(memberList.begin(), memberList.end(), player);
	if(it != memberList.end())
		memberList.erase(it);

	Player* oldLeader = leader;
	leader = player;
	memberList.insert(memberList.begin(), oldLeader);

	char buffer[125];
	sprintf(buffer, "%s is now the leader of the party.", player->getName().c_str());
	broadcastMessage(MSG_INFO_DESCR, buffer, true);

	player->sendTextMessage(MSG_INFO_DESCR, "You are now the leader of the party.");

	updateIcons(oldLeader);
	updateIcons(player);
	return true;
}

bool Party::join(Player* player)
{
	if(isPlayerMember(player) || !isPlayerInvited(player))
		return false;

	memberList.push_back(player);
	player->setParty(this);

	player->removePartyInvitation(this);
	PlayerVector::iterator it = std::find(inviteList.begin(), inviteList.end(), player);
	if(it != inviteList.end())
		inviteList.erase(it);

	char buffer[200];
	sprintf(buffer, "%s has joined the party.", player->getName().c_str());
	broadcastMessage(MSG_INFO_DESCR, buffer);

	sprintf(buffer, "You have joined %s'%s party. Open the party channel to communicate with your companions.", leader->getName().c_str(), (leader->getName()[leader->getName().length() - 1] == 's' ? "" : "s"));
	player->sendTextMessage(MSG_INFO_DESCR, buffer);

	updateIcons(player);
	return true;
}

bool Party::removeInvite(Player* player)
{
	if(!isPlayerInvited(player))
		return false;

	PlayerVector::iterator it = std::find(inviteList.begin(), inviteList.end(), player);
	if(it != inviteList.end())
		inviteList.erase(it);

	leader->sendPlayerPartyIcons(player);
	player->sendPlayerPartyIcons(leader);

	player->removePartyInvitation(this);
	if(canDisband())
		disband();

	return true;
}

void Party::revokeInvitation(Player* player)
{
	if(!player || player->isRemoved())
		return;

	char buffer[150];
	sprintf(buffer, "%s has revoked %s invitation.", leader->getName().c_str(), (leader->getSex(false) ? "his" : "her"));
	player->sendTextMessage(MSG_INFO_DESCR, buffer);

	sprintf(buffer, "Invitation for %s has been revoked.", player->getName().c_str());
	leader->sendTextMessage(MSG_INFO_DESCR, buffer);
	removeInvite(player);
}

bool Party::invitePlayer(Player* player)
{
	if(isPlayerInvited(player, true))
		return false;

	inviteList.push_back(player);
	player->addPartyInvitation(this);

	char buffer[150];
	sprintf(buffer, "%s has been invited.%s", player->getName().c_str(), (!memberList.size() ? " Open the party channel to communicate with your members." : ""));
	leader->sendTextMessage(MSG_INFO_DESCR, buffer);

	sprintf(buffer, "%s has invited you to %s party.", leader->getName().c_str(), (leader->getSex(false) ? "his" : "her"));
	player->sendTextMessage(MSG_INFO_DESCR, buffer);

	leader->sendPlayerPartyIcons(player);
	player->sendPlayerPartyIcons(leader);
	return true;
}

void Party::updateIcons(Player* player)
{
	if(!player || player->isRemoved())
		return;

	PlayerVector::iterator it;
	for(it = memberList.begin(); it != memberList.end(); ++it)
	{
		(*it)->sendPlayerPartyIcons(player);
		player->sendPlayerPartyIcons((*it));
	}

	for(it = inviteList.begin(); it != inviteList.end(); ++it)
	{
		(*it)->sendPlayerPartyIcons(player);
		player->sendPlayerPartyIcons((*it));
	}

	leader->sendPlayerPartyIcons(player);
	player->sendPlayerPartyIcons(leader);
}

void Party::updateAllIcons()
{
	PlayerVector::iterator it;
	for(it = memberList.begin(); it != memberList.end(); ++it)
	{
		for(PlayerVector::iterator iit = memberList.begin(); iit != memberList.end(); ++iit)
			(*it)->sendPlayerPartyIcons((*iit));

		(*it)->sendPlayerPartyIcons(leader);
		leader->sendPlayerPartyIcons((*it));
	}

	leader->sendPlayerPartyIcons(leader);
	for(it = inviteList.begin(); it != inviteList.end(); ++it)
		(*it)->sendPlayerPartyIcons(leader);
}

void Party::broadcastMessage(MessageClasses messageClass, const std::string& text, bool sendToInvitations/* = false*/)
{
	PlayerVector::iterator it;
	if(!memberList.empty())
	{
		for(it = memberList.begin(); it != memberList.end(); ++it)
			(*it)->sendTextMessage(messageClass, text);
	}

	leader->sendTextMessage(messageClass, text);
	if(!sendToInvitations || inviteList.empty())
		return;

	for(it = inviteList.begin(); it != inviteList.end(); ++it)
		(*it)->sendTextMessage(messageClass, text);
}

void Party::addPlayerHealedMember(Player* player, uint32_t points)
{
	if(points <= 0)
		return;

	CountMap::iterator it = pointMap.find(player->getID());
	if(it != pointMap.end())
	{
		it->second.totalHeal += points;
		it->second.ticks = OTSYS_TIME();
	}
	else
		pointMap[player->getID()] = CountBlock_t(points, 0);
}

void Party::addPlayerDamageMonster(Player* player, uint32_t points)
{
	if(points <= 0)
		return;

	CountMap::iterator it = pointMap.find(player->getID());
	if(it != pointMap.end())
	{
		it->second.totalDamage += points;
		it->second.ticks = OTSYS_TIME();
	}
	else
		pointMap[player->getID()] = CountBlock_t(0, points);
}

void Party::clearPlayerPoints(Player* player)
{
	CountMap::iterator it = pointMap.find(player->getID());
	if(it == pointMap.end())
		return;

	pointMap.erase(it);
}

bool Party::isPlayerMember(const Player* player, bool result/* = false*/) const
{
	if(!player || player->isRemoved())
		return result;

	return std::find(memberList.begin(), memberList.end(), player) != memberList.end();
}

bool Party::isPlayerInvited(const Player* player, bool result/* = false*/) const
{
	if(!player || player->isRemoved())
		return result;

	return std::find(inviteList.begin(), inviteList.end(), player) != inviteList.end();
}

bool Party::canOpenCorpse(uint32_t ownerId)
{
	return leader->getID() == ownerId || isPlayerMember(g_game.getPlayerByID(ownerId));
}
