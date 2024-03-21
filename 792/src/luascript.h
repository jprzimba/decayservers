//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Lua script interface
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

#ifndef __OTSERV_LUASCRIPT_H__
#define __OTSERV_LUASCRIPT_H__

#include <string>
#include <map>
#include <list>

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}

#include "position.h"
#include "definitions.h"
#include "database.h"

class Thing;
class Creature;
class Player;
class Item;
class Container;
class AreaCombat;
class Combat;
class Condition;
class Npc;

enum LuaVariantType_t
{
	VARIANT_NONE = 0,
	VARIANT_NUMBER,
	VARIANT_POSITION,
	VARIANT_TARGETPOSITION,
	VARIANT_STRING,
};

struct LuaVariant
{
	LuaVariant()
	{
		type = VARIANT_NONE;
		text = "";
		pos.x = 0;
		pos.y = 0;
		pos.z = 0;
		pos.stackpos = 0;
		number = 0;
	}

	LuaVariantType_t type;
	std::string text;
	PositionEx pos;
	uint32_t number;
};

class LuaScriptInterface;
class Game;
class Npc;

class ScriptEnvironment
{
	public:
		ScriptEnvironment();
		~ScriptEnvironment();

		void resetEnv();
		void resetCallback() {m_callbackId = 0;}

		static bool saveGameState();
		static bool loadGameState();

		void setScriptId(int32_t scriptId, LuaScriptInterface* scriptInterface) {
			m_scriptId = scriptId;
			m_interface = scriptInterface;
		}
		bool setCallbackId(int32_t callbackId, LuaScriptInterface* scriptInterface);
		void setEventDesc(const std::string& desc) {m_eventdesc = desc;}

		std::string getEventDesc() {return m_eventdesc;}
		int32_t getScriptId() {return m_scriptId;}
		int32_t getCallbackId() {return m_callbackId;}
		LuaScriptInterface* getScriptInterface() {return m_interface;}

		void setTimerEvent() {m_timerEvent = true;}
		void resetTimerEvent() {m_timerEvent = false;}

		void getEventInfo(int32_t& scriptId, std::string& desc, LuaScriptInterface*& scriptInterface, int32_t& callbackId, bool& timerEvent);

		static void addTempItem(ScriptEnvironment* env, Item* item);
		static void removeTempItem(ScriptEnvironment* env, Item* item);
		static void removeTempItem(Item* item);
		static void addUniqueThing(Thing* thing);
		static void removeUniqueThing(Thing* thing);
		uint32_t addThing(Thing* thing);
		void insertThing(uint32_t uid, Thing* thing);

		DBResult* getResultByID(uint32_t id);
		uint32_t addResult(DBResult* res);
		bool removeResult(uint32_t id);

		void addGlobalStorageValue(const uint32_t key, const int32_t value);
		bool getGlobalStorageValue(const uint32_t key, int32_t& value) const;

		void setRealPos(const Position& realPos) {m_realPos = realPos;}
		Position getRealPos() {return m_realPos;}

		void setNpc(Npc* npc) {m_curNpc = npc;}
		Npc* getNpc() const {return m_curNpc;}

		Thing* getThingByUID(uint32_t uid);
		Item* getItemByUID(uint32_t uid);
		Container* getContainerByUID(uint32_t uid);
		Creature* getCreatureByUID(uint32_t uid);
		Player* getPlayerByUID(uint32_t uid);
		void removeItemByUID(uint32_t uid);

		static uint32_t addCombatArea(AreaCombat* area);
		static AreaCombat* getCombatArea(uint32_t areaId);

		static uint32_t addCombatObject(Combat* combat);
		static Combat* getCombatObject(uint32_t combatId);

		static uint32_t addConditionObject(Condition* condition);
		static Condition* getConditionObject(uint32_t conditionId);

		static uint32_t getLastCombatId() {return m_lastCombatId;}

	private:
		typedef std::map<uint64_t, Thing*> ThingMap;
		typedef std::vector<const LuaVariant*> VariantVector;
		typedef std::map<uint32_t, int32_t> StorageMap;
		typedef std::map<uint32_t, AreaCombat*> AreaMap;
		typedef std::map<uint32_t, Combat*> CombatMap;
		typedef std::map<uint32_t, Condition*> ConditionMap;
		typedef std::map<uint32_t, DBResult*> DBResultMap;
		typedef std::list<Item*> ItemList;

