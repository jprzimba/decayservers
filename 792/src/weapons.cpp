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
#include "otpch.h"

#include "definitions.h"
#include "weapons.h"
#include "combat.h"
#include "tools.h"
#include "configmanager.h"

extern Game g_game;
extern Vocations g_vocations;
extern ConfigManager g_config;
extern Weapons* g_weapons;

Weapons::Weapons():
m_scriptInterface("Weapon Interface")
{
	m_scriptInterface.initState();
}

Weapons::~Weapons()
{
	clear();
}

const Weapon* Weapons::getWeapon(const Item* item) const
{
	if(!item)
		return NULL;

	WeaponMap::const_iterator it = weapons.find(item->getID());
	if(it != weapons.end())
		return it->second;

	return NULL;
}

void Weapons::clear()
{
	for(WeaponMap::iterator it = weapons.begin(); it != weapons.end(); ++it)
		delete it->second;

	weapons.clear();
	m_scriptInterface.reInitState();
}

LuaScriptInterface& Weapons::getScriptInterface()
{
	return m_scriptInterface;
}

std::string Weapons::getScriptBaseName()
{
	return "weapons";
}

bool Weapons::loadDefaults()
{
	for(uint32_t i = 0; i < Item::items.size(); ++i)
	{
		const ItemType* it = Item::items.getElement(i);
		if(!it || weapons.find(it->id) != weapons.end())
			continue;

		if(it->weaponType != WEAPON_NONE)
		{
			switch(it->weaponType)
			{
				case WEAPON_AXE:
				case WEAPON_SWORD:
				case WEAPON_CLUB:
				{
					if(WeaponMelee* weapon = new WeaponMelee(&m_scriptInterface))
					{
						weapon->configureWeapon(*it);
						weapons[it->id] = weapon;
					}

					break;
				}

				case WEAPON_AMMO:
				case WEAPON_DIST:
				{
					if(it->weaponType == WEAPON_DIST && it->ammoType != AMMO_NONE)
						continue;

					if(WeaponDistance* weapon = new WeaponDistance(&m_scriptInterface))
					{
						weapon->configureWeapon(*it);
						weapons[it->id] = weapon;
					}

					break;
				}
				default:
					break;
			}
		}
	}

	return true;
}

Event* Weapons::getEvent(const std::string& nodeName)
{
	std::string tmpNodeName = asLowerCaseString(nodeName);
	if(tmpNodeName == "melee")
		return new WeaponMelee(&m_scriptInterface);

	if(tmpNodeName == "distance" || tmpNodeName == "ammunition")
		return new WeaponDistance(&m_scriptInterface);

	if(tmpNodeName == "wand" || tmpNodeName == "rod")
		return new WeaponWand(&m_scriptInterface);

	return NULL;
}

bool Weapons::registerEvent(Event* event, const pugi::xml_node& node)
{
	Weapon* weapon = dynamic_cast<Weapon*>(event);
	if(!weapon)
		return false;

	WeaponMap::iterator it = weapons.find(weapon->getID());
	if(it == weapons.end())
	{
		weapons[weapon->getID()] = weapon;
		return true;
	}

	std::clog << "[Warning - Weapons::registerEvent] Duplicate registered item with id: " << weapon->getID() << std::endl;
	return false;
}

//monsters
int32_t Weapons::getMaxMeleeDamage(int32_t attackSkill, int32_t attackValue)
{
	return ((int32_t)std::ceil((attackSkill * (attackValue * 0.05)) + (attackValue * 0.5)));
}

//players
int32_t Weapons::getMaxWeaponDamage(int32_t attackSkill, int32_t attackValue, float attackFactor)
{
	return ((int32_t)std::ceil(((float)(attackSkill * (attackValue * 0.0425) + (attackValue * 0.2)) / attackFactor)) * 2);
}

