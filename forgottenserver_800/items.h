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

#ifndef __ITEMS__
#define __ITEMS__
#include "otsystem.h"
#include "itemloader.h"

#include "const.h"
#include "enums.h"

#include "position.h"
#include <libxml/parser.h>

#define ITEMS 11500
#define SLOTP_WHEREEVER 0xFFFFFFFF
#define SLOTP_HEAD 1 << 0
#define	SLOTP_NECKLACE 1 << 1
#define	SLOTP_BACKPACK 1 << 2
#define	SLOTP_ARMOR 1 << 3
#define	SLOTP_RIGHT 1 << 4
#define	SLOTP_LEFT 1 << 5
#define	SLOTP_LEGS 1 << 6
#define	SLOTP_FEET 1 << 7
#define	SLOTP_RING 1 << 8
#define	SLOTP_AMMO 1 << 9
#define	SLOTP_DEPOT 1 << 10
#define	SLOTP_TWO_HAND 1 << 11
#define SLOTP_HAND SLOTP_LEFT | SLOTP_RIGHT

enum ItemTypes_t
{
	ITEM_TYPE_NONE = 0,
	ITEM_TYPE_DEPOT,
	ITEM_TYPE_MAILBOX,
	ITEM_TYPE_TRASHHOLDER,
	ITEM_TYPE_CONTAINER,
	ITEM_TYPE_DOOR,
	ITEM_TYPE_MAGICFIELD,
	ITEM_TYPE_TELEPORT,
	ITEM_TYPE_BED,
	ITEM_TYPE_KEY,
	ITEM_TYPE_LAST
};

enum FloorChange_t
{
	CHANGE_PRE_FIRST = 0,
	CHANGE_DOWN = CHANGE_PRE_FIRST,
	CHANGE_FIRST = 1,
	CHANGE_NORTH = CHANGE_FIRST,
	CHANGE_EAST = 2,
	CHANGE_SOUTH = 3,
	CHANGE_WEST = 4,
	CHANGE_FIRST_EX = 5,
	CHANGE_NORTH_EX = CHANGE_FIRST_EX,
	CHANGE_EAST_EX = 6,
	CHANGE_SOUTH_EX = 7,
	CHANGE_WEST_EX = 8,
	CHANGE_NONE = 9,
	CHANGE_LAST = CHANGE_NONE
};

struct Abilities
{
	Abilities()
	{
		memset(skills, 0, sizeof(skills));
		memset(skillsPercent, 0, sizeof(skillsPercent));
		memset(stats, 0 , sizeof(stats));
		memset(statsPercent, 0, sizeof(statsPercent));

		memset(absorb, 0, sizeof(absorb));
		memset(increment, 0, sizeof(increment));
		memset(reflect[REFLECT_PERCENT], 0, sizeof(reflect[REFLECT_PERCENT]));
		memset(reflect[REFLECT_CHANCE], 0, sizeof(reflect[REFLECT_CHANCE]));

		elementType = COMBAT_NONE;
		manaShield = invisible = regeneration = preventLoss = preventDrop = false;
		speed = healthGain = healthTicks = manaGain = manaTicks = elementDamage = conditionSuppressions = 0;
	};

	bool manaShield, invisible, regeneration, preventLoss, preventDrop;
	CombatType_t elementType;

	int16_t elementDamage, absorb[COMBAT_LAST + 1], increment[INCREMENT_LAST + 1], reflect[REFLECT_LAST + 1][COMBAT_LAST + 1];
	int32_t skills[SKILL_LAST + 1], skillsPercent[SKILL_LAST + 1], stats[STAT_LAST + 1], statsPercent[STAT_LAST + 1],
		speed, healthGain, healthTicks, manaGain, manaTicks, conditionSuppressions;
};

class Condition;
class ItemType
{
	private:
		ItemType(const ItemType& it) {}

	public:
		ItemType();
		virtual ~ItemType();

		bool isGroundTile() const {return (group == ITEM_GROUP_GROUND);}
		bool isContainer() const {return (group == ITEM_GROUP_CONTAINER);}
		bool isSplash() const {return (group == ITEM_GROUP_SPLASH);}
		bool isFluidContainer() const {return (group == ITEM_GROUP_FLUID);}