		//script file id
		int32_t m_scriptId;
		int32_t m_callbackId;
		bool m_timerEvent;
		LuaScriptInterface* m_interface;
		//script event desc
		std::string m_eventdesc;

		static StorageMap m_globalStorageMap;
		//unique id map
		static ThingMap m_globalMap;

		Position m_realPos;

		//item/creature map
		int32_t m_lastUID;
		ThingMap m_localMap;

		//temporary item list
		typedef std::map<ScriptEnvironment*, ItemList> TempItemListMap;
		static TempItemListMap m_tempItems;

		//area map
		static uint32_t m_lastAreaId;
		static AreaMap m_areaMap;

		//combat map
		static uint32_t m_lastCombatId;
		static CombatMap m_combatMap;

		//condition map
		static uint32_t m_lastConditionId;
		static ConditionMap m_conditionMap;
	
		//result map
		static uint32_t m_lastResultId;
		static DBResultMap m_tempResults;

		//for npc scripts
		Npc* m_curNpc;
};

class Position;

enum PlayerInfo_t
{
	PlayerInfoFood,
	PlayerInfoAccess,
	PlayerInfoLevel,
	PlayerInfoMagLevel,
	PlayerInfoName,
	PlayerInfoVocation,
	PlayerInfoMasterPos,
	PlayerInfoTown,
	PlayerInfoSoul,
	PlayerInfoFreeCap,
	PlayerInfoGuildId,
	PlayerInfoGuildLevel,
	PlayerInfoGuildName,
	PlayerInfoGuildRank,
	PlayerInfoGuildNick,
	PlayerInfoSex,
	PlayerInfoLookDirection,
	PlayerInfoGroupId,
	PlayerInfoGUID,
	PlayerInfoPremiumDays,
	PlayerInfoSkullType,
	PlayerInfoPzLock,
	PlayerInfoGhostStatus,
	PlayerInfoIp,
	PlayerInfoBankBalance
};

#define reportErrorFunc(a)  reportError(__FUNCTION__, a)

enum ErrorCode_t
{
	LUA_ERROR_PLAYER_NOT_FOUND,
	LUA_ERROR_CREATURE_NOT_FOUND,
	LUA_ERROR_ITEM_NOT_FOUND,
	LUA_ERROR_THING_NOT_FOUND,
	LUA_ERROR_TILE_NOT_FOUND,
	LUA_ERROR_HOUSE_NOT_FOUND,
	LUA_ERROR_COMBAT_NOT_FOUND,
	LUA_ERROR_CONDITION_NOT_FOUND,
	LUA_ERROR_AREA_NOT_FOUND,
	LUA_ERROR_CONTAINER_NOT_FOUND,
	LUA_ERROR_VARIANT_NOT_FOUND,
	LUA_ERROR_VARIANT_UNKNOWN,
	LUA_ERROR_SPELL_NOT_FOUND
};


class LuaScriptInterface
{
	public:
		LuaScriptInterface(std::string interfaceName);
		virtual ~LuaScriptInterface();

		virtual bool initState();
		bool reInitState();

		int32_t loadFile(const std::string& file, Npc* npc = NULL);
		int32_t loadBuffer(const std::string& text, Npc* npc /* = NULL*/);
		const std::string& getFileById(int32_t scriptId);

		int32_t getEvent(const std::string& eventName);

		static ScriptEnvironment* getScriptEnv()
		{
			assert(m_scriptEnvIndex >= 0 && m_scriptEnvIndex < 16);
			return &m_scriptEnv[m_scriptEnvIndex];
		}

		static bool reserveScriptEnv()
		{
			++m_scriptEnvIndex;
			if(m_scriptEnvIndex < 15)
				return true;
			else
			{
				--m_scriptEnvIndex;
				return false;
			}
		}

		static void releaseScriptEnv()
		{
			if(m_scriptEnvIndex >= 0)
			{
				m_scriptEnv[m_scriptEnvIndex].resetEnv();
				--m_scriptEnvIndex;
			}
		}