Weapon::Weapon(LuaScriptInterface* _interface) :
	Event(_interface)
{
	m_scripted = false;
	id = 0;
	level = 0;
	magLevel = 0;
	mana = 0;
	manaPercent = 0;
	soul = 0;
	exhaustion = 0;
	premium = false;
	enabled = true;
	wieldUnproperly = false;
	range = 1;
	ammoAction = AMMOACTION_NONE;
}

Weapon::~Weapon()
{
	//
}

void Weapon::setCombatParam(const CombatParams& _params)
{
	params = _params;
}

bool Weapon::configureEvent(const pugi::xml_node& node)
{
	pugi::xml_attribute attr;
	if(!(attr = node.attribute("id")))
	{
		std::clog << "[Error - Weapon::configureEvent] Weapon without id." << std::endl;
		return false;
	}

	id = pugi::cast<uint16_t>(attr.value());
	if((attr = node.attribute("level")))
		level = pugi::cast<int32_t>(attr.value());

	if((attr = node.attribute("maglv")) || (attr = node.attribute("maglevel")))
		magLevel = pugi::cast<int32_t>(attr.value());

	if((attr = node.attribute("mana")))
		mana = pugi::cast<int32_t>(attr.value());

	if((attr = node.attribute("manapercent")))
		manaPercent = pugi::cast<int32_t>(attr.value());

	if((attr = node.attribute("soul")))
		soul = pugi::cast<int32_t>(attr.value());

	if((attr = node.attribute("exhaustion")))
		exhaustion = pugi::cast<uint32_t>(attr.value());

	if((attr = node.attribute("prem")))
		premium = attr.as_bool();

	if((attr = node.attribute("enabled")))
		enabled = attr.as_bool();

	if((attr = node.attribute("unproperly")))
		wieldUnproperly = attr.as_bool();

	std::list<std::string> vocStringList;
	for(pugi::xml_node vocationNode = node.first_child(); vocationNode; vocationNode = vocationNode.next_sibling())
	{
		if(!(attr = vocationNode.attribute("name")))
			continue;

		int32_t vocationId = g_vocations.getVocationId(attr.as_string());
		if(vocationId != -1)
		{
			vocWeaponMap[vocationId] = true;
			int32_t promotedVocation = g_vocations.getPromotedVocation(vocationId);
			if(promotedVocation != 0)
				vocWeaponMap[promotedVocation] = true;

			if(vocationNode.attribute("showInDescription").as_bool(true))
				vocStringList.push_back(asLowerCaseString(attr.as_string()));
		}
	}

	range = Item::items[id].shootRange;
	std::string vocationString;
	for(const std::string& str : vocStringList)
	{
		if(!vocationString.empty())
		{
			if(str != vocStringList.back())
				vocationString += ", ";
			else
				vocationString += " and ";
		}

		vocationString += str;
		vocationString += "s";
	}

	uint32_t wieldInfo = 0;
	if(getReqLevel() > 0)
		wieldInfo |= WIELDINFO_LEVEL;

	if(getReqMagLv() > 0)
		wieldInfo |= WIELDINFO_MAGLV;

	if(!vocationString.empty())
		wieldInfo |= WIELDINFO_VOCREQ;

	if(isPremium())
		wieldInfo |= WIELDINFO_PREMIUM;

	if(wieldInfo != 0)
	{
		ItemType& it = Item::items.getItemType(id);
		it.wieldInfo = wieldInfo;
		it.vocationString = vocationString;
		it.minReqLevel = getReqLevel();
		it.minReqMagicLevel = getReqMagLv();
	}
	return configureWeapon(Item::items[getID()]);
}

bool Weapon::loadFunction(const std::string& functionName)
{
	std::string tmpFunctionName = asLowerCaseString(functionName);
	if(tmpFunctionName == "internalloadweapon" || tmpFunctionName == "default")
	{
		if(configureWeapon(Item::items[getID()]))
			return true;
	}
	else if(tmpFunctionName == "script")
		m_scripted = true;

	return false;
}

