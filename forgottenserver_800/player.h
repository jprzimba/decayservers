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

#ifndef __PLAYER__
#define __PLAYER__

#include "otsystem.h"
#include "enums.h"

#include "creature.h"
#include "cylinder.h"

#include "container.h"
#include "depot.h"

#include "outfit.h"
#include "vocation.h"
#include "group.h"

#include "protocolgame.h"
#include "ioguild.h"
#include "party.h"
#include "npc.h"

class House;
class NetworkMessage;
class Weapon;
class ProtocolGame;
class Npc;
class Party;
class SchedulerTask;
class Quest;

enum skillsid_t
{
	SKILL_LEVEL = 0,
	SKILL_TRIES = 1,
	SKILL_PERCENT = 2
};

enum playerinfo_t
{
	PLAYERINFO_LEVEL,
	PLAYERINFO_LEVELPERCENT,
	PLAYERINFO_HEALTH,
	PLAYERINFO_MAXHEALTH,
	PLAYERINFO_MANA,
	PLAYERINFO_MAXMANA,
	PLAYERINFO_MAGICLEVEL,
	PLAYERINFO_MAGICLEVELPERCENT,
	PLAYERINFO_SOUL,
};

enum freeslot_t
{
	SLOT_TYPE_NONE,
	SLOT_TYPE_INVENTORY,
	SLOT_TYPE_CONTAINER
};

enum chaseMode_t
{
	CHASEMODE_STANDSTILL,
	CHASEMODE_FOLLOW,
};

enum fightMode_t
{
	FIGHTMODE_ATTACK,
	FIGHTMODE_BALANCED,
	FIGHTMODE_DEFENSE
};

enum secureMode_t
{
	SECUREMODE_ON,
	SECUREMODE_OFF
};

enum tradestate_t
{
	TRADE_NONE,
	TRADE_INITIATED,
	TRADE_ACCEPT,
	TRADE_ACKNOWLEDGE,
	TRADE_TRANSFER
};

enum AccountManager_t
{
	MANAGER_NONE,
	MANAGER_NEW,
	MANAGER_ACCOUNT,
	MANAGER_NAMELOCK
};

enum GamemasterCondition_t
{
	GAMEMASTER_INVISIBLE = 0,
	GAMEMASTER_IGNORE = 1,
	GAMEMASTER_TELEPORT = 2
};

enum Exhaust_t
{
	EXHAUST_COMBAT = 1,
	EXHAUST_HEALING = 2
};

typedef std::set<uint32_t> VIPListSet;
typedef std::vector<std::pair<uint32_t, Container*> > ContainerVector;
typedef std::map<uint32_t, std::pair<Depot*, bool> > DepotMap;
typedef std::map<uint32_t, uint32_t> MuteCountMap;
typedef std::list<std::string> LearnedInstantSpellList;
typedef std::list<uint32_t> InvitedToGuildsList;
typedef std::list<Party*> PartyList;

#define SPEED_MAX 1500
#define SPEED_MIN 10
#define STAMINA_MAX (56 * 60 * 60 * 1000)
#define STAMINA_MULTIPLIER (60 * 1000)

class Player : public Creature, public Cylinder
{
	public:
		Player(const std::string& name, ProtocolGame* p);
		virtual ~Player();

		virtual Player* getPlayer() {return this;}
		virtual const Player* getPlayer() const {return this;}

		static MuteCountMap muteCountMap;

		virtual const std::string& getName() const {return name;}
		virtual const std::string& getNameDescription() const {return nameDescription;}
		virtual std::string getDescription(int32_t lookDistance) const;

		const std::string& getSpecialDescription() const {return specialDescription;}
		void setSpecialDescription(const std::string& desc) {specialDescription = desc;}

		void manageAccount(const std::string& text);
		bool isAccountManager() const {return (accountManager != MANAGER_NONE);}
		void kickPlayer(bool displayEffect, bool forceLogout);

		void setGUID(uint32_t _guid) {guid = _guid;}
		uint32_t getGUID() const {return guid;}

		static AutoList<Player> autoList;
		virtual uint32_t rangeId() {return PLAYER_ID_RANGE;}

		void addList();
		void removeList();

		static uint64_t getExpForLevel(uint32_t lv)
		{
			lv--;
			return ((50ULL * lv * lv * lv) - (150ULL * lv * lv) + (400ULL * lv)) / 3ULL;
		}

		uint32_t getPromotionLevel() const {return promotionLevel;}
		void setPromotionLevel(uint32_t pLevel);

