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

#ifndef __OTSERV_ACTIONS_H__
#define __OTSERV_ACTIONS_H__

#include "position.h"

#include <map>
#include "luascript.h"
#include "baseevents.h"
#include "thing.h"

class Action;
class Container;

class Actions : public BaseEvents
{
	public:
		Actions();
		virtual ~Actions();
	
		bool useItem(Player* player, const Position& pos, uint8_t index, Item* item, bool isHotkey);
		bool useItemEx(Player* player, const Position& fromPos, const Position& toPos,
			uint8_t toStackPos, Item* item, bool isHotkey, uint32_t creatureId = 0);

		bool openContainer(Player* player,Container* container, const uint8_t index);

		ReturnValue canUse(const Player* player, const Position& pos);
		ReturnValue canUse(const Player* player, const Position& pos, const Item* item);
		ReturnValue canUseFar(const Creature* creature, const Position& toPos, bool checkLineOfSight);

	protected:
		ReturnValue internalUseItem(Player* player, const Position& pos,
			uint8_t index, Item* item, uint32_t creatureId);
		void showUseHotkeyMessage(Player* player, int32_t id, uint32_t count);

		virtual void clear();
		virtual LuaScriptInterface& getScriptInterface();
		virtual std::string getScriptBaseName();
		virtual Event* getEvent(const std::string& nodeName);
		virtual bool registerEvent(Event* event, const pugi::xml_node& node);

		void registerItemID(int32_t itemId, Event* event);
		void registerActionID(int32_t actionId, Event* event);
		void registerUniqueID(int32_t uniqueId, Event* event);

		typedef std::map<unsigned short, Action*> ActionUseMap;
		ActionUseMap useItemMap;
		ActionUseMap uniqueItemMap;
		ActionUseMap actionItemMap;

		Action* getAction(const Item* item);
		void clearMap(ActionUseMap& map);

		LuaScriptInterface m_scriptInterface;
};

typedef bool (ActionFunction)(Player* player, Item* item, const PositionEx& posFrom, const PositionEx& posTo, bool extendedUse, uint32_t creatureId);

class Action : public Event
{
	public:
		Action(const Action* copy);
		Action(LuaScriptInterface* _interface);
		virtual ~Action();

		virtual bool configureEvent(const pugi::xml_node& node);
		virtual bool loadFunction(const std::string& functionName);

		//scripting
		virtual bool executeUse(Player* player, Item* item, const PositionEx& posFrom,
			const PositionEx& posTo, bool extendedUse, uint32_t creatureId);
		//

		bool getAllowFarUse() const {return allowFarUse;}
		void setAllowFarUse(bool v){allowFarUse = v;}

		bool getCheckLineOfSight() const {return checkLineOfSight;}
		void setCheckLineOfSight(bool v){checkLineOfSight = v;}

		virtual ReturnValue canExecuteAction(const Player* player, const Position& toPos);
		virtual bool hasOwnErrorHandler() {return false;}

		ActionFunction* function;

	protected:
		virtual std::string getScriptEventName();
	
		static ActionFunction highscoreBook;
		static ActionFunction increaseItemId;
		static ActionFunction decreaseItemId;
	
		bool allowFarUse;
		bool checkLineOfSight;
};

#endif