		static void reportError(const char* function, const std::string& error_desc);

		std::string getInterfaceName() {return m_interfaceName;}
		const std::string& getLastLuaError() const {return m_lastLuaError;}
		void dumpLuaStack();

		lua_State* getLuaState() {return m_luaState;}

		bool pushFunction(int32_t functionId);

		static int32_t luaErrorHandler(lua_State* L);
		int32_t callFunction(uint32_t nParams);

		//push/pop common structures
		static void pushThing(lua_State* L, Thing* thing, uint32_t thingid);
		static void pushVariant(lua_State* L, const LuaVariant& var);
		static void pushPosition(lua_State* L, const PositionEx& position);
		static void pushPosition(lua_State* L, const Position& position, uint32_t stackpos);
		static void pushCallback(lua_State* L, int32_t callback);

		static LuaVariant popVariant(lua_State* L);
		static void popPosition(lua_State* L, PositionEx& position);
		static void popPosition(lua_State* L, Position& position, uint32_t& stackpos);
		static uint32_t popNumber(lua_State* L);
		static double popFloatNumber(lua_State* L);
		static std::string popString(lua_State* L);
		static int32_t popCallback(lua_State* L);
		static bool popBoolean(lua_State* L);
	
		static int32_t getField(lua_State* L, const char* key);
		static uint32_t getFieldU32(lua_State* L, const char* key);
		static void setField(lua_State* L, const char* index, int32_t val);
		static void setField(lua_State* L, const char* index, const std::string& val);
		static std::string getFieldString(lua_State* L, const char* key);
		static void setFieldBool(lua_State* L, const char* index, bool val);
		static bool getFieldBool(lua_State* L, const char* key);
		static std::string escapeString(const std::string& string);

	protected:
		virtual bool closeState();

		virtual void registerFunctions();

		static std::string getErrorDesc(ErrorCode_t code);
		static bool getArea(lua_State* L, std::list<uint32_t>& list, uint32_t& rows);

		//lua functions
		static int32_t luaDoRemoveItem(lua_State* L);
		static int32_t luaDoFeedPlayer(lua_State* L);
		static int32_t luaDoPlayerSendCancel(lua_State* L);
		static int32_t luaDoSendDefaultCancel(lua_State* L);
		static int32_t luaGetClosestFreeTile(lua_State* L);
		static int32_t luaDoTeleportThing(lua_State* L);
		static int32_t luaDoTransformItem(lua_State* L);
		static int32_t luaDoSendMagicEffect(lua_State* L);
		static int32_t luaDoChangeTypeItem(lua_State* L);
		static int32_t luaDoSendAnimatedText(lua_State* L);
		static int32_t luaDoSendDistanceShoot(lua_State* L);
		static int32_t luaDoShowTextWindow(lua_State* L);
		static int32_t luaDoShowTextDialog(lua_State* L);
		static int32_t luaDoDecayItem(lua_State* L);
		static int32_t luaDoCreateItem(lua_State* L);
		static int32_t luaDoCreateItemEx(lua_State* L);
		static int32_t luaDoCreateTeleport(lua_State* L);
		static int32_t luaDoSummonCreature(lua_State* L);
		static int32_t luaDoConvinceCreature(lua_State* L);
		static int32_t luaGetMonsterTargetList(lua_State* L);
		static int32_t luaGetMonsterFriendList(lua_State* L);
		static int32_t luaDoSetMonsterTarget(lua_State* L);
		static int32_t luaDoMonsterChangeTarget(lua_State* L);
		static int32_t luaDoAddCondition(lua_State* L);
		static int32_t luaDoRemoveCondition(lua_State* L);
		static int32_t luaDoRemoveCreature(lua_State* L);
		static int32_t luaDoMoveCreature(lua_State* L);
		static int32_t luaDoPlayerSetBankBalance(lua_State* L);
		static int32_t luaGetHouseTilesSize(lua_State* L);
		static int32_t luaDoCreatureSay(lua_State* L);
		static int32_t luaDoPlayerAddSkillTry(lua_State* L);
		static int32_t luaDoCreatureAddHealth(lua_State* L);
		static int32_t luaDoPlayerAddMana(lua_State* L);
		static int32_t luaDoPlayerAddManaSpent(lua_State* L);
		static int32_t luaDoPlayerAddItem(lua_State* L);
		static int32_t luaDoPlayerAddItemEx(lua_State* L);
		static int32_t luaDoTileAddItemEx(lua_State* L);
		static int32_t luaDoRelocate(lua_State* L);
		static int32_t luaDoPlayerSendTextMessage(lua_State* L);
		static int32_t luaDoPlayerRemoveMoney(lua_State* L);
		static int32_t luaDoPlayerAddMoney(lua_State* L);
		static int32_t luaDoPlayerSetTown(lua_State* L);
		static int32_t luaDoPlayerSetVocation(lua_State* L);
		static int32_t luaDoPlayerRemoveItem(lua_State* L);
		static int32_t luaDoPlayerAddSoul(lua_State* L);
		static int32_t luaDoPlayerAddExp(lua_State* L);
		static int32_t luaDoPlayerSetGuildRank(lua_State* L);
		static int32_t luaDoPlayerSetGuildNick(lua_State* L);
		static int32_t luaDoPlayerSetSex(lua_State* L);
		static int32_t luaDoSetCreatureLight(lua_State* L);
		static int32_t luaDoSetCreatureDropLoot(lua_State* L);
		static int32_t luaGetPlayerSkullType(lua_State* L);
		static int32_t luaGetPlayerBankBalance(lua_State* L);
		static int32_t luaDoPlayerSetGuildId(lua_State* L);