bool Weapon::configureWeapon(const ItemType& it)
{
	id = it.id;
	return true;
}

std::string Weapon::getScriptEventName()
{
	return "onUseWeapon";
}

int32_t Weapon::playerWeaponCheck(Player* player, Creature* target) const
{
	const Position& playerPos = player->getPosition();
	const Position& targetPos = target->getPosition();

	if(playerPos.z != targetPos.z)
		return 0;

	int32_t trueRange;
	const ItemType& it = Item::items[getID()];
	if(it.weaponType == WEAPON_AMMO)
		trueRange = player->getShootRange();
	else
		trueRange = range;

	if(std::max<uint32_t>(std::abs(playerPos.x - targetPos.x), std::abs(playerPos.y - targetPos.y)) > trueRange)
		return 0;

	if(!player->hasFlag(PlayerFlag_IgnoreWeaponCheck))
	{
		if(!enabled)
			return 0;

		if(player->getMana() < getManaCost(player))
			return 0;

		if(player->getPlayerInfo(PLAYERINFO_SOUL) < soul)
			return 0;

		if(isPremium() && !player->isPremium())
			return 0;

		if(!vocWeaponMap.empty())
		{
			if(vocWeaponMap.find(player->getVocationId()) == vocWeaponMap.end())
				return 0;
		}

		int32_t damageModifier = 100;
		if(player->getLevel() < getReqLevel())
			damageModifier = (isWieldedUnproperly() ? damageModifier / 2 : 0);
		if(player->getMagicLevel() < getReqMagLv())
			damageModifier = (isWieldedUnproperly() ? damageModifier / 2 : 0);
		return damageModifier;
	}
	return 100;
}

bool Weapon::useWeapon(Player* player, Item* item, Creature* target) const
{
	int32_t damageModifier = playerWeaponCheck(player, target);
	if(damageModifier == 0)
		return false;
	return internalUseWeapon(player, item, target, damageModifier);
}

bool Weapon::useFist(Player* player, Creature* target)
{
	const Position& playerPos = player->getPosition();
	const Position& targetPos = target->getPosition();
	if(Position::areInRange<1,1>(playerPos, targetPos))
	{
		float attackFactor = player->getAttackFactor();
		int32_t attackSkill = player->getSkill(SKILL_FIST, SKILL_LEVEL);
		int32_t attackValue = 7;

		int32_t maxDamage = Weapons::getMaxWeaponDamage(attackSkill, attackValue, attackFactor);
		if(random_range(1, 100) <= g_config.getNumber(ConfigManager::CRITICAL_HIT_CHANCE))
			maxDamage <<= 1;
		int32_t damage = -random_range(0, maxDamage, DISTRO_NORMAL);

		CombatParams params;
		params.combatType = COMBAT_PHYSICALDAMAGE;
		params.blockedByArmor = true;
		params.blockedByShield = true;
		Combat::doCombatHealth(player, target, damage, damage, params);

		if(!player->hasFlag(PlayerFlag_NotGainSkill) && player->getAddAttackSkill())
			player->addSkillAdvance(SKILL_FIST, 1);

		return true;
	}

	return false;
}

bool Weapon::internalUseWeapon(Player* player, Item* item, Creature* target, int32_t damageModifier) const
{
	if(m_scripted)
	{
		LuaVariant var;
		var.type = VARIANT_NUMBER;
		var.number = target->getID();
		executeUseWeapon(player, var);
	}
	else
	{
		int32_t damage = (getWeaponDamage(player, target, item) * damageModifier) / 100;
		Combat::doCombatHealth(player, target, damage, damage, params);
	}

	onUsedAmmo(player, item, target->getTile());
	onUsedWeapon(player, item, target->getTile());
	return true;
}

