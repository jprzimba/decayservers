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

#include "party.h"
#include "game.h"

extern Game g_game;

Party::Party(Player* player, Player* target)
{
	leader = player;
	leader->setParty(this);
	leader->sendCreatureShield(leader);
	leader->sendCreatureSkull(leader);
	invitePlayer(target);
}

Party::~Party()
{
	PlayerVector tmp_members = members;
	PlayerVector tmp_invitations = invitations;
	Player* tmp_leader = leader;
	
	invitations.clear();
	leader->sendTextMessage(MSG_INFO_DESCR, "Your party has been disbanded.");
	leader->setParty(NULL);
	leader = NULL;
	while(!members.empty())
	{
		members.back()->sendTextMessage(MSG_INFO_DESCR, "Your party has been disbanded.");
		members.back()->setParty(NULL);
		members.pop_back();
	}

	for(uint32_t i = 0; i < tmp_members.size(); i++)
	{
		for(uint32_t j = 0; j < tmp_members.size(); j++)
		{
			tmp_members[i]->sendCreatureShield(tmp_members[j]);
			tmp_members[i]->sendCreatureSkull(tmp_members[j]);
		}
		tmp_members[i]->sendCreatureShield(tmp_leader);
		tmp_members[i]->sendCreatureSkull(tmp_leader);
		tmp_leader->sendCreatureShield(tmp_members[i]);
		tmp_leader->sendCreatureSkull(tmp_members[i]);
	}
	
	for(uint32_t i = 0; i < tmp_invitations.size(); i++)
	{
		tmp_invitations[i]->sendCreatureShield(tmp_leader);
		tmp_leader->sendCreatureShield(tmp_invitations[i]);
	}
	
	tmp_leader->sendCreatureShield(tmp_leader);
	tmp_leader->sendCreatureSkull(tmp_leader);
}

void Party::leave(Player* player)
{
	if(members.size() > 0)
	{
		if(leader == player)
			passLeadership(members.front());
		PlayerVector::iterator target = std::find(members.begin(), members.end(), player);
		if(target == members.end())
			return;
		members.erase(target);
		player->setParty(NULL);
	}
	
	char buffer[55];
	sprintf(buffer, "%s has left the party.", player->getName().c_str());
	for(uint32_t i = 0; i < members.size(); i++)
	{
		members[i]->sendTextMessage(MSG_INFO_DESCR, buffer);
		members[i]->sendCreatureShield(player);
		members[i]->sendCreatureSkull(player);
		player->sendCreatureShield(members[i]);
		player->sendCreatureSkull(members[i]);
	}
	leader->sendTextMessage(MSG_INFO_DESCR, buffer);
	leader->sendCreatureShield(player);
	leader->sendCreatureSkull(player);
	if(leader != player)
	{
		player->sendCreatureShield(leader);
		player->sendCreatureSkull(leader);
	}
	player->sendCreatureSkull(player);
	player->sendCreatureShield(player);
	player->sendTextMessage(MSG_INFO_DESCR, "You have left the party.");

	if(members.empty() && (leader == player || invitations.empty()))
		delete this;
}

void Party::passLeadership(Player* player)
{
	members.insert(members.begin(), leader);
	Player* oldLeader = leader;
	leader = player;
	PlayerVector::iterator target = std::find(members.begin(), members.end(), leader);
	members.erase(target);

	char buffer[100];
	sprintf(buffer, "%s is now the leader of the party.", leader->getName().c_str());
	for(uint32_t i = 0; i < members.size(); i++)
	{
		members[i]->sendTextMessage(MSG_INFO_DESCR, buffer);
		members[i]->sendCreatureShield(leader);
		members[i]->sendCreatureShield(oldLeader);
	}
	leader->sendCreatureShield(leader);
	leader->sendCreatureShield(members.front());
	members.front()->sendCreatureShield(members.front());
	
	for(uint32_t i = 0; i < invitations.size(); i++)
	{
		members.front()->sendCreatureShield(invitations[i]);
		invitations[i]->sendCreatureShield(members.front());
	}
	invitations.clear();
	leader->sendTextMessage(MSG_INFO_DESCR, "You are now the leader of the party.");
}

void Party::acceptInvitation(Player* player)
{
	PlayerVector::iterator target = std::find(invitations.begin(), invitations.end(), player);
	if(target == invitations.end())
		return;

	char buffer[95];
	sprintf(buffer, "%s has joined the party.", player->getName().c_str());
	player->setParty(this);
	invitations.erase(target);
	for(uint32_t i = 0; i < members.size(); i++)
	{
		members[i]->sendTextMessage(MSG_INFO_DESCR, buffer);
		members[i]->sendCreatureSkull(player);
		members[i]->sendCreatureShield(player);
		
		player->sendCreatureSkull(members[i]);
		player->sendCreatureShield(members[i]);
	}
	leader->sendTextMessage(MSG_INFO_DESCR, buffer);
	sprintf(buffer, "You have joined %s'%s party.", leader->getName().c_str(), (leader->getName()[leader->getName().length() - 1] == 's' ? "" : "s"));
	player->sendTextMessage(MSG_INFO_DESCR, buffer);
	members.push_back(player);
	player->sendCreatureSkull(player);
	player->sendCreatureShield(player);
	leader->sendCreatureSkull(player);
	leader->sendCreatureShield(player);
	player->sendCreatureSkull(leader);
	player->sendCreatureShield(leader);
}

void Party::revokeInvitation(Player* player)
{
	PlayerVector::iterator target = std::find(invitations.begin(), invitations.end(), player);
	if(target == invitations.end())
		return;

	char buffer[85];
	if(!player->getParty())
	{
		sprintf(buffer, "%s has revoked %s invitation.", leader->getName().c_str(), (leader->getSex() == PLAYERSEX_FEMALE ? "her" : "his"));
		player->sendTextMessage(MSG_INFO_DESCR, buffer);
	}
	sprintf(buffer, "Invitation for %s has been revoked.", player->getName().c_str());
	leader->sendTextMessage(MSG_INFO_DESCR, buffer);

	invitations.erase(target);

	player->sendCreatureShield(leader);
	leader->sendCreatureShield(player);

	if(invitations.empty() && members.empty())
		delete this;
}

void Party::invitePlayer(Player* player)
{
	char buffer[90];
	sprintf(buffer, "%s has been invited.", player->getName().c_str());
	leader->sendTextMessage(MSG_INFO_DESCR, buffer);
	sprintf(buffer, "%s has invited you to %s party.", leader->getName().c_str(), (leader->getSex() == PLAYERSEX_FEMALE ? "her" : "his"));
	player->sendTextMessage(MSG_INFO_DESCR, buffer);
	
	invitations.push_back(player);

	player->sendCreatureShield(leader);
	leader->sendCreatureShield(player);
}

bool Party::isInvited(Player* player)
{
	return std::find(invitations.begin(), invitations.end(), player) != invitations.end();
}

bool Party::canOpenCorpse(uint32_t ownerId)
{
	Player* player = g_game.getPlayerByID(ownerId);
	if(!player)
		return false;

	PlayerVector::iterator it = std::find(members.begin(), members.end(), player);
	if(it != members.end() || leader->getID() == ownerId)
		return true;

	return false;
}
