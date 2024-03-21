//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// class representing the gamestate
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

#ifndef __OTSERV_GAME_H__
#define __OTSERV_GAME_H__

#include <queue>
#include <vector>
#include <set>

#include "map.h"
#include "position.h"
#include "item.h"
#include "container.h"
#include "player.h"
#include "spawn.h"
#include "templates.h"
#include "scheduler.h"
#include "npc.h"
#include "iologindata.h"

class Creature;
class Monster;
class Npc;
class CombatInfo;

enum stackPosType_t
{
	STACKPOS_NORMAL,
	STACKPOS_MOVE,
	STACKPOS_LOOK,
	STACKPOS_USE
};

enum WorldType_t
{
	WORLD_TYPE_NO_PVP = 1,
	WORLD_TYPE_PVP = 2,
	WORLD_TYPE_PVP_ENFORCED = 3
};

enum GameState_t
{
	GAME_STATE_FIRST = 1,
	GAME_STATE_STARTUP = GAME_STATE_FIRST,
	GAME_STATE_INIT = 2,
	GAME_STATE_NORMAL = 3,
	GAME_STATE_CLOSED = 4,
	GAME_STATE_CLOSING = 5,
	GAME_STATE_SHUTDOWN = 6,
	GAME_STATE_LAST = GAME_STATE_SHUTDOWN
};

enum LightState_t
{
	LIGHT_STATE_DAY,
	LIGHT_STATE_NIGHT,
	LIGHT_STATE_SUNSET,
	LIGHT_STATE_SUNRISE
};

struct RuleViolation
{
	RuleViolation(Player* _reporter, const std::string& _text, uint32_t _time) :
		reporter(_reporter),
		text(_text),
		time(_time),
		gamemaster(NULL),
		isOpen(true)
	{
	}

	Player* reporter;
	std::string text;
	uint32_t time;
	Player* gamemaster;
	bool isOpen;

	private:
		RuleViolation(const RuleViolation&);
};

typedef std::map< uint32_t, shared_ptr<RuleViolation> > RuleViolationsMap;

#define EVENT_LIGHTINTERVAL 10000
#define EVENT_DECAYINTERVAL 10000

typedef std::vector< std::pair<std::string, uint16_t> > Highscore;

/**
  * Main Game class.
  * This class is responsible to control everything that happens
  */

class Game
{
	public:
		Game();
		~Game();

		Highscore getHighscore(uint16_t skill);
		void timedHighscoreUpdate();
		bool reloadHighscores();
		std::string getHighscoreString(unsigned short skill);

		void autoSave();
		void prepareServerSave();
		void serverSave();

		/**
		  * Load a map.
		  * \param filename Mapfile to load
		  * \returns int32_t 0 built-in spawns, 1 needs xml spawns, 2 needs sql spawns, -1 if got error
		  */
		int32_t loadMap(std::string filename);

		/**
		  * Get the map size - info purpose only
		  * \param width width of the map
		  * \param height height of the map
		  */
		void getMapDimensions(uint32_t& width, uint32_t& height)
		{
			width = map->mapWidth;
			height = map->mapHeight;
			return;
		}

		void setWorldType(WorldType_t type);
		WorldType_t getWorldType() const {return worldType;}
		int32_t getInFightTicks() {return inFightTicks;}

		Cylinder* internalGetCylinder(Player* player, const Position& pos);
		Thing* internalGetThing(Player* player, const Position& pos, int32_t index,
			uint32_t spriteId = 0, stackPosType_t type = STACKPOS_NORMAL);
		void internalGetPosition(Item* item, Position& pos, uint8_t& stackpos);

		std::string getTradeErrorDescription(ReturnValue ret, Item* item);

		/**
		  * Get a single tile of the map.
		  * \returns A pointer to the tile
		*/
		Tile* getTile(uint32_t x, uint32_t y, uint32_t z);