bool Weapon::internalUseWeapon(Player* player, Item* item, Tile* tile) const
{
	if(m_scripted)
	{
		LuaVariant var;
		var.type = VARIANT_TARGETPOSITION;
		var.pos = tile->getPosition();
		executeUseWeapon(player, var);
	}
	else
	{
		Combat::postCombatEffects(player, tile->getPosition(), params);
		g_game.addMagicEffect(tile->getPosition(), NM_ME_POFF);
	}

	onUsedAmmo(player, item, tile);
	onUsedWeapon(player, item, tile);
	return true;
}

void Weapon::onUsedWeapon(Player* player, Item* item, Tile* destTile) const
{
	if(!player->hasFlag(PlayerFlag_NotGainSkill))
	{
		skills_t skillType;
		uint32_t skillPoint = 0;
		if(getSkillType(player, item, skillType, skillPoint))
			player->addSkillAdvance(skillType, skillPoint);
	}

	if(!player->hasFlag(PlayerFlag_HasNoExhaustion) && exhaustion > 0)
		player->addWeaponExhaust(exhaustion);

	int32_t manaCost = getManaCost(player);
	if(manaCost > 0)
	{
		player->addManaSpent(manaCost);
		player->changeMana(-manaCost);
	}

	if(!player->hasFlag(PlayerFlag_HasInfiniteSoul) && soul > 0)
		player->changeSoul(-soul);
}

void Weapon::onUsedAmmo(Player* player, Item* item, Tile* destTile) const
{
	if(g_config.getBool(ConfigManager::REMOVE_AMMO))
	{
		if(ammoAction == AMMOACTION_REMOVECOUNT)
		{
			int32_t newCount = std::max<int32_t>(0, item->getItemCount() - 1);
			g_game.transformItem(item, item->getID(), newCount);
		}
		else if(ammoAction == AMMOACTION_REMOVECHARGE)
		{
			int32_t newCharge = std::max<int32_t>((int32_t)0, ((int32_t)item->getCharges()) - 1);
			g_game.transformItem(item, item->getID(), newCharge);
		}
		else if(ammoAction == AMMOACTION_MOVE)
			g_game.internalMoveItem(item->getParent(), destTile, INDEX_WHEREEVER, item, 1, NULL, FLAG_NOLIMIT);
		else if(ammoAction == AMMOACTION_MOVEBACK){ /* do nothing */ }
		else if(item->hasCharges())
		{
			int32_t newCharge = std::max<int32_t>((int32_t)0, ((int32_t)item->getCharges()) - 1);
			g_game.transformItem(item, item->getID(), newCharge);
		}
	}
}

int32_t Weapon::getManaCost(const Player* player) const
{
	if(mana != 0)
		return mana;

	if(manaPercent != 0)
	{
		int32_t maxMana = player->getMaxMana();
		int32_t manaCost = (maxMana * manaPercent) / 100;
		return manaCost;
	}
	return 0;
}

bool Weapon::executeUseWeapon(Player* player, const LuaVariant& var) const
{
	//onUseWeapon(cid, var)
	if(m_scriptInterface->reserveScriptEnv())
	{
		ScriptEnvironment* env = m_scriptInterface->getScriptEnv();
	
		env->setScriptId(m_scriptId, m_scriptInterface);
		env->setRealPos(player->getPosition());
	
		lua_State* L = m_scriptInterface->getLuaState();
	
		uint32_t cid = env->addThing(player);

		m_scriptInterface->pushFunction(m_scriptId);
		lua_pushnumber(L, cid);
		m_scriptInterface->pushVariant(L, var);

		bool result = m_scriptInterface->callFunction(2) != 0;
		m_scriptInterface->releaseScriptEnv();
		
		return result;
	}
	else
	{
		std::clog << "[Error] Call stack overflow. Weapon::executeUseWeapon" << std::endl;
		return false;
	}
}

WeaponMelee::WeaponMelee(LuaScriptInterface* _interface) :
	Weapon(_interface)
{
	params.blockedByArmor = true;
	params.blockedByShield = true;
	params.combatType = COMBAT_PHYSICALDAMAGE;
	elementType = COMBAT_NONE;
	elementDamage = 0;
}