		//queries
		static int32_t luaGetCreatureByName(lua_State* L);
		static int32_t luaGetPlayerGUIDByName(lua_State* L);
		static int32_t luaGetAccountNumberByPlayerName(lua_State* L);
		static int32_t luaGetPlayersByAccountNumber(lua_State* L);
		static int32_t luaGetIPByPlayerName(lua_State* L);
		static int32_t luaGetPlayersByIPAddress(lua_State* L);

		//get item info
		static int32_t luaGetItemRWInfo(lua_State* L);
		static int32_t luaGetThingfromPos(lua_State* L);
		static int32_t luaGetThing(lua_State* L);
		static int32_t luaGetThingPos(lua_State* L);
		static int32_t luaGetTileItemById(lua_State* L);
		static int32_t luaGetTileItemByType(lua_State* L);
		static int32_t luaGetTileThingByPos(lua_State* L);
		static int32_t luaGetTopCreature(lua_State* L);
		static int32_t luaHasProperty(lua_State* L);
		static int32_t luaGetDepotId(lua_State* L);

		//set item
		static int32_t luaDoSetItemActionId(lua_State* L);
		static int32_t luaDoSetItemText(lua_State* L);
		static int32_t luaDoSetItemSpecialDescription(lua_State* L);

		//get tile info
		static int32_t luaGetTilePzInfo(lua_State* L);
		static int32_t luaGetTileHouseInfo(lua_State* L);
		static int32_t luaQueryTileAddThing(lua_State* L);
		static int32_t luaGetTownName(lua_State* L);
		static int32_t luaGetTownTemplePosition(lua_State* L);

		//houses
		static int32_t luaGetHouseOwner(lua_State* L);
		static int32_t luaGetHouseName(lua_State* L);
		static int32_t luaGetHouseEntry(lua_State* L);
		static int32_t luaGetHouseRent(lua_State* L);
		static int32_t luaGetHouseTown(lua_State* L);
		static int32_t luaGetHouseAccessList(lua_State* L);
		static int32_t luaGetHouseByPlayerGUID(lua_State* L);
		static int32_t luaSetHouseOwner(lua_State* L);
		static int32_t luaSetHouseAccessList(lua_State* L);