		bool changeOutfit(Outfit_t outfit, bool checkList);
		void hasRequestedOutfit(bool v) {requestedOutfit = v;}

		Vocation* getVocation() const {return vocation;}
		int32_t getPlayerInfo(playerinfo_t playerinfo) const;

		void setParty(Party* _party) {party = _party;}
		Party* getParty() const {return party;}
		PartyShields_t getPartyShield(const Creature* creature) const;
		bool isInviting(const Player* player) const;
		bool isPartner(const Player* player) const;
		void sendPlayerPartyIcons(Player* player);
		bool addPartyInvitation(Party* party);
		bool removePartyInvitation(Party* party);
		void clearPartyInvitations();

		uint32_t getGuildId() const {return guildId;}
		void setGuildId(uint32_t newId) {guildId = newId;}
		uint32_t getRankId() const {return rankId;}
		void setRankId(uint32_t newId) {rankId = newId;}

		GuildLevel_t getGuildLevel() const {return guildLevel;}
		bool setGuildLevel(GuildLevel_t newLevel, uint32_t rank = 0);

		const std::string& getGuildName() const {return guildName;}
		void setGuildName(const std::string& newName) {guildName = newName;}
		const std::string& getRankName() const {return rankName;}
		void setRankName(const std::string& newName) {rankName = newName;}

		const std::string& getGuildNick() const {return guildNick;}
		void setGuildNick(const std::string& newNick) {guildNick = newNick;}

		bool isGuildInvited(uint32_t guildId) const;
		void leaveGuild();

		void setFlags(uint64_t flags) {if(group) group->setFlags(flags);}
		bool hasFlag(PlayerFlags value) const {return group != NULL && group->hasFlag(value);}
		void setCustomFlags(uint64_t flags) {if(group) group->setCustomFlags(flags);}
		bool hasCustomFlag(PlayerCustomFlags value) const {return group != NULL && group->hasCustomFlag(value);}

		void addBlessing(int16_t blessing) {blessings += blessing;}
		bool hasBlessing(int16_t value) const {return (blessings & ((int16_t)1 << value));}
		uint16_t getBlessings() const;

		OperatingSystem_t getOperatingSystem() const {return operatingSystem;}
		void setOperatingSystem(OperatingSystem_t clientOs) {operatingSystem = clientOs;}
		uint32_t getClientVersion() const {return clientVersion;}
		void setClientVersion(uint32_t version) {clientVersion = version;}

		bool hasClient() const {return client;}
		bool isVirtual() const {return (getID() == 0);}
		void disconnect() {if(client) client->disconnect();}
		uint32_t getIP() const;
		bool canOpenCorpse(uint32_t ownerId);

		Container* getContainer(uint32_t cid);
		int32_t getContainerID(const Container* container) const;

		void addContainer(uint32_t cid, Container* container);
		void closeContainer(uint32_t cid);

		virtual bool setStorage(const uint32_t key, const std::string& value);
		virtual void eraseStorage(const uint32_t key);

		void generateReservedStorage();
		bool transferMoneyTo(const std::string& name, uint64_t amount);
		void increaseCombatValues(int32_t& min, int32_t& max, bool useCharges, bool countWeapon);

		void setGroupId(int32_t newId);
		int32_t getGroupId() const {return groupId;}
		void setGroup(Group* newGroup);
		Group* getGroup() const {return group;}

		virtual bool isGhost() const {return hasCondition(CONDITION_GAMEMASTER, GAMEMASTER_INVISIBLE) || hasFlag(PlayerFlag_CannotBeSeen);}

		void switchSaving() {saving = !saving;}
		bool isSaving() const {return saving;}

		uint32_t getIdleTime() const {return idleTime;}
		void setIdleTime(uint32_t amount) {idleTime = amount;}

		bool checkLoginDelay(uint32_t playerId) const;
		bool isTrading() const {return tradePartner;}

		uint32_t getAccount() const {return accountId;}
		uint16_t getAccess() const {return group ? group->getAccess() : 0;}
		uint16_t getGhostAccess() const {return group ? group->getGhostAccess() : 0;}

		bool isPremium() const;
		int32_t getPremiumDays() const {return premiumDays;}
		void addPremiumDays(int32_t days);

		uint32_t getLevel() const {return level;}
		uint64_t getExperience() const {return experience;}
		uint32_t getMagicLevel() const {return getPlayerInfo(PLAYERINFO_MAGICLEVEL);}
		uint64_t getSpentMana() const {return manaSpent;}

		uint32_t getVocationId() const {return vocation_id;}
		void setVocation(uint32_t vocId);
		uint16_t getSex(bool full) const {return full ? sex : sex % 2;}
		void setSex(uint16_t);

