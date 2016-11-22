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

#ifndef __OTSERV_NPC_H__
#define __OTSERV_NPC_H__

#include "creature.h"
#include "luascript.h"
#include "templates.h"

class Npc;
class Player;
struct NpcState;

typedef std::list<Npc*> NpcList;

class Npcs
{
	public:
		Npcs() {}
		~Npcs() {}

		void reload();
};

class NpcScriptInterface : public LuaScriptInterface
{
	public:
		NpcScriptInterface();
		virtual ~NpcScriptInterface();

		bool loadNpcLib(std::string file);

		static void pushState(lua_State *L, NpcState* state);
		static void popState(lua_State *L, NpcState* &state);

	protected:
		virtual void registerFunctions();

		static int32_t luaActionSay(lua_State* L);
		static int32_t luaActionMove(lua_State* L);
		static int32_t luaActionMoveTo(lua_State* L);
		static int32_t luaActionTurn(lua_State* L);
		static int32_t luaActionFollow(lua_State* L);
		static int32_t luaCreatureGetName(lua_State* L);
		static int32_t luaCreatureGetPos(lua_State* L);
		static int32_t luaSelfGetPos(lua_State* L);
		static int32_t luagetDistanceTo(lua_State* L);
		static int32_t luaSetNpcFocus(lua_State* L);
		static int32_t luaGetNpcCid(lua_State* L);
		static int32_t luaGetNpcPos(lua_State* L);
		static int32_t luaGetNpcState(lua_State* L);
		static int32_t luaSetNpcState(lua_State* L);
		static int32_t luaGetNpcName(lua_State* L);
		static int32_t luaGetNpcParameter(lua_State* L);

	private:
		virtual bool initState();
		virtual bool closeState();

		bool m_libLoaded;
};

class NpcEventsHandler
{
	public:
		NpcEventsHandler(Npc* npc);
		virtual ~NpcEventsHandler();

		virtual void onCreatureAppear(const Creature* creature) {}
		virtual void onCreatureDisappear(const Creature* creature) {}
		virtual void onCreatureMove(const Creature* creature, const Position& oldPos, const Position& newPos) {}
		virtual void onCreatureSay(const Creature* creature, SpeakClasses, const std::string& text) {}
		virtual void onThink() {}

		bool isLoaded();

	protected:
		Npc* m_npc;
		bool m_loaded;
};

class NpcScript : public NpcEventsHandler
{
	public:
		NpcScript(std::string file, Npc* npc);
		virtual ~NpcScript();

		virtual void onCreatureAppear(const Creature* creature);
		virtual void onCreatureDisappear(const Creature* creature);
		virtual void onCreatureMove(const Creature* creature, const Position& oldPos, const Position& newPos);
		virtual void onCreatureSay(const Creature* creature, SpeakClasses, const std::string& text);
		virtual void onThink();

	private:
		NpcScriptInterface* m_scriptInterface;

		int32_t m_onCreatureAppear;
		int32_t m_onCreatureDisappear;
		int32_t m_onCreatureMove;
		int32_t m_onCreatureSay;
		int32_t m_onThink;
};

enum RespondParam_t
{
	RESPOND_DEFAULT = 0,
	RESPOND_MALE = 1,
	RESPOND_FEMALE = 2,
	RESPOND_PZBLOCK = 4,
	RESPOND_LOWMONEY = 8,
	RESPOND_NOAMOUNT = 16,
	RESPOND_LOWAMOUNT = 32,
	RESPOND_PREMIUM = 64,
	RESPOND_DRUID = 128,
	RESPOND_KNIGHT = 256,
	RESPOND_PALADIN = 512,
	RESPOND_SORCERER = 1024,
	RESPOND_LOWLEVEL = 2048
};

enum InteractType_t
{
	INTERACT_TEXT,
	INTERACT_EVENT
};

enum ReponseActionParam_t
{
	ACTION_NONE,
	ACTION_SETTOPIC,
	ACTION_SETLEVEL,
	ACTION_SETPRICE,
	ACTION_SETBUYPRICE,
	ACTION_SETSELLPRICE,
	ACTION_TAKEMONEY,
	ACTION_GIVEMONEY,
	ACTION_SELLITEM,
	ACTION_BUYITEM,
	ACTION_GIVEITEM,
	ACTION_TAKEITEM,
	ACTION_SETAMOUNT,
	ACTION_SETITEM,
	ACTION_SETSUBTYPE,
	ACTION_SETEFFECT,
	ACTION_SETSPELL,
	ACTION_SETLISTNAME,
	ACTION_SETLISTPNAME,
	ACTION_TEACHSPELL,
	ACTION_SETSTORAGE,
	ACTION_SETTELEPORT,
	ACTION_SCRIPT,
	ACTION_SCRIPTPARAM,
	ACTION_ADDQUEUE,
	ACTION_SETIDLE
};

enum StorageComparision_t
{
	STORAGE_LESS,
	STORAGE_LESSOREQUAL,
	STORAGE_EQUAL,
	STORAGE_GREATEROREQUAL,
	STORAGE_GREATER
};