		/**
		  * Get a leaf of the map.
		  * \returns A pointer to a leaf
		*/
		QTreeLeafNode* getLeaf(uint32_t x, uint32_t y);

		/**
		  * Returns a creature based on the unique creature identifier
		  * \param id is the unique creature id to get a creature pointer to
		  * \returns A Creature pointer to the creature
		  */
		Creature* getCreatureByID(uint32_t id);

		/**
		  * Returns a player based on the unique creature identifier
		  * \param id is the unique player id to get a player pointer to
		  * \returns A Pointer to the player
		  */
		Player* getPlayerByID(uint32_t id);

		/**
		  * Returns a creature based on a string name identifier
		  * \param s is the name identifier
		  * \returns A Pointer to the creature
		  */
		Creature* getCreatureByName(const std::string& s);

		/**
		  * Returns a player based on a string name identifier
		  * \param s is the name identifier
		  * \returns A Pointer to the player
		  */
		Player* getPlayerByName(const std::string& s);

		/**
		  * Returns a player based on a string name identifier, with support for the "~" wildcard.
		  * \param s is the name identifier, with or without wildcard
		  * \param player will point to the found player (if any)
		  * \return "RET_PLAYERWITHTHISNAMEISNOTONLINE" or "RET_NAMEISTOOAMBIGIOUS"
		  */
		ReturnValue getPlayerByNameWildcard(const std::string& s, Player*& player);

		/**
		  * Returns a player based on an account number identifier
		  * \param acc is the account identifier
		  * \returns A Pointer to the player
		  */
		Player* getPlayerByAccount(uint32_t acc);

		/**
		  * Returns all players based on their account number identifier
		  * \param acc is the account identifier
		  * \return A vector of all players with the selected account number
		  */
		PlayerVector getPlayersByAccount(uint32_t acc);

		/**
		  * Returns all players with a certain IP address
		  * \param ip is the IP address of the clients, as an unsigned long
		  * \param mask An IP mask, default 255.255.255.255
		  * \return A vector of all players with the selected IP
		  */
		PlayerVector getPlayersByIP(uint32_t ip, uint32_t mask = 0xFFFFFFFF);

		/* Place Creature on the map without sending out events to the surrounding.
		  * \param creature Creature to place on the map
		  * \param pos The position to place the creature
		  * \param forced If true, placing the creature will not fail because of obstacles (creatures/items)
		  */
		bool internalPlaceCreature(Creature* creature, const Position& pos, bool forced = false);

		/**
		  * Place Creature on the map.
		  * \param creature Creature to place on the map
		  * \param pos The position to place the creature
		  * \param forced If true, placing the creature will not fail because of obstacles (creatures/items)
		  */
		bool placeCreature(Creature* creature, const Position& pos, bool force = false);
		ReturnValue placeSummon(Creature* creature, const std::string& name);

		/**
		  * Remove Creature from the map.
		  * Removes the Creature the map
		  * \param c Creature to remove
		  */
		bool removeCreature(Creature* creature, bool isLogout = true);

		void addCreatureCheck(Creature* creature);
		void removeCreatureCheck(Creature* creature);

		bool violationWindow(uint32_t playerId, std::string targetPlayerName, int32_t reasonId, int32_t actionId, std::string banComment, std::string statement, bool IPBanishment);

		uint32_t getPlayersOnline() {return (uint32_t)Player::listPlayer.list.size();}
		uint32_t getMonstersOnline() {return (uint32_t)Monster::listMonster.list.size();}
		uint32_t getNpcsOnline() {return (uint32_t)Npc::listNpc.list.size();}
		uint32_t getCreaturesOnline() {return (uint32_t)listCreature.list.size();}
		uint32_t getLastPlayersRecord() {return lastPlayersRecord;}

		void getWorldLightInfo(LightInfo& lightInfo);

