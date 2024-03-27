//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Party system
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

#include "party.h"
#include "player.h"
#include "game.h"

extern Game g_game;

Party::Party(Player* _leader)
{

	if(_leader)
	{
		setLeader(_leader);
		_leader->setParty(this);
		_leader->sendPlayerPartyIcons(_leader);
	}
}

Party::~Party()
{
	//
}

void Party::disband()
{
	getLeader()->sendTextMessage(MSG_INFO_DESCR, "Your party has been disbanded.");
	getLeader()->setParty(NULL);
	getLeader()->sendPlayerPartyIcons(getLeader());

	for(uint32_t i = 0; i < inviteList.size(); ++i)
	{
		inviteList[i]->removePartyInvitation(this);
		inviteList[i]->sendPlayerPartyIcons(getLeader());
		inviteList[i]->sendPlayerPartyIcons(inviteList[i]);
		getLeader()->sendPlayerPartyIcons(inviteList[i]);
	}
	inviteList.clear();

	for(uint32_t i = 0; i < memberList.size(); ++i)
	{
		memberList[i]->setParty(NULL);
		memberList[i]->sendPlayerPartyIcons(getLeader());
		memberList[i]->sendPlayerPartyIcons(memberList[i]);
		memberList[i]->sendTextMessage(MSG_INFO_DESCR, "Your party has been disbanded.");
		getLeader()->sendPlayerPartyIcons(memberList[i]);
	}
	memberList.clear();

	setLeader(NULL);
	delete this;
}

bool Party::leaveParty(Player* player)
{
	if(!player)
		return false;

	if(!isPlayerMember(player) && getLeader() != player)
		return false;

	bool hasNoLeader = false;
	if(getLeader() == player)
	{
		if(!memberList.empty())
		{
			if(memberList.size() == 1 && inviteList.empty())
				hasNoLeader = true;
			else
				passPartyLeadership(memberList.front());
		}
		else
			hasNoLeader = true;
	}

	//since we already passed the leadership, we remove the player from the list
	PlayerVector::iterator it = std::find(memberList.begin(), memberList.end(), player);
	if(it != memberList.end())
		memberList.erase(it);

	it = std::find(inviteList.begin(), inviteList.end(), player);
	if(it != inviteList.end())
		inviteList.erase(it);

	player->setParty(NULL);
	player->sendTextMessage(MSG_INFO_DESCR, "You have left the party.");
	player->sendPlayerPartyIcons(player);

	updatePartyIcons(player);

	char buffer[105];
	sprintf(buffer, "%s has left the party.", player->getName().c_str());
	broadcastPartyMessage(MSG_INFO_DESCR, buffer);

	if(hasNoLeader || disbandParty())
		disband();

	return true;
}

bool Party::passPartyLeadership(Player* player)
{
	if(!player || getLeader() == player || !isPlayerMember(player))
		return false;

	//Remove it before to broadcast the message correctly
	PlayerVector::iterator it = std::find(memberList.begin(), memberList.end(), player);
	if(it != memberList.end())
		memberList.erase(it);

	char buffer[125];
	sprintf(buffer, "%s is now the leader of the party.", player->getName().c_str());
	broadcastPartyMessage(MSG_INFO_DESCR, buffer, true);

	Player* oldLeader = getLeader();
	setLeader(player);

	memberList.insert(memberList.begin(), oldLeader);

	updatePartyIcons(oldLeader);
	updatePartyIcons(getLeader());
	updatePartyIcons(player);

	player->sendTextMessage(MSG_INFO_DESCR, "You are now the leader of the party.");
	return true;
}

bool Party::joinParty(Player* player)
{
	if(!player || player->isRemoved())
		return false;

	char buffer[120];
	sprintf(buffer, "%s has joined the party.", player->getName().c_str());
	broadcastPartyMessage(MSG_INFO_DESCR, buffer);

	memberList.push_back(player);
	player->setParty(this);

	PlayerVector::iterator it = std::find(inviteList.begin(), inviteList.end(), player);
	if(it != inviteList.end())
		inviteList.erase(it);
	player->removePartyInvitation(this);

	updatePartyIcons(player);

	sprintf(buffer, "You have joined %s'%s party.", leader->getName().c_str(), (leader->getName()[leader->getName().length() - 1] == 's' ? "" : "s"));
	player->sendTextMessage(MSG_INFO_DESCR, buffer);
	return true;
}