		uint64_t getStamina() const {return hasFlag(PlayerFlag_HasInfiniteStamina) ? STAMINA_MAX : stamina;}
		void setStamina(uint64_t value) {stamina = std::min((uint64_t)STAMINA_MAX, (uint64_t)std::max((uint64_t)0, value));}
		uint32_t getStaminaMinutes() const {return (uint32_t)(getStamina() / (uint64_t)STAMINA_MULTIPLIER);}
		void setStaminaMinutes(uint32_t value) {setStamina((uint64_t)(value * STAMINA_MULTIPLIER));}
		void useStamina(int64_t value) {stamina = std::min((int64_t)STAMINA_MAX, (int64_t)std::max((int64_t)0, ((int64_t)stamina + value)));}
		uint64_t getSpentStamina() {return (uint64_t)STAMINA_MAX - stamina;}

		int64_t getLastLoad() const {return lastLoad;}
		time_t getLastLogin() const {return lastLogin;}
		time_t getLastLogout() const {return lastLogout;}

		Position getLoginPosition() const {return loginPosition;}

		uint32_t getTown() const {return town;}
		void setTown(uint32_t _town) {town = _town;}

		virtual bool isPushable() const;
		virtual int32_t getThrowRange() const {return 1;}

		bool isMuted(uint16_t channelId, SpeakClasses type, uint32_t& time);
		void addMessageBuffer();
		void removeMessageBuffer();

		double getCapacity() const {return capacity;}
		void setCapacity(double newCapacity) {capacity = newCapacity;}
		double getFreeCapacity() const;

		virtual int32_t getSoul() const {return getPlayerInfo(PLAYERINFO_SOUL);}
		virtual int32_t getMaxHealth() const {return getPlayerInfo(PLAYERINFO_MAXHEALTH);}
		virtual int32_t getMaxMana() const {return getPlayerInfo(PLAYERINFO_MAXMANA);}
		int32_t getSoulMax() const {return soulMax;}

		Item* getInventoryItem(slots_t slot) const;
		Item* getEquippedItem(slots_t slot) const;

		bool isItemAbilityEnabled(slots_t slot) const {return inventoryAbilities[slot];}
		void setItemAbility(slots_t slot, bool enabled) {inventoryAbilities[slot] = enabled;}

		int32_t getVarSkill(skills_t skill) const {return varSkills[skill];}
		void setVarSkill(skills_t skill, int32_t modifier) {varSkills[skill] += modifier;}

		int32_t getVarStats(stats_t stat) const {return varStats[stat];}
		void setVarStats(stats_t stat, int32_t modifier);
		int32_t getDefaultStats(stats_t stat);

		void setConditionSuppressions(uint32_t conditions, bool remove);

		uint32_t getLossPercent(lossTypes_t lossType) const {return lossPercent[lossType];}
		void setLossPercent(lossTypes_t lossType, uint32_t newPercent) {lossPercent[lossType] = newPercent;}

		Depot* getDepot(uint32_t depotId, bool autoCreateDepot);
		bool addDepot(Depot* depot, uint32_t depotId);
		void useDepot(uint32_t depotId, bool value);

		virtual bool canSee(const Position& pos) const;
		virtual bool canSeeCreature(const Creature* creature) const;

		virtual bool canSeeInvisibility() const {return hasFlag(PlayerFlag_CanSenseInvisibility);}

		virtual RaceType_t getRace() const {return RACE_BLOOD;}

		//safe-trade functions
		void setTradeState(tradestate_t state) {tradeState = state;}
		tradestate_t getTradeState() {return tradeState;}
		Item* getTradeItem() {return tradeItem;}

		//V.I.P. functions
		void notifyLogIn(Player* loginPlayer);
		void notifyLogOut(Player* logoutPlayer);
		bool removeVIP(uint32_t guid);
		bool addVIP(uint32_t guid, std::string& name, bool isOnline, bool internal = false);

		//follow functions
		virtual bool setFollowCreature(Creature* creature, bool fullPathSearch = false);

		//follow events
		virtual void onFollowCreature(const Creature* creature);

		//walk events
		virtual void onWalk(Direction& dir);
		virtual void onWalkAborted();
		virtual void onWalkComplete();

		void stopWalk() {cancelNextWalk = true;}

		void setChaseMode(chaseMode_t mode);
		void setFightMode(fightMode_t mode) {fightMode = mode;}
		void setSecureMode(secureMode_t mode) {secureMode = mode;}
		secureMode_t getSecureMode() const {return secureMode;}