		//get creature info functions
		static int32_t luaGetPlayerFood(lua_State* L);
		static int32_t luaGetPlayerIp(lua_State* L);
		static int32_t luaGetPlayerAccess(lua_State* L);
		static int32_t luaGetPlayerLevel(lua_State* L);
		static int32_t luaGetCreatureLookDirection(lua_State* L);
		static int32_t luaGetPlayerMagLevel(lua_State* L);
		static int32_t luaGetCreatureMana(lua_State* L);
		static int32_t luaGetCreatureMaxMana(lua_State* L);
		static int32_t luaGetCreatureHealth(lua_State* L);
		static int32_t luaGetCreatureMaxHealth(lua_State* L);
		static int32_t luaGetCreatureMaster(lua_State* L);
		static int32_t luaGetCreatureSummons(lua_State* L);
		static int32_t luaGetSpectators(lua_State* L);
		static int32_t luaGetCreatureSpeed(lua_State* L);
		static int32_t luaGetCreatureBaseSpeed(lua_State* L);
		static int32_t luaGetCreatureTarget(lua_State* L);
		static int32_t luaGetPlayerName(lua_State* L);
		static int32_t luaGetPlayerSkill(lua_State* L);
		static int32_t luaGetPlayerVocation(lua_State* L);
		static int32_t luaGetPlayerMasterPos(lua_State* L);
		static int32_t luaGetPromotedVocation(lua_State* L);
		static int32_t luaGetPlayerTown(lua_State* L);
		static int32_t luaGetPlayerItemCount(lua_State* L);
		static int32_t luaGetPlayerSoul(lua_State* L);
		static int32_t luaGetPlayerFreeCap(lua_State* L);
		static int32_t luaGetPlayerLight(lua_State* L);
		static int32_t luaGetPlayerSlotItem(lua_State* L);
		static int32_t luaGetPlayerItemById(lua_State* L);
		static int32_t luaGetPlayerLossPercent(lua_State* L);

		static int32_t luaGetPlayerDepotItems(lua_State* L);
		static int32_t luaGetPlayerGuildId(lua_State* L);
		static int32_t luaGetPlayerGuildLevel(lua_State* L);
		static int32_t luaGetPlayerGuildName(lua_State* L);
		static int32_t luaGetPlayerGuildRank(lua_State* L);
		static int32_t luaGetPlayerGuildNick(lua_State* L);
		static int32_t luaGetPlayerSex(lua_State* L);
		static int32_t luaGetPlayerLookDir(lua_State* L);
		static int32_t luaGetPlayerBlessing(lua_State* L);
		static int32_t luaDoPlayerAddBlessing(lua_State* L);
		static int32_t luaGetPlayerGUID(lua_State* L);
		static int32_t luaGetPlayerFlagValue(lua_State* L);
		static int32_t luaGetCreatureCondition(lua_State* L);
		static int32_t luaGetPlayerByNameWildcard(lua_State* L);
		static int32_t luaGetTownId(lua_State* L);

		static int32_t luaGetPlayerGroupId(lua_State* L);
		static int32_t luaSetPlayerGroupId(lua_State* L);

		static int32_t luaPlayerLearnInstantSpell(lua_State* L);
		static int32_t luaCanPlayerLearnInstantSpell(lua_State* L);
		static int32_t luaGetPlayerLearnedInstantSpell(lua_State* L);
		static int32_t luaGetPlayerInstantSpellInfo(lua_State* L);
		static int32_t luaGetPlayerInstantSpellCount(lua_State* L);
		static int32_t luaGetInstantSpellInfoByName(lua_State* L);
		static int32_t luaGetInstantSpellWords(lua_State* L);

		static int32_t luaGetPlayerStorageValue(lua_State* L);
		static int32_t luaSetPlayerStorageValue(lua_State* L);

		static int32_t luaGetGlobalStorageValue(lua_State* L);
		static int32_t luaSetGlobalStorageValue(lua_State* L);

		static int32_t luaDoPlayerAddOutfit(lua_State* L);
		static int32_t luaDoPlayerRemOutfit(lua_State* L);
		static int32_t luaCanPlayerWearOutfit(lua_State* L);

		static int32_t luaGetWorldType(lua_State* L);
		static int32_t luaGetWorldTime(lua_State* L);
		static int32_t luaGetWorldLight(lua_State* L);
		static int32_t luaGetWorldCreatures(lua_State* L);
		static int32_t luaGetWorldUpTime(lua_State* L);
		static int32_t luaBroadcastMessage(lua_State* L);
		static int32_t luaDoPlayerBroadcastMessage(lua_State* L);
		static int32_t luaGetGuildId(lua_State* L);
		static int32_t luaGetExperienceStage(lua_State* L);

