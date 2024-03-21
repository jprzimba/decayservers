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

#ifndef __OTSERV_CREATUREEVENT_H__
#define __OTSERV_CREATUREEVENT_H__

#include "luascript.h"
#include "baseevents.h"

enum CreatureEventType_t
{
	CREATURE_EVENT_NONE,
	CREATURE_EVENT_LOGIN,
	CREATURE_EVENT_LOGOUT,
	CREATURE_EVENT_THINK,
	CREATURE_EVENT_PREPAREDEATH,
	CREATURE_EVENT_DEATH,
	CREATURE_EVENT_KILL,
	CREATURE_EVENT_ADVANCE,
	CREATURE_EVENT_LOOK
};

class CreatureEvent;

class CreatureEvents : public BaseEvents
{
	public:
		CreatureEvents();
		virtual ~CreatureEvents();

		// global events
		uint32_t playerLogin(Player* player);
		uint32_t playerLogout(Player* player);

		CreatureEvent* getEventByName(const std::string& name, bool forceLoaded = true);

	protected:
		virtual LuaScriptInterface& getScriptInterface();
		virtual std::string getScriptBaseName();
		virtual Event* getEvent(const std::string& nodeName);
		virtual bool registerEvent(Event* event, const pugi::xml_node& node);
		virtual void clear();

		//creature events
		typedef std::map<std::string, CreatureEvent*> CreatureEventList;
		CreatureEventList m_creatureEvents;

		LuaScriptInterface m_scriptInterface;
};

class CreatureEvent : public Event
{
	public:
		CreatureEvent(LuaScriptInterface* _interface);
		virtual ~CreatureEvent() {}

		virtual bool configureEvent(const pugi::xml_node& node);

		CreatureEventType_t getEventType() const {return m_type;}
		const std::string& getName() const {return m_eventName;}
		bool isLoaded() const {return m_loaded;}

		void clearEvent();
		void copyEvent(CreatureEvent* creatureEvent);

		//scripting
		uint32_t executeOnLogin(Player* player);
		uint32_t executeOnLogout(Player* player);
		uint32_t executeOnThink(Creature* creature, uint32_t interval);
		uint32_t executeOnPrepareDeath(Player* player, Creature* killer);
		uint32_t executeOnDeath(Creature* creature, Item* corpse, Creature* killer);
		uint32_t executeOnKill(Creature* creature, Creature* target);
		uint32_t executeOnAdvance(Player* player, skills_t skill, uint32_t oldLevel, uint32_t newLevel);
		uint32_t executeOnLook(Player* player, Thing* target, uint16_t itemId);
		//

	protected:
		virtual std::string getScriptEventName();

		std::string m_eventName;
		CreatureEventType_t m_type;
		bool m_loaded;
};

#endif // __OTSERV_CREATUREEVENT_H__