		//combat functions
		virtual bool setAttackedCreature(Creature* creature);
		bool isImmune(CombatType_t type) const;
		bool isImmune(ConditionType_t type) const;
		bool hasShield() const;
		virtual bool isAttackable() const;

		virtual void changeHealth(int32_t healthChange);
		virtual void changeMana(int32_t manaChange);
		void changeSoul(int32_t soulChange);

		bool isPzLocked() const {return pzLocked;}
		void setPzLocked(bool v) {pzLocked = v;}
		virtual BlockType_t blockHit(Creature* attacker, CombatType_t combatType, int32_t& damage,
			bool checkDefense = false, bool checkArmor = false);
		virtual void doAttacking(uint32_t interval);
		virtual bool hasExtraSwing() {return lastAttack > 0 && ((OTSYS_TIME() - lastAttack) >= getAttackSpeed());}
		int32_t getShootRange() const {return shootRange;}

		int32_t getSkill(skills_t skilltype, skillsid_t skillinfo) const;
		bool getAddAttackSkill() const {return addAttackSkillPoint;}
		BlockType_t getLastAttackBlockType() const {return lastAttackBlockType;}

		Item* getWeapon(bool ignoreAmmo = false);
		virtual WeaponType_t getWeaponType();
		int32_t getWeaponSkill(const Item* item) const;
		void getShieldAndWeapon(const Item* &shield, const Item* &weapon) const;

		virtual void drainHealth(Creature* attacker, CombatType_t combatType, int32_t damage);
		virtual void drainMana(Creature* attacker, CombatType_t combatType, int32_t damage);

		void addExperience(uint64_t exp);
		void removeExperience(uint64_t exp, bool updateStats = true);
		void addManaSpent(uint64_t amount, bool useMultiplier = true);
		void addSkillAdvance(skills_t skill, uint32_t count, bool useMultiplier = true);
		bool addUnjustifiedKill(const Player* attacked);

		virtual int32_t getArmor() const;
		virtual int32_t getDefense() const;
		virtual float getAttackFactor() const;
		virtual float getDefenseFactor() const;

		void addExhaust(uint32_t ticks, Exhaust_t type);
		void addInFightTicks(bool pzLock, int32_t ticks = 0);
		void addDefaultRegeneration(uint32_t addTicks);

		virtual double getGainedExperience(Creature* attacker) const;

		//combat event functions
		virtual void onAddCondition(ConditionType_t type, bool hadCondition);
		virtual void onAddCombatCondition(ConditionType_t type, bool hadCondition);
		virtual void onEndCondition(ConditionType_t type);
		virtual void onCombatRemoveCondition(const Creature* attacker, Condition* condition);
		virtual void onTickCondition(ConditionType_t type, int32_t interval, bool& _remove);
		virtual void onAttackedCreature(Creature* target);
		virtual void onSummonAttackedCreature(Creature* summon, Creature* target);
		virtual void onAttacked();
		virtual void onAttackedCreatureDrain(Creature* target, int32_t points);
		virtual void onSummonAttackedCreatureDrain(Creature* summon, Creature* target, int32_t points);
		virtual void onTargetCreatureGainHealth(Creature* target, int32_t points);
		virtual bool onKilledCreature(Creature* target, uint32_t& flags);
		virtual void onGainExperience(double& gainExp, bool fromMonster, bool multiplied);
		virtual void onAttackedCreatureBlockHit(Creature* target, BlockType_t blockType);
		virtual void onBlockHit(BlockType_t blockType);
		virtual void onChangeZone(ZoneType_t zone);
		virtual void onAttackedCreatureChangeZone(ZoneType_t zone);
		virtual void onIdleStatus();
		virtual void onPlacedCreature();

		virtual void getCreatureLight(LightInfo& light) const;
		Skulls_t getSkull() const;
		Skulls_t getSkullType(const Creature* creature) const;

		bool hasAttacked(const Player* attacked) const;
		void addAttacked(const Player* attacked);
		void clearAttacked() {attackedSet.clear();}

		time_t getSkullEnd() const {return skullEnd;}
		void setSkullEnd(time_t _time, bool login, Skulls_t _skull);

		bool addOutfit(uint32_t outfitId, uint32_t addons);
		bool removeOutfit(uint32_t outfitId, uint32_t addons);

		bool canWearOutfit(uint32_t outfitId, uint32_t addons);
		bool canLogout(bool checkInfight);