bool WeaponMelee::configureEvent(const pugi::xml_node& node)
{
	if(!Weapon::configureEvent(node))
		return false;

	return true;
}

bool WeaponMelee::configureWeapon(const ItemType& it)
{
	if(it.abilities)
	{
		elementType = it.abilities->elementType;
		elementDamage = it.abilities->elementDamage;
	}
	return Weapon::configureWeapon(it);
}

bool WeaponMelee::useWeapon(Player* player, Item* item, Creature* target) const
{
	if(!Weapon::useWeapon(player, item, target))
		return false;

	if(elementDamage != 0)
	{
		int32_t damage = getElementDamage(player, item);
		CombatParams eParams;
		eParams.combatType = elementType;
		eParams.isAggressive = true;
		eParams.useCharges = true;
		Combat::doCombatHealth(player, target, damage, damage, eParams);
	}
	return true;
}

void WeaponMelee::onUsedWeapon(Player* player, Item* item, Tile* destTile) const
{
	Weapon::onUsedWeapon(player, item, destTile);
}

void WeaponMelee::onUsedAmmo(Player* player, Item* item, Tile* destTile) const
{
	Weapon::onUsedAmmo(player, item, destTile);
}

bool WeaponMelee::getSkillType(const Player* player, const Item* item,
	skills_t& skill, uint32_t& skillpoint) const
{
	skillpoint = 0;

	if(player->getAddAttackSkill())
	{
		switch(player->getLastAttackBlockType())
		{
			case BLOCK_DEFENSE:
			case BLOCK_ARMOR:
			case BLOCK_NONE:
			{
				skillpoint = 1;
				break;
			}

			default:
			{
				skillpoint = 0;
				break;
			}
		}
	}

	WeaponType_t weaponType = item->getWeaponType();

	switch(weaponType)
	{
		case WEAPON_SWORD:
		{
			skill = SKILL_SWORD;
			return true;
			break;
		}

		case WEAPON_CLUB:
		{
			skill = SKILL_CLUB;
			return true;
			break;
		}

		case WEAPON_AXE:
		{
			skill = SKILL_AXE;
			return true;
			break;
		}
		
		default:
			return false;
			break;
	}
}

int32_t WeaponMelee::getElementDamage(const Player* player, const Item* item) const
{
	int32_t attackSkill = player->getWeaponSkill(item);
	float attackFactor = player->getAttackFactor();
	int32_t maxValue = Weapons::getMaxWeaponDamage(attackSkill, elementDamage, attackFactor);
	return -random_range(0, maxValue, DISTRO_NORMAL);
}

int32_t WeaponMelee::getWeaponDamage(const Player* player, const Creature* target, const Item* item, bool maxDamage /*= false*/) const
{
	int32_t attackSkill = player->getWeaponSkill(item);
	int32_t attackValue = std::max<int32_t>((int32_t)0, ((int32_t)item->getAttack() - elementDamage));
	float attackFactor = player->getAttackFactor();

	int32_t maxValue = Weapons::getMaxWeaponDamage(attackSkill, attackValue, attackFactor);
	if(random_range(1, 100) <= g_config.getNumber(ConfigManager::CRITICAL_HIT_CHANCE))
		maxValue <<= 1;

	Vocation* vocation = player->getVocation();
	if(vocation && vocation->meleeDamageMultipler != 1.0)
		maxValue = int32_t(maxValue * vocation->meleeDamageMultipler);

	if(maxDamage)
		return -maxValue;

	return -random_range(0, maxValue, DISTRO_NORMAL);
}

WeaponDistance::WeaponDistance(LuaScriptInterface* _interface) :
	Weapon(_interface)
{
	hitChance = -1;
	maxHitChance = 0;
	breakChance = 0;
	ammuAttackValue = 0;
	params.blockedByArmor = true;
	params.combatType = COMBAT_PHYSICALDAMAGE;
}

