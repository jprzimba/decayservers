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

class Party
{
	public:
		Party(Player* player, Player* target);
		~Party();
		
		void invitePlayer(Player* player);
		void acceptInvitation(Player* player);
		void revokeInvitation(Player* player);
		void passLeadership(Player* player);
		bool isInvited(Player* player);
		void leave(Player* player);
		bool canOpenCorpse(uint32_t ownerId);		

		Player* getLeader() const {return leader;}
		void setLeader(Player* player) {leader = player;}

	protected:
		Player* leader;

		PlayerVector members;
		PlayerVector invitations;
};
#endif