		//tile
		//send methods
		void sendAddTileItem(const Tile* tile, const Position& pos, const Item* item)
			{if(client) client->sendAddTileItem(tile, pos, item);}
		void sendUpdateTileItem(const Tile* tile, const Position& pos, const Item* oldItem, const Item* newItem)
			{if(client) client->sendUpdateTileItem(tile, pos, tile->getClientIndexOfThing(this, oldItem), newItem);}
		void sendRemoveTileItem(const Tile* tile, const Position& pos, uint32_t stackpos, const Item* item)
			{if(client) client->sendRemoveTileItem(tile, pos, stackpos);}
		void sendUpdateTile(const Tile* tile, const Position& pos)
			{if(client) client->sendUpdateTile(tile, pos);}

		void sendChannelMessage(std::string author, std::string text, SpeakClasses type, uint8_t channel)
			{if(client) client->sendChannelMessage(author, text, type, channel);}
		void sendCreatureAppear(const Creature* creature)
			{if(client) client->sendAddCreature(creature, creature->getPosition());}
		void sendCreatureDisappear(const Creature* creature, uint32_t stackpos)
			{if(client) client->sendRemoveCreature(creature, creature->getPosition(), stackpos);}
		void sendCreatureMove(const Creature* creature, const Tile* newTile, const Position& newPos,
			const Tile* oldTile, const Position& oldPos, uint32_t oldStackpos, bool teleport)
			{if(client) client->sendMoveCreature(creature, newTile, newPos, newTile->getClientIndexOfThing(
				this, creature), oldTile, oldPos, oldStackpos, teleport);}

		void sendCreatureTurn(const Creature* creature)
			{if(client) client->sendCreatureTurn(creature, creature->getTile()->getClientIndexOfThing(this, creature));}
		void sendCreatureSay(const Creature* creature, SpeakClasses type, const std::string& text, Position* pos = NULL)
			{if(client) client->sendCreatureSay(creature, type, text, pos);}
		void sendCreatureSquare(const Creature* creature, SquareColor_t color)
			{if(client) client->sendCreatureSquare(creature, color);}
		void sendCreatureChangeOutfit(const Creature* creature, const Outfit_t& outfit)
			{if(client) client->sendCreatureOutfit(creature, outfit);}
		void sendCreatureChangeVisible(const Creature* creature, Visible_t visible);
		void sendCreatureLight(const Creature* creature)
			{if(client) client->sendCreatureLight(creature);}
		void sendCreatureShield(const Creature* creature)
			{if(client) client->sendCreatureShield(creature);}

		//container
		void sendAddContainerItem(const Container* container, const Item* item);
		void sendUpdateContainerItem(const Container* container, uint8_t slot, const Item* oldItem, const Item* newItem);
		void sendRemoveContainerItem(const Container* container, uint8_t slot, const Item* item);
		void sendContainer(uint32_t cid, const Container* container, bool hasParent)
			{if(client) client->sendContainer(cid, container, hasParent);}

		//inventory
		void sendAddInventoryItem(slots_t slot, const Item* item)
			{if(client) client->sendAddInventoryItem(slot, item);}
		void sendUpdateInventoryItem(slots_t slot, const Item* oldItem, const Item* newItem)
			{if(client) client->sendUpdateInventoryItem(slot, newItem);}
		void sendRemoveInventoryItem(slots_t slot, const Item* item)
			{if(client) client->sendRemoveInventoryItem(slot);}

		//event methods
		virtual void onUpdateTileItem(const Tile* tile, const Position& pos, const Item* oldItem,
			const ItemType& oldType, const Item* newItem, const ItemType& newType);
		virtual void onRemoveTileItem(const Tile* tile, const Position& pos,
			const ItemType& iType, const Item* item);

		virtual void onCreatureAppear(const Creature* creature);
		virtual void onCreatureDisappear(const Creature* creature, bool isLogout);
		virtual void onCreatureMove(const Creature* creature, const Tile* newTile, const Position& newPos,
			const Tile* oldTile, const Position& oldPos, bool teleport);

		virtual void onAttackedCreatureDisappear(bool isLogout);
		virtual void onFollowCreatureDisappear(bool isLogout);

		//cylinder implementations
		virtual Cylinder* getParent() {return Creature::getParent();}
		virtual const Cylinder* getParent() const {return Creature::getParent();}
		virtual bool isRemoved() const {return Creature::isRemoved();}
		virtual Position getPosition() const {return Creature::getPosition();}
		virtual Tile* getTile() {return Creature::getTile();}
		virtual const Tile* getTile() const {return Creature::getTile();}
		virtual Item* getItem() {return NULL;}
		virtual const Item* getItem() const {return NULL;}
		virtual Creature* getCreature() {return this;}
		virtual const Creature* getCreature() const {return this;}