		bool isDoor() const {return (type == ITEM_TYPE_DOOR);}
		bool isMagicField() const {return (type == ITEM_TYPE_MAGICFIELD);}
		bool isTeleport() const {return (type == ITEM_TYPE_TELEPORT);}
		bool isKey() const {return (type == ITEM_TYPE_KEY);}
		bool isDepot() const {return (type == ITEM_TYPE_DEPOT);}
		bool isMailbox() const {return (type == ITEM_TYPE_MAILBOX);}
		bool isTrashHolder() const {return (type == ITEM_TYPE_TRASHHOLDER);}
		bool isBed() const {return (type == ITEM_TYPE_BED);}

		bool isRune() const {return (group == ITEM_GROUP_RUNE);}
		bool hasSubType() const {return (isFluidContainer() || isSplash() || stackable || charges);}

		bool stopTime, showCount, stackable, showDuration, showCharges, showAttributes,
			allowDistRead, canReadText, canWriteText, forceSerialize, isVertical, isHorizontal, isHangable,
			useable, moveable, pickupable, rotable, replaceable, lookThrough,
			hasHeight, blockSolid, blockPickupable, blockProjectile, blockPathFind, allowPickupable, alwaysOnTop,
			floorChange[CHANGE_LAST];

		MagicEffect_t magicEffect;
		FluidTypes_t fluidSource;
		WeaponType_t weaponType;
		Direction bedPartnerDir;
		AmmoAction_t ammoAction;
		CombatType_t combatType;
		RaceType_t corpseType;
		ShootEffect_t shootType;
		Ammo_t ammoType;

		uint16_t transformUseTo[2], transformToFree, transformEquipTo, transformDeEquipTo,
			id, clientId, maxItems, slotPosition, wieldPosition, speed, maxTextLen, writeOnceItemId;

		int32_t attack, extraAttack, defense, extraDefense, armor, breakChance, hitChance, maxHitChance,
			runeLevel, runeMagLevel, lightLevel, lightColor, decayTo, rotateTo, alwaysOnTopOrder;
		uint32_t shootRange, charges, decayTime, attackSpeed, wieldInfo, minReqLevel, minReqMagicLevel,
			worth, levelDoor;

		std::string name, pluralName, article, description, runeSpellName, vocationString;

		Condition* condition;
		Abilities abilities;
		itemgroup_t group;
		ItemTypes_t type;
		float weight;
};

template<typename A>
class Array
{
	public:
		Array(uint32_t n);
		virtual ~Array();

		A getElement(uint32_t id);
		const A getElement(uint32_t id) const;
		void addElement(A a, uint32_t pos);

		uint32_t size() {return m_size;}

	private:
		A* m_data;
		uint32_t m_size;
};

template<typename A>
Array<A>::Array(uint32_t n)
{
	m_data = (A*)malloc(sizeof(A) * n);
	memset(m_data, 0, sizeof(A) * n);
	m_size = n;
}

template<typename A>
Array<A>::~Array()
{
	free(m_data);
}

template<typename A>
A Array<A>::getElement(uint32_t id)
{
	if(id < m_size)
		return m_data[id];

	return 0;
}

template<typename A>
const A Array<A>::getElement(uint32_t id) const
{
	if(id < m_size)
		return m_data[id];

	return 0;
}

template<typename A>
void Array<A>::addElement(A a, uint32_t pos)
{
	#define INCREMENT 5000
	if(pos >= m_size)
	{
		m_data = (A*)realloc(m_data, sizeof(A) * (pos + INCREMENT));
		memset(m_data + m_size, 0, sizeof(A) * (pos + INCREMENT - m_size));
		m_size = pos + INCREMENT;
	}

	m_data[pos] = a;
}

typedef std::map<int32_t, int32_t> IntegerMap;

class Items
{
	public:
		Items(): items(ITEMS) {}
		virtual ~Items() {clear();}

		bool reload();
		int32_t loadFromOtb(std::string);
		bool loadFromXml();
		void parseItemNode(xmlNodePtr itemNode, uint32_t id);

		void addItemType(ItemType* iType);
		ItemType& getItemType(int32_t id);
		const ItemType& getItemType(int32_t id) const;
		const ItemType& operator[](int32_t id) const {return getItemType(id);}

		int32_t getItemIdByName(const std::string& name);
		const ItemType& getItemIdByClientId(int32_t spriteId) const;

		uint32_t size() {return items.size();}
		IntegerMap getMoneyMap() {return moneyMap;}
		const ItemType* getElement(uint32_t id) const {return items.getElement(id);}

		static uint32_t dwMajorVersion;
		static uint32_t dwMinorVersion;
		static uint32_t dwBuildNumber;

	private:
		void clear();

		Array<ItemType*> items;

		IntegerMap moneyMap;
		IntegerMap reverseItemMap;
};
#endif