		//type validation
		static int32_t luaIsPlayer(lua_State* L);
		static int32_t luaIsPlayerPzLocked(lua_State* L);
		static int32_t luaIsPlayerGhost(lua_State* L);
		static int32_t luaIsCreature(lua_State* L);
		static int32_t luaIsContainer(lua_State* L);
		static int32_t luaIsCorpse(lua_State* L);
		static int32_t luaIsMoveable(lua_State* L);
		static int32_t luaIsValidItemId(lua_State* L);

		//container
		static int32_t luaGetContainerSize(lua_State* L);
		static int32_t luaGetContainerCap(lua_State* L);
		static int32_t luaGetContainerItem(lua_State* L);
		static int32_t luaDoAddContainerItem(lua_State* L);

		//
		static int32_t luaCreateCombatObject(lua_State* L);
		static int32_t luaCreateCombatArea(lua_State* L);
		static int32_t luaSetCombatArea(lua_State* L);
		static int32_t luaSetCombatCondition(lua_State* L);
		static int32_t luaSetCombatParam(lua_State* L);
		static int32_t luaCreateConditionObject(lua_State* L);
		static int32_t luaSetConditionParam(lua_State* L);
		static int32_t luaAddDamageCondition(lua_State* L);
		static int32_t luaAddOutfitCondition(lua_State* L);

		static int32_t luaSetCombatCallBack(lua_State* L);
		static int32_t luaSetCombatFormula(lua_State* L);
		static int32_t luaSetConditionFormula(lua_State* L);
		static int32_t luaDoCombat(lua_State* L);

		static int32_t luaDoAreaCombatHealth(lua_State* L);
		static int32_t luaDoTargetCombatHealth(lua_State* L);

		//
		static int32_t luaDoAreaCombatMana(lua_State* L);
		static int32_t luaDoTargetCombatMana(lua_State* L);

		static int32_t luaDoAreaCombatCondition(lua_State* L);
		static int32_t luaDoTargetCombatCondition(lua_State* L);

		static int32_t luaDoAreaCombatDispel(lua_State* L);
		static int32_t luaDoTargetCombatDispel(lua_State* L);

		static int32_t luaDoChallengeCreature(lua_State* L);

		static int32_t luaNumberToVariant(lua_State* L);
		static int32_t luaStringToVariant(lua_State* L);
		static int32_t luaPositionToVariant(lua_State* L);
		static int32_t luaTargetPositionToVariant(lua_State* L);

		static int32_t luaVariantToNumber(lua_State* L);
		static int32_t luaVariantToString(lua_State* L);
		static int32_t luaVariantToPosition(lua_State* L);

		static int32_t luaDoChangeSpeed(lua_State* L);

		static int32_t luaDoCreatureChangeOutfit(lua_State* L);
		static int32_t luaSetCreatureOutfit(lua_State* L);
		static int32_t luaGetCreatureOutfit(lua_State* L);
		static int32_t luaSetMonsterOutfit(lua_State* L);
		static int32_t luaSetItemOutfit(lua_State* L);
		static int32_t luaGetCreaturePosition(lua_State* L);
		static int32_t luaGetCreatureName(lua_State* L);

		static int32_t luaIsItemStackable(lua_State* L);
		static int32_t luaIsItemRune(lua_State* L);
		static int32_t luaIsItemDoor(lua_State* L);
		static int32_t luaIsItemContainer(lua_State* L);
		static int32_t luaIsItemFluidContainer(lua_State* L);
		static int32_t luaIsItemMoveable(lua_State* L);
		static int32_t luaGetItemName(lua_State* L);
		static int32_t luaGetItemDescriptions(lua_State* L);
		static int32_t luaGetItemWeight(lua_State* L);
		static int32_t luaGetItemIdByName(lua_State* L);
		static int32_t luaIsSightClear(lua_State* L);

		static int32_t luaDebugPrint(lua_State* L);
		static int32_t luaIsInArray(lua_State* L);
		static int32_t luaGetFluidSourceType(lua_State* L);
		static int32_t luaAddEvent(lua_State* L);
		static int32_t luaStopEvent(lua_State* L);
		static int32_t luaRegisterCreatureEvent(lua_State* L);