		//container
		void onAddContainerItem(const Container* container, const Item* item);
		void onUpdateContainerItem(const Container* container, uint8_t slot,
			const Item* oldItem, const ItemType& oldType, const Item* newItem, const ItemType& newType);
		void onRemoveContainerItem(const Container* container, uint8_t slot, const Item* item);

		void onCloseContainer(const Container* container);
		void onSendContainer(const Container* container);
		void autoCloseContainers(const Container* container);

		//inventory
		void onAddInventoryItem(slots_t slot, Item* item) {}
		void onUpdateInventoryItem(slots_t slot, Item* oldItem, const ItemType& oldType,
			Item* newItem, const ItemType& newType);
		void onRemoveInventoryItem(slots_t slot, Item* item);

		void sendAnimatedText(const Position& pos, uint8_t color, std::string text) const
			{if(client) client->sendAnimatedText(pos,color,text);}
		void sendCancel(const std::string& msg) const
			{if(client) client->sendCancel(msg);}
		void sendCancelMessage(ReturnValue message) const;
		void sendCancelTarget() const
			{if(client) client->sendCancelTarget();}
		void sendCancelWalk() const
			{if(client) client->sendCancelWalk();}
		void sendChangeSpeed(const Creature* creature, uint32_t newSpeed) const
			{if(client) client->sendChangeSpeed(creature, newSpeed);}
		void sendCreatureHealth(const Creature* creature) const
			{if(client) client->sendCreatureHealth(creature);}
		void sendDistanceShoot(const Position& from, const Position& to, uint8_t type) const
			{if(client) client->sendDistanceShoot(from, to, type);}
		void sendHouseWindow(House* house, uint32_t listId) const;
		void sendOutfitWindow() const {if(client) client->sendOutfitWindow();}
		void sendQuests() const {if(client) client->sendQuests();}
		void sendQuestInfo(Quest* quest) const {if(client) client->sendQuestInfo(quest);}
		void sendCreatureSkull(const Creature* creature) const
			{if(client) client->sendCreatureSkull(creature);}
		void sendFYIBox(std::string message)
			{if(client) client->sendFYIBox(message);}
		void sendCreatePrivateChannel(uint16_t channelId, const std::string& channelName)
			{if(client) client->sendCreatePrivateChannel(channelId, channelName);}
		void sendClosePrivate(uint16_t channelId) const
			{if(client) client->sendClosePrivate(channelId);}
		void sendIcons() const;
		void sendMagicEffect(const Position& pos, uint8_t type) const
			{if(client) client->sendMagicEffect(pos, type);}
		void sendStats();
		void sendSkills() const
			{if(client) client->sendSkills();}
		void sendTextMessage(MessageClasses type, const std::string& message) const
			{if(client) client->sendTextMessage(type, message);}
		void sendReLoginWindow() const
			{if(client) client->sendReLoginWindow();}
		void sendTextWindow(Item* item, uint16_t maxLen, bool canWrite) const
			{if(client) client->sendTextWindow(windowTextId, item, maxLen, canWrite);}
		void sendTextWindow(uint32_t itemId, const std::string& text) const
			{if(client) client->sendTextWindow(windowTextId, itemId, text);}
		void sendToChannel(Creature* creature, SpeakClasses type, const std::string& text, uint16_t channelId, uint32_t time = 0) const
			{if(client) client->sendToChannel(creature, type, text, channelId, time);}
		void sendTradeItemRequest(const Player* player, const Item* item, bool ack) const
			{if(client) client->sendTradeItemRequest(player, item, ack);}
		void sendTradeClose() const
			{if(client) client->sendCloseTrade();}
		void sendWorldLight(LightInfo& lightInfo)
			{if(client) client->sendWorldLight(lightInfo);}
		void sendChannelsDialog()
			{if(client) client->sendChannelsDialog();}
		void sendOpenPrivateChannel(const std::string& receiver)
			{if(client) client->sendOpenPrivateChannel(receiver);}
		void sendOutfitWindow()
			{if(client) client->sendOutfitWindow();}
		void sendCloseContainer(uint32_t cid)
			{if(client) client->sendCloseContainer(cid);}
		void sendChannel(uint16_t channelId, const std::string& channelName)
			{if(client) client->sendChannel(channelId, channelName);}
		void sendRuleViolationsChannel(uint16_t channelId)
			{if(client) client->sendRuleViolationsChannel(channelId);}
		void sendRemoveReport(const std::string& name)
			{if(client) client->sendRemoveReport(name);}
		void sendLockRuleViolation()
			{if(client) client->sendLockRuleViolation();}
		void sendRuleViolationCancel(const std::string& name)
			{if(client) client->sendRuleViolationCancel(name);}
		void sendAddMarker(const Position& pos, MapMarks_t markType, const std::string& desc)
			{if (client) client->sendAddMarker(pos, markType, desc);}
		void sendCritical() const;