enum NpcEvent_t
{
	EVENT_NONE,
	EVENT_BUSY,
	EVENT_THINK,
	EVENT_PLAYER_ENTER,
	EVENT_PLAYER_MOVE,
	EVENT_PLAYER_LEAVE,
	/*
	EVENT_CREATURE_ENTER,
	EVENT_CREATURE_MOVE,
	EVENT_CREATURE_LEAVE,
	*/
};

struct ScriptVars
{
	ScriptVars()
	{
		n1 = -1;
		n2 = -1;
		n3 = -1;
		b1 = false;
		b2 = false;
		b3 = false;
		s1 = "";
		s2 = "";
		s3 = "";
	}

	int32_t n1;
	int32_t n2;
	int32_t n3;
	bool b1;
	bool b2;
	bool b3;
	std::string s1;
	std::string s2;
	std::string s3;
};

struct ListItem
{
	ListItem()
	{
		itemId = 0;
		subType = 1;
		sellPrice = 0;
		buyPrice = 0;
		keywords = "";
		name = "";
		pluralName = "";
	}

	int32_t sellPrice;
	int32_t buyPrice;
	int32_t itemId;
	int32_t subType;
	std::string keywords;
	std::string name;
	std::string pluralName;
};

struct NpcState
{
	int32_t topic;
	bool isIdle;
	bool isQueued;
	int32_t price;
	int32_t sellPrice;
	int32_t buyPrice;
	int32_t amount;
	int32_t itemId;
	int32_t subType;
	std::string spellName;
	std::string listName;
	std::string listPluralName;
	int32_t level;
	uint64_t prevInteraction;
	std::string respondToText;
	uint32_t respondToCreature;
	std::string prevRespondToText;

	//script variables
	ScriptVars scriptVars;

	//Do not forget to update pushState/popState if you add more variables
};

class Npc : public Creature
{
	public:
		virtual ~Npc();

		virtual Npc* getNpc() {return this;}
		virtual const Npc* getNpc() const {return this;}

		virtual bool isPushable() const {return false;}

		virtual uint32_t idRange() {return 0x80000000;}
		static AutoList<Npc> listNpc;
		void removeList() {listNpc.removeList(getID());}
		void addList() {listNpc.addList(this);}

		static Npc* createNpc(const std::string& name);

		virtual bool canSee(const Position& pos) const;

		bool load();
		void reload();

		virtual const std::string& getName() const {return name;}
		virtual const std::string& getNameDescription() const {return name;}

		void doSay(std::string msg);
		void doMove(Direction dir);
		void doTurn(Direction dir);
		void doMoveTo(Position pos);
		bool isLoaded() {return loaded;}

		void setCreatureFocus(Creature* creature);

		NpcScriptInterface* getScriptInterface();

	protected:
		Npc(const std::string& _name);

		virtual void onAddTileItem(const Tile* tile, const Position& pos, const Item* item);
		virtual void onUpdateTileItem(const Tile* tile, const Position& pos, uint32_t stackpos,
			const Item* oldItem, const ItemType& oldType, const Item* newItem, const ItemType& newType);
		virtual void onRemoveTileItem(const Tile* tile, const Position& pos, uint32_t stackpos,
			const ItemType& iType, const Item* item);
		virtual void onUpdateTile(const Tile* tile, const Position& pos);

		virtual void onCreatureAppear(const Creature* creature, bool isLogin);
		virtual void onCreatureDisappear(const Creature* creature, uint32_t stackpos, bool isLogout);
		virtual void onCreatureMove(const Creature* creature, const Tile* newTile, const Position& newPos,
			const Tile* oldTile, const Position& oldPos, uint32_t oldStackPos, bool teleport);

		virtual void onCreatureTurn(const Creature* creature, uint32_t stackpos);
		virtual void onCreatureSay(const Creature* creature, SpeakClasses type, const std::string& text);
		virtual void onCreatureChangeOutfit(const Creature* creature, const Outfit_t& outfit);
		virtual void onThink(uint32_t interval);
		virtual std::string getDescription(int32_t lookDistance) const;

		bool isImmune(CombatType_t type) const {return true;}
		bool isImmune(ConditionType_t type) const {return true;}
		virtual bool isAttackable() const { return attackable; }
		virtual bool getNextStep(Direction& dir);

		bool canWalkTo(const Position& fromPos, Direction dir);
		bool getRandomStep(Direction& dir);

		void reset();
		bool loadFromXml(const std::string& name);

		void onPlayerEnter(Player* player, NpcState* state);
		void onPlayerLeave(Player* player, NpcState* state);

		typedef std::map<std::string, std::string> ParametersMap;
		ParametersMap m_parameters;

		NpcState* getState(const Player* player, bool makeNew = true);

		std::string name;
		std::string m_filename;
		uint32_t walkTicks;
		bool floorChange;
		bool attackable;
		bool isIdle;
		int32_t talkRadius;
		int32_t idleTime;
		int32_t focusCreature;

		uint32_t npcTalkEvent;

		typedef std::map<std::string, std::list<ListItem> > ItemListMap;
		ItemListMap itemListMap;

		typedef std::list<NpcState*> StateList;
		StateList stateList;
		NpcEventsHandler* m_npcEventHandler;

		typedef std::list<uint32_t> QueueList;
		QueueList queueList;
		bool loaded;

		static NpcScriptInterface* m_scriptInterface;

		friend class Npcs;
		friend class NpcScriptInterface;
};

#endif
