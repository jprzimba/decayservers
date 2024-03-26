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

#ifndef __CHAT__
#define __CHAT__
#include "otsystem.h"

#include <fstream>
#include "const.h"
#include "party.h"

class Player;
enum ChannelFlags_t
{
	CHANNELFLAG_NONE = 0,
	CHANNELFLAG_ENABLED = 1 << 0,
	CHANNELFLAG_ACTIVE = 1 << 1,
	CHANNELFLAG_LOGGED = 1 << 2,
};

typedef std::map<uint32_t, Player*> UsersMap;
typedef std::list<uint32_t> InviteList;

class ChatChannel
{
	public:
		ChatChannel(uint16_t id, const std::string& name, uint16_t flags, uint32_t access = 0,
			uint32_t level = 1, Condition* condition = NULL, int32_t conditionId = -1,
			const std::string& conditionMessage = "", VocationMap* vocationMap = NULL);
		virtual ~ChatChannel()
		{
			if(m_condition)
				delete m_condition;

			if(m_vocationMap)
				delete m_vocationMap;
		}
		static uint16_t staticFlags;

		uint16_t getId() const {return m_id;}
		const std::string& getName() const {return m_name;}
		uint16_t getFlags() const {return m_flags;}

		int32_t getConditionId() const {return m_conditionId;}
		const std::string& getConditionMessage() const {return m_conditionMessage;}
		const UsersMap& getUsers() {return m_users;}

		uint32_t getLevel() const {return m_level;}
		uint32_t getAccess() const {return m_access;}
		virtual const uint32_t getOwner() {return 0;}

		bool hasFlag(uint16_t value) const {return ((m_flags & (uint16_t)value) == (uint16_t)value);}
		bool checkVocation(uint32_t vocationId) const
			{return !m_vocationMap || m_vocationMap->empty() || m_vocationMap->find(
				vocationId) != m_vocationMap->end();}

		bool addUser(Player* player);
		bool removeUser(Player* player);

		bool talk(Player* player, SpeakClasses type, const std::string& text, uint32_t _time = 0);

	protected:
		uint16_t m_id, m_flags;
		int32_t m_conditionId;
		uint32_t m_access, m_level;
		std::string m_name, m_conditionMessage;

		Condition* m_condition;
		VocationMap* m_vocationMap;

		UsersMap m_users;
		boost::shared_ptr<std::ofstream> m_file;
};

class PrivateChatChannel : public ChatChannel
{
	public:
		PrivateChatChannel(uint16_t id, std::string name, uint16_t flags);
		virtual ~PrivateChatChannel() {}

		virtual const uint32_t getOwner() {return m_owner;}
		void setOwner(uint32_t id) {m_owner = id;}

		bool isInvited(const Player* player);

		void invitePlayer(Player* player, Player* invitePlayer);
		void excludePlayer(Player* player, Player* excludePlayer);

		bool addInvited(Player* player);
		bool removeInvited(Player* player);

		void closeChannel();

	protected:
		InviteList m_invites;
		uint32_t m_owner;
};

typedef std::list<ChatChannel*> ChannelList;
typedef std::map<uint32_t, std::string> StatementMap;

class Chat
{
	public:
		Chat(): statement(0), dummyPrivate(NULL), partyName("Party"), lootName("Loot") {}
		virtual ~Chat();

		bool reload();
		bool loadFromXml();
		bool parseChannelNode(xmlNodePtr p);

		ChatChannel* createChannel(Player* player, uint16_t channelId);
		bool deleteChannel(Player* player, uint16_t channelId);

		ChatChannel* addUserToChannel(Player* player, uint16_t channelId);
		bool removeUserFromChannel(Player* player, uint16_t channelId);
		void removeUserFromAllChannels(Player* player);

		bool talkToChannel(Player* player, SpeakClasses type, const std::string& text, uint16_t channelId);

		ChatChannel* getChannel(Player* player, uint16_t channelId);
		ChatChannel* getChannelById(uint16_t channelId);

		std::string getChannelName(Player* player, uint16_t channelId);
		ChannelList getChannelList(Player* player);

		PrivateChatChannel* getPrivateChannel(Player* player);
		bool isPrivateChannel(uint16_t channelId) const {return m_privateChannels.find(channelId) != m_privateChannels.end();}

		uint32_t statement;
		StatementMap statementMap;

	private:
		void clear();

		typedef std::map<uint16_t, ChatChannel*> NormalChannelMap;
		NormalChannelMap m_normalChannels;

		typedef std::map<uint16_t, PrivateChatChannel*> PrivateChannelMap;
		PrivateChannelMap m_privateChannels;

		typedef std::map<Party*, ChatChannel*> PartyChannelMap;
		PartyChannelMap m_partyChannels;

		typedef std::map<uint32_t, ChatChannel*> GuildChannelMap;
		GuildChannelMap m_guildChannels;

		typedef std::map<uint32_t, ChatChannel*> LootChannelMap;
		LootChannelMap m_lootChannels;

		ChatChannel* dummyPrivate;
		std::string partyName, lootName;
};
#endif