		void receivePing() {lastPong = OTSYS_TIME();}
		virtual void onThink(uint32_t interval);
		uint32_t getAttackSpeed();

		void setLastMail(uint64_t v) {lastMail = v;}
		uint16_t getMailAttempts() const {return mailAttempts;}
		void addMailAttempt() {++mailAttempts;}

		virtual void postAddNotification(Creature* actor, Thing* thing, const Cylinder* oldParent,
			int32_t index, cylinderlink_t link = LINK_OWNER);
		virtual void postRemoveNotification(Creature* actor, Thing* thing, const Cylinder* newParent,
			int32_t index, bool isCompleteRemoval, cylinderlink_t link = LINK_OWNER);

		void setNextAction(int64_t time) {if(time > nextAction) {nextAction = time;}}
		bool canDoAction() const {return nextAction <= OTSYS_TIME();}
		uint32_t getNextActionTime() const;

		Item* getWriteItem(uint32_t& _windowTextId, uint16_t& _maxWriteLen);
		void setWriteItem(Item* item, uint16_t _maxWriteLen = 0);

		House* getEditHouse(uint32_t& _windowTextId, uint32_t& _listId);
		void setEditHouse(House* house, uint32_t listId = 0);

		void learnInstantSpell(const std::string& name);
		void unlearnInstantSpell(const std::string& name);
		bool hasLearnedInstantSpell(const std::string& name) const;

		VIPListSet VIPList;
		ContainerVector containerVec;
		InvitedToGuildsList invitedToGuildsList;
		ConditionList storedConditionList;
		DepotMap depots;

		uint32_t marriage;
		uint64_t balance;
		double rates[SKILL__LAST + 1];
		Container transferContainer;

	protected:
		void checkTradeState(const Item* item);

		bool gainExperience(double& gainExp, bool fromMonster);
		bool rateExperience(double& gainExp, bool fromMonster);
		void updateBaseSpeed()
		{
			if(!hasFlag(PlayerFlag_SetMaxSpeed))
				baseSpeed = vocation->getBaseSpeed() + (2 * (level - 1));
			else
				baseSpeed = SPEED_MAX;
		}

		void updateInventoryWeight();
		void updateItemsLight(bool internal = false);

		void setNextWalkActionTask(SchedulerTask* task);
		void setNextWalkTask(SchedulerTask* task);
		void setNextActionTask(SchedulerTask* task);

		virtual bool onDeath();
		virtual Item* createCorpse(DeathList deathList);

		virtual void dropCorpse(DeathList deathList);
		virtual void dropLoot(Container* corpse);

		//cylinder implementations
		virtual ReturnValue __queryAdd(int32_t index, const Thing* thing, uint32_t count,
			uint32_t flags, Creature* actor = NULL) const;
		virtual ReturnValue __queryMaxCount(int32_t index, const Thing* thing, uint32_t count, uint32_t& maxQueryCount,
			uint32_t flags) const;
		virtual ReturnValue __queryRemove(const Thing* thing, uint32_t count, uint32_t flags, Creature* actor = NULL) const;
		virtual Cylinder* __queryDestination(int32_t& index, const Thing* thing, Item** destItem,
			uint32_t& flags);

		virtual void __addThing(Creature* actor, Thing* thing);
		virtual void __addThing(Creature* actor, int32_t index, Thing* thing);

		virtual void __updateThing(Thing* thing, uint16_t itemId, uint32_t count);
		virtual void __replaceThing(uint32_t index, Thing* thing);

		virtual void __removeThing(Thing* thing, uint32_t count);

		virtual Thing* __getThing(uint32_t index) const;
		virtual int32_t __getIndexOfThing(const Thing* thing) const;
		virtual int32_t __getFirstIndex() const;
		virtual int32_t __getLastIndex() const;
		virtual uint32_t __getItemTypeCount(uint16_t itemId, int32_t subType = -1,
			bool itemCount = true) const;
		virtual std::map<uint32_t, uint32_t>& __getAllItemTypeCount(std::map<uint32_t,
			uint32_t>& countMap, bool itemCount = true) const;