bool WeaponDistance::configureEvent(const pugi::xml_node& node)
{
	if(!Weapon::configureEvent(node))
		return false;

	const ItemType& it = Item::items[id];

	//default values
	if(it.ammoType != AMMO_NONE)
	{
		//hit chance on two-handed weapons is limited to 90%
		maxHitChance = 90;
	}
	else
	{
		//one-handed is set to 75%
		maxHitChance = 75;
	}

	if(it.hitChance != -1)
		hitChance = it.hitChance;

	if(it.maxHitChance != -1)
		maxHitChance = it.maxHitChance;

	if(it.breakChance != -1)
		breakChance = it.breakChance;

	if(it.ammoAction != AMMOACTION_NONE)
		ammoAction = it.ammoAction;

	return true;
}

bool WeaponDistance::configureWeapon(const ItemType& it)
{
	//default values
	if(it.ammoType != AMMO_NONE)
	{
		//hit chance on two-handed weapons is limited to 90%
		maxHitChance = 90;
	}
	else
	{
		//one-handed is set to 75%
		maxHitChance = 75;
	}

	params.distanceEffect = it.shootType;
	range = it.shootRange;
	ammuAttackValue = it.attack;

	if(it.hitChance > 0)
		hitChance = it.hitChance;

	if(it.maxHitChance > 0)
		maxHitChance = it.maxHitChance;

	if(it.breakChance > 0)
		breakChance = it.breakChance;

	if(it.ammoAction != AMMOACTION_NONE)
		ammoAction = it.ammoAction;

	return Weapon::configureWeapon(it);
}

int32_t WeaponDistance::playerWeaponCheck(Player* player, Creature* target) const
{
	Item* bow = player->getWeapon(true);
	if(bow && bow->getWeaponType() == WEAPON_DIST && bow->getID() != id)
	{
		const Weapon* weap = g_weapons->getWeapon(bow);
		if(weap)
			return weap->playerWeaponCheck(player, target);
	}
	return Weapon::playerWeaponCheck(player, target);
}