		static int32_t luaDoPlayerPopupFYI(lua_State* L);
		static int32_t luaMayNotMove(lua_State* L);

		static int32_t luaDoPlayerAddPremiumDays(lua_State* L);
		static int32_t luaDoPlayerRemovePremiumDays(lua_State* L);
		static int32_t luaGetPlayerPremiumDays(lua_State* L);

		static int32_t luaGetOnlinePlayers(lua_State* L);
		static int32_t luaDoSaveServer(lua_State* L);
		static int32_t luaDoCleanMap(lua_State* L);
		static int32_t luaGetPlayersOnline(lua_State* L);

		static int32_t luaDoAddMark(lua_State* L);
		static int32_t luaGetGameState(lua_State* L);
		static int32_t luaDoSetGameState(lua_State* L);

		static int32_t luaGetPlayerParty(lua_State* L);
		static int32_t luaDoPlayerJoinParty(lua_State* L);
		static int32_t luaGetPartyMembers(lua_State* L);

		static int32_t luaDoCreateMonster(lua_State* L);
		static int32_t luaDoCreateNpc(lua_State* L);
		static int32_t luaDoSummonMonster(lua_State* L);

		//

		static int32_t internalGetPlayerInfo(lua_State* L, PlayerInfo_t info);

		static const luaL_Reg luaBitReg[13];
		static int luaBitNot(lua_State* L);
		static int luaBitAnd(lua_State* L);
		static int luaBitOr(lua_State* L);
		static int luaBitXor(lua_State* L);
		static int luaBitLeftShift(lua_State* L);
		static int luaBitRightShift(lua_State* L);
		static int luaBitUNot(lua_State* L);
		static int luaBitUAnd(lua_State* L);
		static int luaBitUOr(lua_State* L);
		static int luaBitUXor(lua_State* L);
		static int luaBitULeftShift(lua_State* L);
		static int luaBitURightShift(lua_State* L);

		static const luaL_Reg luaDatabaseTable[10];
		static int32_t luaDatabaseExecute(lua_State* L);
		static int32_t luaDatabaseStoreQuery(lua_State* L);
		static int32_t luaDatabaseEscapeString(lua_State* L);
		static int32_t luaDatabaseEscapeBlob(lua_State* L);
		static int32_t luaDatabaseLastInsertId(lua_State* L);
		static int32_t luaDatabaseStringComparer(lua_State* L);
		static int32_t luaDatabaseUpdateLimiter(lua_State* L);
		static int32_t luaDatabaseConnected(lua_State* L);
		static int32_t luaDatabaseTableExists(lua_State* L);

		static const luaL_Reg luaResultTable[8];
		static int32_t luaResultGetDataInt(lua_State* L);
		static int32_t luaResultGetDataLong(lua_State* L);
		static int32_t luaResultGetDataString(lua_State* L);
		static int32_t luaResultGetDataStream(lua_State* L);
		static int32_t luaResultGetAllData(lua_State* L);
		static int32_t luaResultNext(lua_State* L);
		static int32_t luaResultFree(lua_State* L);

		lua_State* m_luaState;
		std::string m_lastLuaError;

	private:
		static ScriptEnvironment m_scriptEnv[16];
		static int32_t m_scriptEnvIndex;

		int32_t m_runningEventId;
		std::string m_loadingFile;

		//script file cache
		typedef std::map<int32_t , std::string> ScriptsCache;
		ScriptsCache m_cacheFiles;

		//events information
		struct LuaTimerEventDesc
		{
			int32_t scriptId;
			int32_t function;
			std::list<int> parameters;
			uint32_t eventId;
		};
		uint32_t m_lastEventTimerId;

		typedef std::map<uint32_t , LuaTimerEventDesc > LuaTimerEvents;
		LuaTimerEvents m_timerEvents;

		static int32_t protectedCall(lua_State* L, int32_t nargs, int32_t nresults);
		std::string getStackTrace(const std::string& error_desc);

		void executeTimerEvent(uint32_t eventIndex);

		std::string m_interfaceName;

		friend class Game;
};

#endif