bool Party::removeInvite(Player* player)
{
	if(!player || player->isRemoved() || !isPlayerInvited(player))
		return false;

	PlayerVector::iterator it = std::find(inviteList.begin(), inviteList.end(), player);
	if(it != inviteList.end())
		inviteList.erase(it);
	player->removePartyInvitation(this);

	getLeader()->sendPlayerPartyIcons(player);
	player->sendPlayerPartyIcons(getLeader());

	if(disbandParty())
		disband();

	return true;
}

void Party::revokeInvitation(Player* player)
{
	char buffer[100];
	sprintf(buffer, "%s has revoked %s invitation.", leader->getName().c_str(), (leader->getSex() == PLAYERSEX_FEMALE ? "her" : "his"));
	player->sendTextMessage(MSG_INFO_DESCR, buffer);

	sprintf(buffer, "Invitation for %s has been revoked.", player->getName().c_str());
	getLeader()->sendTextMessage(MSG_INFO_DESCR, buffer);

	removeInvite(player);
}

bool Party::invitePlayer(Player* player)
{
	if(!player || player->isRemoved())
		return false;

	if(std::find(inviteList.begin(), inviteList.end(), player) != inviteList.end())
		return false;

	inviteList.push_back(player);
	getLeader()->sendPlayerPartyIcons(player);
	player->sendPlayerPartyIcons(getLeader());
	player->addPartyInvitation(this);

	char buffer[105];
	sprintf(buffer, "%s has been invited.", player->getName().c_str());
	leader->sendTextMessage(MSG_INFO_DESCR, buffer);

	sprintf(buffer, "%s has invited you to %s party.", leader->getName().c_str(), (leader->getSex() == PLAYERSEX_FEMALE ? "her" : "his"));
	player->sendTextMessage(MSG_INFO_DESCR, buffer);
	return true;
}

bool Party::isPlayerMember(const Player* player) const
{
	PlayerVector::const_iterator it = std::find(memberList.begin(), memberList.end(), player);
	return it != memberList.end();
}

bool Party::isPlayerInvited(const Player* player) const
{
	PlayerVector::const_iterator it = std::find(inviteList.begin(), inviteList.end(), player);
	return it != inviteList.end();
}

void Party::updatePartyIcons(Player* player)
{
	for(PlayerVector::iterator it = memberList.begin(); it != memberList.end(); ++it)
	{
		(*it)->sendPlayerPartyIcons(player);
		player->sendPlayerPartyIcons((*it));
	}

	for(PlayerVector::iterator it = inviteList.begin(); it != inviteList.end(); ++it)
	{
		(*it)->sendPlayerPartyIcons(player);
		player->sendPlayerPartyIcons((*it));
	}

	getLeader()->sendPlayerPartyIcons(player);
	player->sendPlayerPartyIcons(getLeader());
}

void Party::updateAllPartyIcons()
{
	for(PlayerVector::iterator it = memberList.begin(); it != memberList.end(); ++it)
	{
		for(PlayerVector::iterator it2 = memberList.begin(); it2 != memberList.end(); ++it2)
			(*it)->sendPlayerPartyIcons((*it2));

		(*it)->sendPlayerPartyIcons(getLeader());
		getLeader()->sendPlayerPartyIcons((*it));
	}

	for(PlayerVector::iterator it = inviteList.begin(); it != inviteList.end(); ++it)
		(*it)->sendPlayerPartyIcons(getLeader());

	getLeader()->sendPlayerPartyIcons(getLeader());
}

void Party::broadcastPartyMessage(MessageClasses msgClass, const std::string& msg, bool sendToInvitations /*= false*/)
{
	PlayerVector::iterator it;
	if(!memberList.empty())
	{
		for(it = memberList.begin(); it != memberList.end(); ++it)
			(*it)->sendTextMessage(msgClass, msg);
	}

	getLeader()->sendTextMessage(msgClass, msg);
	if(sendToInvitations && !inviteList.empty())
	{
		for(it = inviteList.begin(); it != inviteList.end(); ++it)
			(*it)->sendTextMessage(msgClass, msg);
	}
}

bool Party::canOpenCorpse(uint32_t ownerId)
{
	if(Player* player = g_game.getPlayerByID(ownerId))
		return leader->getID() == ownerId || isPlayerMember(player);

	return false;
}