		void getSpectators(SpectatorVec& list, const Position& centerPos, bool checkforduplicate = false, bool multifloor = false,
			int32_t minRangeX = 0, int32_t maxRangeX = 0,
			int32_t minRangeY = 0, int32_t maxRangeY = 0)
		{
			map->getSpectators(list, centerPos, checkforduplicate, multifloor, minRangeX, maxRangeY, minRangeY, maxRangeY);
		}

		const SpectatorVec& getSpectators(const Position& centerPos) {return map->getSpectators(centerPos);}

		void clearSpectatorCache()
		{
			if(map)
				map->clearSpectatorCache();
		}

		ReturnValue internalMoveCreature(Creature* creature, Direction direction, uint32_t flags = 0);
		ReturnValue internalMoveCreature(Creature* creature, Cylinder* fromCylinder, Cylinder* toCylinder, uint32_t flags = 0);

		ReturnValue internalMoveItem(Cylinder* fromCylinder, Cylinder* toCylinder, int32_t index,
			Item* item, uint32_t count, Item** _moveItem, uint32_t flags = 0);

		ReturnValue internalAddItem(Cylinder* toCylinder, Item* item, int32_t index = INDEX_WHEREEVER,
			uint32_t flags = 0, bool test = false);
		ReturnValue internalAddItem(Cylinder* toCylinder, Item* item, int32_t index,
			uint32_t flags, bool test, uint32_t& remainderCount);
		ReturnValue internalRemoveItem(Item* item, int32_t count = -1,  bool test = false);

		ReturnValue internalPlayerAddItem(Player* player, Item* item, bool dropOnMap = true, slots_t slot = SLOT_WHEREEVER);

		/**
		  * Find an item of a certain type
		  * \param cylinder to search the item
		  * \param itemId is the item to remove
		  * \param subType is the extra type an item can have such as charges/fluidtype, default is -1
			* meaning it's not used
		  * \param depthSearch if true it will check child containers aswell
		  * \returns A pointer to the item to an item and NULL if not found
		  */
		Item* findItemOfType(Cylinder* cylinder, uint16_t itemId,
			bool depthSearch = true, int32_t subType = -1);

		/**
		  * Remove item(s) of a certain type
		  * \param cylinder to remove the item(s) from
		  * \param itemId is the item to remove
		  * \param count is the amount to remove
		  * \param subType is the extra type an item can have such as charges/fluidtype, default is -1
			* meaning it's not used
		  * \returns true if the removal was successful
		  */
		bool removeItemOfType(Cylinder* cylinder, uint16_t itemId, int32_t count, int32_t subType = -1);

		/**
		  * Get the amount of money in a a cylinder
		  * \returns the amount of money found
		  */
		uint32_t getMoney(const Cylinder* cylinder);

		/**
		  * Remove/Add item(s) with a monetary value
		  * \param cylinder to remove the money from
		  * \param money is the amount to remove
		  * \param flags optional flags to modifiy the default behaviour
		  * \returns true if the removal was successful
		  */
		bool removeMoney(Cylinder* cylinder, int32_t money, uint32_t flags = 0);

		/**
		  * Add item(s) with monetary value
		  * \param cylinder which will receive money
		  * \param money the amount to give
		  * \param flags optional flags to modify default behavior
		  */
		void addMoney(Cylinder* cylinder, int32_t money, uint32_t flags = 0);

		/**
		  * Transform one item to another type/count
		  * \param item is the item to transform
		  * \param newId is the new itemid
		  * \param newCount is the new count value, use default value (-1) to not change it
		  * \returns true if the tranformation was successful
		  */
		Item* transformItem(Item* item, uint16_t newId, int32_t newCount = -1);

		/**
		  * Teleports an object to another position
		  * \param thing is the object to teleport
		  * \param newPos is the new position
		  * \returns true if the teleportation was successful
		  */
		ReturnValue internalTeleport(Thing* thing, const Position& newPos, bool pushMove);

