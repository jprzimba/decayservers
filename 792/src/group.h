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

#ifndef __GROUP__
#define __GROUP__
#include "otsystem.h"

class Group
{
	public:
		virtual ~Group() {}

		Group()
		{
			m_name = m_fullName = "";
			m_access = m_depotLimit = m_maxVips = m_flags = 0;
		}
		Group(uint32_t id): m_id(id)
		{
			m_name = m_fullName = "";
			m_access = m_depotLimit = m_maxVips = m_flags = 0;
		}

		std::string getName() const {return m_name;}
		void setName(const std::string& v) {m_name = v;}

		std::string getFullName() const {return m_fullName;}
		void setFullName(const std::string& v) {m_fullName = v;}

		uint16_t getAccess() const {return m_access;}
		void setAccess(uint16_t v) {m_access = v;}

		uint32_t getId() const {return m_id;}
		void setId(uint32_t v) {m_id = v;}
		uint32_t getDepotLimit(bool premium = false) const;
		void setDepotLimit(uint32_t v) {m_depotLimit = v;}
		uint32_t getMaxVips(bool premium = false) const;
		void setMaxVips(uint32_t v) {m_maxVips = v;}

		uint64_t getFlags() const {return m_flags;}
		void setFlags(uint64_t v) {m_flags = v;}

		bool hasFlag(uint64_t value) const {return (m_flags & ((uint64_t)1 << value));}

	private:
		std::string m_name, m_fullName;
		uint16_t m_access;
		uint32_t m_id, m_depotLimit, m_maxVips;
		uint64_t m_flags;
};


typedef std::map<uint32_t, Group*> GroupsMap;
class Groups
{
	public:
		virtual ~Groups() {clear();}
		static Groups* getInstance()
		{
			static Groups instance;
			return &instance;
		}

		bool loadFromXml();

		void clear();
		bool reload();

		Group* getGroup(uint32_t groupId);
		int32_t getGroupId(const std::string& name);

		GroupsMap::iterator getFirstGroup() {return groupsMap.begin();}
		GroupsMap::iterator getLastGroup() {return groupsMap.end();}

	private:
		Groups() {}
		GroupsMap groupsMap;
		static Group defGroup;
};
#endif
