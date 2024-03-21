//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// IOGuild Class - saving/loading guild changes for offline players
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

#ifndef __IOGUILD__
#define __IOGUILD__

#include "player.h"

class IOGuild
{
	public:
		IOGuild() {}
		~IOGuild() {}

		static IOGuild* getInstance()
		{
			static IOGuild instance;
			return &instance;
		}

		bool createGuild(Player* player);
		bool joinGuild(Player* player, uint32_t guildId);
		bool disbandGuild(uint32_t guild_id);
		bool getGuildIdByName(uint32_t& guildId, const std::string& guildName);
		uint32_t getRankIdByGuildIdAndLevel(uint32_t guildId, uint32_t guildLevel);
		bool guildExists(uint32_t guildId);
		std::string getRankName(int16_t guildLevel, uint32_t guildId);
		bool changeRankName(std::string oldRankName, std::string newRankName, uint32_t guildId);
		bool rankNameExists(std::string rankName, uint32_t guildId);
		bool isInvitedToGuild(uint32_t guid, uint32_t guildId);
		bool invitePlayerToGuild(uint32_t guid, uint32_t guildId);
		bool revokeGuildInvite(uint32_t guid, uint32_t guildId);
		uint32_t getGuildId(uint32_t guid);
		int8_t getGuildLevel(uint32_t guid);
		bool setGuildLevel(uint32_t guid, GuildLevel_t level);
		bool setGuildNick(uint32_t guid, std::string guildNick);
		bool setMotd(uint32_t guildId, std::string newMotd);
		bool updateOwnerId(uint32_t guildId, uint32_t guid);
		std::string getMotd(uint32_t guildId);
};

#endif