		/**
			* Turn a creature to a different direction.
			* \param creature Creature to change the direction
			* \param dir Direction to turn to
			*/
		bool internalCreatureTurn(Creature* creature, Direction dir);

		/**
		  * Creature wants to say something.
		  * \param creature Creature pointer
		  * \param type Type of message
		  * \param text The text to say
		  */
		bool internalCreatureSay(Creature* creature, SpeakClasses type, const std::string& text);

		Position getClosestFreeTile(Creature* creature, Position pos, bool extended = false, bool ignoreHouse = true);
			std::string getSearchString(const Position fromPos, const Position toPos, bool fromIsCreature = false, bool toIsCreature = false);

		int32_t getMotdNum();
		void loadMotd();
		void loadPlayersRecord();
		void checkPlayersRecord();

		void sendGuildMotd(uint32_t playerId, uint32_t guildId);
		void sendRVRDisabled(uint32_t playerId);
		void kickPlayer(uint32_t playerId, bool displayEffect);
		bool playerReportBug(uint32_t playerId, std::string bug);

		bool internalStartTrade(Player* player, Player* partner, Item* tradeItem);
		bool internalCloseTrade(Player* player);
		bool playerBroadcastMessage(Player* player, const std::string& text, SpeakClasses type);
		bool broadcastMessage(const std::string& text, MessageClasses type);

		//Implementation of player invoked events
		bool playerMoveThing(uint32_t playerId, const Position& fromPos, uint16_t spriteId, uint8_t fromStackPos,
			const Position& toPos, uint8_t count);
		bool playerMoveCreature(uint32_t playerId, uint32_t movingCreatureId,
			const Position& movingCreatureOrigPos, const Position& toPos);
		bool playerMoveItem(uint32_t playerId, const Position& fromPos,
			uint16_t spriteId, uint8_t fromStackPos, const Position& toPos, uint8_t count);
		bool playerMove(uint32_t playerId, Direction direction);
		bool playerCreatePrivateChannel(uint32_t playerId);
		bool playerChannelInvite(uint32_t playerId, const std::string& name);
		bool playerChannelExclude(uint32_t playerId, const std::string& name);
		bool playerRequestChannels(uint32_t playerId);
		bool playerOpenChannel(uint32_t playerId, uint16_t channelId);
		bool playerCloseChannel(uint32_t playerId, uint16_t channelId);
		bool playerOpenPrivateChannel(uint32_t playerId, std::string& receiver);
		bool playerProcessRuleViolation(uint32_t playerId, const std::string& name);
		bool playerCloseRuleViolation(uint32_t playerId, const std::string& name);
		bool playerCancelRuleViolation(uint32_t playerId);
		bool playerReceivePing(uint32_t playerId);
		bool playerAutoWalk(uint32_t playerId, std::list<Direction>& listDir);
		bool playerStopAutoWalk(uint32_t playerId);
		bool playerUseItemEx(uint32_t playerId, const Position& fromPos, uint8_t fromStackPos,
			uint16_t fromSpriteId, const Position& toPos, uint8_t toStackPos, uint16_t toSpriteId, bool isHotkey);
		bool playerUseItem(uint32_t playerId, const Position& pos, uint8_t stackPos,
			uint8_t index, uint16_t spriteId, bool isHotkey);
		bool playerUseBattleWindow(uint32_t playerId, const Position& fromPos,
			uint8_t fromStackPos, uint32_t creatureId, uint16_t spriteId, bool isHotkey);
		bool playerCloseContainer(uint32_t playerId, uint8_t cid);
		bool playerMoveUpContainer(uint32_t playerId, uint8_t cid);
		bool playerUpdateContainer(uint32_t playerId, uint8_t cid);
		bool playerUpdateTile(uint32_t playerId, const Position& pos);
		bool playerRotateItem(uint32_t playerId, const Position& pos, uint8_t stackPos, const uint16_t spriteId);
		bool playerWriteItem(uint32_t playerId, uint32_t windowTextId, const std::string& text);
		bool playerUpdateHouseWindow(uint32_t playerId, uint8_t listId, uint32_t windowTextId, const std::string& text);
		bool playerRequestTrade(uint32_t playerId, const Position& pos, uint8_t stackPos,
			uint32_t tradePlayerId, uint16_t spriteId);
		bool playerAcceptTrade(uint32_t playerId);
		bool playerLookInTrade(uint32_t playerId, bool lookAtCounterOffer, int index);
		bool playerCloseTrade(uint32_t playerId);
		bool playerSetAttackedCreature(uint32_t playerId, uint32_t creatureId);
		bool playerFollowCreature(uint32_t playerId, uint32_t creatureId);
		bool playerCancelAttackAndFollow(uint32_t playerId);
		bool playerSetFightModes(uint32_t playerId, fightMode_t fightMode, chaseMode_t chaseMode, secureMode_t secureMode);
		bool playerLookAt(uint32_t playerId, const Position& pos, uint16_t spriteId, uint8_t stackPos);
		bool playerRequestAddVip(uint32_t playerId, const std::string& name);
		bool playerRequestRemoveVip(uint32_t playerId, uint32_t guid);
		bool playerTurn(uint32_t playerId, Direction dir);
		bool playerShowQuestLog(uint32_t playerId);
		bool playerShowQuestLine(uint32_t playerId, uint16_t questId);
		bool playerRequestOutfit(uint32_t playerId);
		bool playerSay(uint32_t playerId, uint16_t channelId, SpeakClasses type,
			const std::string& receiver, const std::string& text);
		bool playerChangeOutfit(uint32_t playerId, Outfit_t outfit);
		bool playerInviteToParty(uint32_t playerId, uint32_t invitedId);
		bool playerJoinParty(uint32_t playerId, uint32_t leaderId);
		bool playerRevokePartyInvitation(uint32_t playerId, uint32_t invitedId);
		bool playerPassPartyLeadership(uint32_t playerId, uint32_t newLeaderId);
		bool playerLeaveParty(uint32_t playerId);