		virtual void __internalAddThing(Thing* thing);
		virtual void __internalAddThing(uint32_t index, Thing* thing);

		uint32_t getVocAttackSpeed() const {return vocation->getAttackSpeed();}
		virtual int32_t getStepSpeed() const
		{
			if(getSpeed() > SPEED_MAX)
				return SPEED_MAX;

			if(getSpeed() < SPEED_MIN)
				return SPEED_MIN;

			return getSpeed();
		}

		virtual uint32_t getDamageImmunities() const {return damageImmunities;}
		virtual uint32_t getConditionImmunities() const {return conditionImmunities;}
		virtual uint32_t getConditionSuppressions() const {return conditionSuppressions;}

		virtual uint16_t getLookCorpse() const;
		virtual uint64_t getLostExperience() const;

		virtual void getPathSearchParams(const Creature* creature, FindPathParams& fpp) const;
		static uint32_t getPercentLevel(uint64_t count, uint64_t nextLevelCount);

		bool isPromoted(uint32_t pLevel = 1) const {return promotionLevel >= pLevel;}
		bool hasCapacity(const Item* item, uint32_t count) const;

	private:
		bool inventoryAbilities[11];
		bool pzLocked;
		bool saving;
		bool isConnecting;
		bool requestedOutfit;
		bool outfitAttributes;
		bool addAttackSkillPoint;

		OperatingSystem_t operatingSystem;
		AccountManager_t accountManager;
		BlockType_t lastAttackBlockType;
		chaseMode_t chaseMode;
		fightMode_t fightMode;
		secureMode_t secureMode;
		tradestate_t tradeState;
		GuildLevel_t guildLevel;

		int16_t blessings;
		uint16_t maxWriteLen;
		uint16_t sex;
		uint16_t mailAttempts;

		int32_t premiumDays;
		int32_t soul;
		int32_t soulMax;
		int32_t vocation_id;
		int32_t groupId;
		int32_t varSkills[SKILL_LAST + 1];
		int32_t varStats[STAT_LAST + 1];
		int32_t messageBuffer;
		int32_t bloodHitCount;
		int32_t shieldBlockCount;
		int32_t shootRange;

		uint32_t clientVersion;
		uint32_t messageTicks;
		uint32_t idleTime;
		uint32_t accountId;
		uint32_t lastIP;
		uint32_t level;
		uint32_t levelPercent;
		uint32_t magLevel;
		uint32_t magLevelPercent;
		uint32_t damageImmunities;
		uint32_t conditionImmunities;
		uint32_t conditionSuppressions;
		uint32_t condition; //?
		uint32_t nextStepEvent;
		uint32_t actionTaskEvent;
		uint32_t walkTaskEvent;
		uint32_t lossPercent[LOSS_LAST + 1];
		uint32_t skills[SKILL_LAST + 1][3];
		uint32_t guid;
		uint32_t editListId;
		uint32_t windowTextId;
		uint32_t guildId;
		uint32_t rankId;
		uint32_t promotionLevel;
		uint32_t town;

		time_t skullEnd;
		time_t lastLogin;
		time_t lastLogout;
		int64_t lastLoad;
		int64_t lastPong;
		int64_t lastPing;
		int64_t nextAction;
		uint64_t stamina;
		uint64_t experience;
		uint64_t manaSpent;
		uint64_t lastAttack;
		uint64_t lastMail;

		double inventoryWeight;
		double capacity;

		std::string password;
		std::string name, nameDescription, specialDescription;
		std::string guildName, rankName, guildNick;

		Position loginPosition;
		LightInfo itemsLight;
		std::pair<Container*, int32_t> backpack;

		Vocation* vocation;
		ProtocolGame* client;
		SchedulerTask* walkTask;
		Party* party;
		Group* group;
		Item* inventory[11];
		Player* tradePartner;
		Item* tradeItem;
		Item* writeItem;
		House* editHouse;

		typedef std::set<uint32_t> AttackedSet;
		AttackedSet attackedSet;
		PartyList invitePartyList;
		OutfitMap outfits;
		LearnedInstantSpellList learnedInstantSpellList;
		
		bool talkState[13];
		int32_t newVocation;
		PlayerSex_t _newSex;
		uint32_t realAccount, newAccount;
		char newAccountNumber[10];
		std::string newPassword, newCharacterName, removeChar, accountNumberAttempt, recoveryKeyAttempt, namelockedPlayer, recoveryKey;

		friend class Game;
		friend class LuaInterface;
		friend class Npc;
		friend class Map;
		friend class Actions;
		friend class IOLoginData;
		friend class ProtocolGame;
};
#endif