bool WeaponDistance::useWeapon(Player* player, Item* item, Creature* target) const
{
	int32_t damageModifier = playerWeaponCheck(player, target);
	if(damageModifier == 0)
		return false;

	int32_t chance;
	if(hitChance == -1)
	{
		//hit chance is based on distance to target and distance skill
		uint32_t skill = player->getSkill(SKILL_DIST, SKILL_LEVEL);
		const Position& playerPos = player->getPosition();
		const Position& targetPos = target->getPosition();
		uint32_t distance = std::max<uint32_t>(std::abs(playerPos.x - targetPos.x), std::abs(playerPos.y - targetPos.y));

		if(maxHitChance == 75)
		{
			//chance for one-handed weapons
			switch(distance)
			{
				case 1: chance = (uint32_t)((float)std::min<uint32_t>(skill, (uint32_t)74)) + 1; break;
				case 2: chance = (uint32_t)((float)2.4 * std::min<uint32_t>(skill, (uint32_t)28)) + 8; break;
				case 3: chance = (uint32_t)((float)1.55 * std::min<uint32_t>(skill, (uint32_t)45)) + 6; break;
				case 4: chance = (uint32_t)((float)1.25 * std::min<uint32_t>(skill, (uint32_t)58)) + 3; break;
				case 5: chance = (uint32_t)((float)std::min<uint32_t>(skill, (uint32_t)74)) + 1; break;
				case 6: chance = (uint32_t)((float)0.8 * std::min<uint32_t>(skill, (uint32_t)90)) + 3; break;
				case 7: chance = (uint32_t)((float)0.7 * std::min<uint32_t>(skill, (uint32_t)104)) + 2; break;
				default: chance = hitChance; break;
			}
		}
		else if(maxHitChance == 90)
		{
			//formula for two-handed weapons
			switch(distance)
			{
				case 1: chance = (uint32_t)((float)1.2 * std::min<uint32_t>(skill, (uint32_t)74)) + 1; break;
				case 2: chance = (uint32_t)((float)3.2 * std::min<uint32_t>(skill, (uint32_t)28)); break;
				case 3: chance = (uint32_t)((float)2.0 * std::min<uint32_t>(skill, (uint32_t)45)); break;
				case 4: chance = (uint32_t)((float)1.55 * std::min<uint32_t>(skill, (uint32_t)58)); break;
				case 5: chance = (uint32_t)((float)1.2 * std::min<uint32_t>(skill, (uint32_t)74)) + 1; break;
				case 6: chance = (uint32_t)((float)1.0 * std::min<uint32_t>(skill, (uint32_t)90)); break;
				case 7: chance = (uint32_t)((float)1.0 * std::min<uint32_t>(skill, (uint32_t)90)); break;
				default: chance = hitChance; break;
			}
		}
		else if(maxHitChance == 100)
		{
			switch(distance)
			{
				case 1: chance = (uint32_t)((float)1.35 * std::min<uint32_t>(skill, (uint32_t)73)) + 1; break;
				case 2: chance = (uint32_t)((float)3.2 * std::min<uint32_t>(skill, (uint32_t)30)) + 4; break;
				case 3: chance = (uint32_t)((float)2.05 * std::min<uint32_t>(skill, (uint32_t)48)) + 2; break;
				case 4: chance = (uint32_t)((float)1.5 * std::min<uint32_t>(skill, (uint32_t)65)) + 2; break;
				case 5: chance = (uint32_t)((float)1.35 * std::min<uint32_t>(skill, (uint32_t)73)) + 1; break;
				case 6: chance = (uint32_t)((float)1.2 * std::min<uint32_t>(skill, (uint32_t)87)) - 4; break;
				case 7: chance = (uint32_t)((float)1.1 * std::min<uint32_t>(skill, (uint32_t)90)) + 1; break;
				default: chance = hitChance; break;
			}
		}
		else
			chance = maxHitChance;
	}
	else
		chance = hitChance;

	Item* bow = player->getWeapon(true);
	if(bow && bow != item && bow->getHitChance() > 0)
		chance += bow->getHitChance();

	if(chance >= random_range(1, 100))
		Weapon::internalUseWeapon(player, item, target, damageModifier);
	else
	{
		//miss target
		typedef std::pair<int32_t, int32_t> dPair;
		std::vector<dPair> destList;
		destList.push_back(dPair(-1, -1));
		destList.push_back(dPair(-1, 0));
		destList.push_back(dPair(-1, 1));
		destList.push_back(dPair(0, -1));
		destList.push_back(dPair(0, 1));
		destList.push_back(dPair(1, -1));
		destList.push_back(dPair(1, 0));
		destList.push_back(dPair(1, 1));

		std::random_shuffle(destList.begin(), destList.end());

		Position destPos = target->getPosition();
		Tile* destTile = target->getTile();
		Tile* tmpTile = NULL;

		for(std::vector<dPair>::iterator it = destList.begin(); it != destList.end(); ++it)
		{
			tmpTile = g_game.getTile(destPos.x + it->first, destPos.y + it->second, destPos.z);
			if(tmpTile && !tmpTile->hasProperty(IMMOVABLEBLOCKSOLID))
			{
				destTile = tmpTile;
				break;
			}
		}

		Weapon::internalUseWeapon(player, item, destTile);
	}

	return true;
}

void WeaponDistance::onUsedWeapon(Player* player, Item* item, Tile* destTile) const
{
	Weapon::onUsedWeapon(player, item, destTile);
}