		void updatePremium(Account account);

		void cleanup();
		void shutdown();
		void FreeThing(Thing* thing);

		bool canThrowObjectTo(const Position& fromPos, const Position& toPos, bool checkLineOfSight = true,
			int32_t rangex = Map::maxClientViewportX, int32_t rangey = Map::maxClientViewportY);
		bool isSightClear(const Position& fromPos, const Position& toPos, bool sameFloor);

		bool getPathTo(const Creature* creature, const Position& destPos,
			std::list<Direction>& listDir, int32_t maxSearchDist /*= -1*/);

		bool getPathToEx(const Creature* creature, const Position& targetPos, std::list<Direction>& dirList,
			const FindPathParams& fpp);

		bool getPathToEx(const Creature* creature, const Position& targetPos, std::list<Direction>& dirList,
			uint32_t minTargetDist, uint32_t maxTargetDist, bool fullPathSearch = true,
			bool clearSight = true, int32_t maxSearchDist = -1);

		void changeSpeed(Creature* creature, int32_t varSpeedDelta);
		void internalCreatureChangeOutfit(Creature* creature, const Outfit_t& oufit);
		void internalCreatureChangeVisible(Creature* creature, bool visible);
		void changeLight(const Creature* creature);
		void updateCreatureSkull(Player* player);

		void sendPublicSquare(Player* sender, SquareColor_t color);

		GameState_t getGameState();
		void setGameState(GameState_t newState);
		void saveGameState();
		void loadGameState();
		void refreshMap();

		//Events
		void checkCreatureWalk(uint32_t creatureId);
		void updateCreatureWalk(uint32_t creatureId);
		void checkCreatureAttack(uint32_t creatureId);
		void checkCreatures();
		void checkLight();

