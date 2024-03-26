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

#ifndef __PARTY_H__
#define __PARTY_H__

#include "player.h"

typedef std::vector<Player*> PlayerVector;

class Player;
class Party;

class Party
{
	public:
		Party(Player* _leader);
		virtual ~Party();

		Player* getLeader() const {return leader;}
		void setLeader(Player* _leader) {leader = _leader;}
		PlayerVector getMembers() {return memberList;}

		void disband();
		bool invitePlayer(Player* player);
		bool joinParty(Player* player);
		void revokeInvitation(Player* player);
		bool passPartyLeadership(Player* player);
		bool leaveParty(Player* player);
		bool removeInvite(Player* player);

		bool isPlayerMember(const Player* player) const;
		bool isPlayerInvited(const Player* player) const;
		void updateAllPartyIcons();
		void updatePartyIcons(Player* player);
		void broadcastPartyMessage(MessageClasses msgClass, const std::string& msg, bool sendToInvitations = false);
		bool disbandParty() {return (memberList.empty() && inviteList.empty());}
		bool canOpenCorpse(uint32_t ownerId);

	protected:

		Player* leader;
		PlayerVector memberList;
		PlayerVector inviteList;

};

#endif