void WeaponDistance::onUsedAmmo(Player* player, Item* item, Tile* destTile) const
{
	if(ammoAction == AMMOACTION_MOVEBACK && breakChance > 0 && random_range(1, 100) < breakChance)
	{
		int32_t newCount = std::max<int32_t>(0, item->getItemCount() - 1);
		g_game.transformItem(item, item->getID(), newCount);
	}
	else
		Weapon::onUsedAmmo(player, item, destTile);
}

int32_t WeaponDistance::getWeaponDamage(const Player* player, const Creature* target, const Item* item, bool maxDamage /*= false*/) const
{
	int32_t attackValue = ammuAttackValue;
	if(item->getWeaponType() == WEAPON_AMMO)
	{
		Item* bow = const_cast<Player*>(player)->getWeapon(true);
		if(bow)
			attackValue += bow->getAttack();
	}

	int32_t attackSkill = player->getSkill(SKILL_DIST, SKILL_LEVEL);
	float attackFactor = player->getAttackFactor();

	int32_t maxValue = Weapons::getMaxWeaponDamage(attackSkill, attackValue, attackFactor);
	if(random_range(1, 100) <= g_config.getNumber(ConfigManager::CRITICAL_HIT_CHANCE))
		maxValue <<= 1;

	Vocation* vocation = player->getVocation();
	if(vocation && vocation->distDamageMultipler != 1.0)
		maxValue = int32_t(maxValue * vocation->distDamageMultipler);

	if(maxDamage)
		return -maxValue;

	int32_t minValue = 0;
	if(target)
	{
		if(target->getPlayer())
			minValue = (int32_t)std::ceil(player->getLevel() * 0.1);
		else
			minValue = (int32_t)std::ceil(player->getLevel() * 0.2);
	}

	return -random_range(minValue, maxValue, DISTRO_NORMAL);
}

bool WeaponDistance::getSkillType(const Player* player, const Item* item,
	skills_t& skill, uint32_t& skillpoint) const
{
	skill = SKILL_DIST;
	skillpoint = 0;

	if(player->getAddAttackSkill())
	{
		switch(player->getLastAttackBlockType())
		{
			case BLOCK_NONE:
			{
				skillpoint = 2;
				break;
			}

			case BLOCK_DEFENSE:
			case BLOCK_ARMOR:
			{
				skillpoint = 1;
				break;
			}

			default:
				skillpoint = 0;
				break;
		}
	}
	return true;
}

WeaponWand::WeaponWand(LuaScriptInterface* _interface) :
	Weapon(_interface)
{
	minChange = 0;
	maxChange = 0;
}

bool WeaponWand::configureEvent(const pugi::xml_node& node)
{
	if(!Weapon::configureEvent(node)) {
		return false;
	}

	pugi::xml_attribute attr;
	if((attr = node.attribute("min"))) {
		minChange = pugi::cast<int32_t>(attr.value());
	}

	if((attr = node.attribute("max"))) {
		maxChange = pugi::cast<int32_t>(attr.value());
	}

	if((attr = node.attribute("type"))) {
		std::string tmpStrValue = asLowerCaseString(attr.as_string());
		if(tmpStrValue == "earth" || tmpStrValue == "poison") {
			params.combatType = COMBAT_POISONDAMAGE;
		} else if(tmpStrValue == "energy") {
			params.combatType = COMBAT_ENERGYDAMAGE;
		} else if(tmpStrValue == "fire") {
			params.combatType = COMBAT_FIREDAMAGE;
		} else {
			std::clog << "[Warning - WeaponWand::configureEvent] Type \"" << attr.as_string() << "\" does not exist." << std::endl;
		}
	}

	return true;
}

bool WeaponWand::configureWeapon(const ItemType& it)
{
	range = it.shootRange;
	params.distanceEffect = it.shootType;

	return Weapon::configureWeapon(it);
}

int32_t WeaponWand::getWeaponDamage(const Player* player, const Creature* target, const Item* item, bool maxDamage /*= false*/) const
{
	if(maxDamage)
		return -maxChange;

	return random_range(-minChange, -maxChange, DISTRO_NORMAL);
}