		bool combatBlockHit(CombatType_t combatType, Creature* attacker, Creature* target,
			int32_t& healthChange, bool checkDefense, bool checkArmor);

		bool combatChangeHealth(CombatType_t combatType, Creature* attacker, Creature* target, int32_t healthChange);
		bool combatChangeMana(Creature* attacker, Creature* target, int32_t manaChange);

		//animation help functions
		void addCreatureHealth(const Creature* target);
		void addCreatureHealth(const SpectatorVec& list, const Creature* target);
		void addAnimatedText(const Position& pos, uint8_t textColor,
			const std::string& text);
		void addAnimatedText(const SpectatorVec& list, const Position& pos, uint8_t textColor,
			const std::string& text);
		void addMagicEffect(const Position& pos, uint8_t effect, bool ghostMode = false);
		void addMagicEffect(const SpectatorVec& list, const Position& pos, uint8_t effect, bool ghostMode = false);
		void addDistanceEffect(const Position& fromPos, const Position& toPos,
		uint8_t effect);

		void startDecay(Item* item);

		Map* getMap() { return map;}
		const Map* getMap() const { return map;}

		int32_t getLightHour() const {return lightHour;}

		const RuleViolationsMap& getRuleViolations() const {return ruleViolations;}
		bool cancelRuleViolation(Player* player);
		bool closeRuleViolation(Player* player);

		bool loadExperienceStages();
		uint64_t getExperienceStage(uint32_t level);

		void setServerSaveMessage(int16_t key, bool value) {serverSaveMessage[key] = value;}
		bool getServerSaveMessage(int16_t key) const {return serverSaveMessage[key];}

	protected:
		bool playerSaySpell(Player* player, const std::string& text);
		bool playerWhisper(Player* player, const std::string& text);
		bool playerYell(Player* player, const std::string& text);
		bool playerSpeakTo(Player* player, SpeakClasses type, const std::string& receiver, const std::string& text);
		bool playerTalkToChannel(Player* player, SpeakClasses type, const std::string& text, unsigned short channelId);
		bool playerReportRuleViolation(Player* player, const std::string& text);
		bool playerContinueReport(Player* player, const std::string& text);

		Highscore highscoreStorage[9];
		time_t lastHSUpdate;

		bool serverSaveMessage[3];

		std::vector<Thing*> ToReleaseThings;

		uint32_t checkLightEvent;

		//list of items that are in trading state, mapped to the player
		std::map<Item*, uint32_t> tradeItems;

		//list of reported rule violations, for correct channel listing
		RuleViolationsMap ruleViolations;

		AutoList<Creature> listCreature;

		size_t checkCreatureLastIndex;
		std::vector<Creature*> checkCreatureVectors[EVENT_CREATURECOUNT];

		#ifdef __DEBUG_CRITICALSECTION__
		static OTSYS_THREAD_RETURN monitorThread(void *p);
		#endif

		struct GameEvent
		{
			int64_t tick;
			int type;
			void* data;
		};

		void checkDecay();
		void internalDecayItem(Item* item);

		typedef std::list<Item*> DecayList;
		DecayList decayItems;
		DecayList toDecayItems;

		static const int32_t LIGHT_LEVEL_DAY = 250;
		static const int32_t LIGHT_LEVEL_NIGHT = 40;
		static const int32_t SUNSET = 1305;
		static const int32_t SUNRISE = 430;
		int32_t lightLevel;
		LightState_t lightState;
		int32_t lightHour;
		int32_t lightHourDelta;

		uint32_t maxPlayers;
		uint32_t inFightTicks;

		GameState_t gameState;
		WorldType_t worldType;
		Map* map;

		void savePlayersRecord();
		std::string lastMotdText;
		int32_t lastMotdNum;
		uint32_t lastPlayersRecord;

		typedef std::map<uint32_t,uint32_t> StageList;
		StageList stages;
		uint32_t lastStageLevel;
		bool useLastStageLevel;
};
#endif
