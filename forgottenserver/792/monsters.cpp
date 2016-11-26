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

#include "monsters.h"
#include "monster.h"
#include "container.h"
#include "tools.h"
#include "spells.h"
#include "combat.h"
#include "luascript.h"
#include "weapons.h"
#include "configmanager.h"
#include "game.h"

extern Spells* g_spells;
extern Monsters g_monsters;
extern ConfigManager g_config;
extern Game g_game;

MonsterType::MonsterType()
{
	reset();
}

void MonsterType::reset()
{
	experience = 0;

	defense = 0;
	armor = 0;

	canPushItems = false;
	canPushCreatures = false;
	staticAttackChance = 95;
	maxSummons = 0;
	targetDistance = 1;
	runAwayHealth = 0;
	pushable = true;
	base_speed = 200;
	health = 100;
	health_max = 100;

	outfit.lookHead = 0;
	outfit.lookBody = 0;
	outfit.lookLegs = 0;
	outfit.lookFeet = 0;
	outfit.lookType = 0;
	outfit.lookTypeEx = 0;
	outfit.lookAddons = 0;
	lookcorpse = 0;

	conditionImmunities = 0;
	damageImmunities = 0;
	race = RACE_BLOOD;
	isSummonable = false;
	isIllusionable = false;
	isConvinceable = false;
	isAttackable = true;
	isHostile = true;

	lightLevel = 0;
	lightColor = 0;

	manaCost = 0;
	summonList.clear();
	lootItems.clear();
	elementMap.clear();

	for(SpellList::iterator it = spellAttackList.begin(); it != spellAttackList.end(); ++it)
	{
		if(it->combatSpell)
		{
			delete it->spell;
			it->spell = nullptr;
		}
	}

	spellAttackList.clear();

	for(SpellList::iterator it = spellDefenseList.begin(); it != spellDefenseList.end(); ++it)
	{
		if(it->combatSpell)
		{
			delete it->spell;
			it->spell = nullptr;
		}
	}

	spellDefenseList.clear();

	yellSpeedTicks = 0;
	yellChance = 0;
	voiceVector.clear();

	changeTargetSpeed = 0;
	changeTargetChance = 0;

	scriptList.clear();
}

MonsterType::~MonsterType()
{
	reset();
}

uint32_t Monsters::getLootRandom()
{
	return random_range(0, MAX_LOOTCHANCE)/g_config.getNumber(ConfigManager::RATE_LOOT);
}

void MonsterType::createLoot(Container* corpse)
{
	if(g_config.getNumber(ConfigManager::RATE_LOOT) == 0)
	{
		corpse->__startDecaying();
		return;
	}
		
	Player* owner = g_game.getPlayerByID(corpse->getCorpseOwner());
	if(owner)
	{
		for(LootItems::const_iterator it = lootItems.begin(); it != lootItems.end() && (corpse->capacity() - corpse->size() > 0); it++)
		{
			Item* tmpItem = createLootItem(*it);
			if(tmpItem)
			{
				//check containers
				if(Container* container = tmpItem->getContainer())
				{
					createLootContainer(container, *it);
					if(container->size() == 0)
						delete container;
					else
						corpse->__internalAddThing(tmpItem);
				}
				else
					corpse->__internalAddThing(tmpItem);
			}
		}
	}

	corpse->__startDecaying();
}

void MonsterType::createSurpriseBag(Container* corpse, std::string name)
{
	if(!g_config.getBool(ConfigManager::SURPRISE_BAGS))
        return;

	MonsterType* mType = g_monsters.getMonsterType(name);
	if(corpse->capacity() - corpse->size() > 0){
		int32_t surpriseBagId = random_range(6570, 6571);
		if(random_range(1,100) <= g_config.getNumber(ConfigManager::SUPRISEBAG_PERCENT)){
			corpse->__internalAddThing(Item::CreateItem(surpriseBagId, 1));
		}
	}
}

Item* MonsterType::createLootItem(const LootBlock& lootBlock)
{
	Item* tmpItem = nullptr;
	if(Item::items[lootBlock.id].stackable)
	{
		uint32_t randvalue = Monsters::getLootRandom();
		if(randvalue < lootBlock.chance)
		{
			uint32_t n = randvalue % lootBlock.countmax + 1;
			tmpItem = Item::CreateItem(lootBlock.id, n);
		}
	}
	else
	{
		if(Monsters::getLootRandom() < lootBlock.chance)
			tmpItem = Item::CreateItem(lootBlock.id, 0);
	}

	if(tmpItem)
	{
		if(lootBlock.subType != -1)
			tmpItem->setSubType(lootBlock.subType);

		if(lootBlock.actionId != -1)
			tmpItem->setActionId(lootBlock.actionId);

		if(lootBlock.text != "")
			tmpItem->setText(lootBlock.text);

		return tmpItem;
	}

	return nullptr;
}

void MonsterType::createLootContainer(Container* parent, const LootBlock& lootblock)
{
	if(parent->size() < parent->capacity())
	{
		LootItems::const_iterator it;
		for(it = lootblock.childLoot.begin(); it != lootblock.childLoot.end(); it++)
		{
			Item* tmpItem = createLootItem(*it);
			if(tmpItem)
			{
				if(Container* container = tmpItem->getContainer())
				{
					createLootContainer(container, *it);
					if(container->size() == 0)
						delete container;
					else
						parent->__internalAddThing(container);
				}
				else
					parent->__internalAddThing(tmpItem);
			}
		}
	}
}

Monsters::Monsters()
{
	loaded = false;
}

bool Monsters::loadFromXml(bool reloading /*= false*/)
{
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file("data/monster/monsters.xml");
	if(!result) {
		std::cout << "[Error - Monsters::loadFromXml] Failed to load data/monster/monsters.xml: " << result.description() << std::endl;
		return false;
	}

	loaded = true;

	for(pugi::xml_node monsterNode = doc.child("monsters").first_child(); monsterNode; monsterNode = monsterNode.next_sibling()) {
		loadMonster("data/monster/" + std::string(monsterNode.attribute("file").as_string()), monsterNode.attribute("name").as_string(), reloading);
	}
	return true;
}

bool Monsters::reload()
{
	loaded = false;
	return loadFromXml(true);
}

ConditionDamage* Monsters::getDamageCondition(ConditionType_t conditionType,
	int32_t maxDamage, int32_t minDamage, int32_t startDamage, uint32_t tickInterval)
{
	ConditionDamage* condition = static_cast<ConditionDamage*>(Condition::createCondition(CONDITIONID_COMBAT, conditionType, 0, 0));
	condition->setParam(CONDITIONPARAM_TICKINTERVAL, tickInterval);
	condition->setParam(CONDITIONPARAM_MINVALUE, minDamage);
	condition->setParam(CONDITIONPARAM_MAXVALUE, maxDamage);
	condition->setParam(CONDITIONPARAM_STARTVALUE, startDamage);
	condition->setParam(CONDITIONPARAM_DELAYED, 1);
	return condition;
}

bool Monsters::deserializeSpell(const pugi::xml_node& node, spellBlock_t& sb, const std::string& description)
{
	sb.chance = 100;
	sb.speed = 2000;
	sb.range = 0;
	sb.minCombatValue = 0;
	sb.maxCombatValue = 0;
	sb.combatSpell = false;
	sb.isMelee = false;

	std::string name;
	std::string scriptName;
	bool isScripted = false;

	pugi::xml_attribute attr;
	if((attr = node.attribute("script"))) {
		scriptName = attr.as_string();
		isScripted = true;
	} else if((attr = node.attribute("name"))) {
		name = attr.as_string();
	} else {
		return false;
	}

	if((attr = node.attribute("speed")) || (attr = node.attribute("interval"))) {
		sb.speed = std::max<int32_t>(1, pugi::cast<int32_t>(attr.value()));
	}

	if((attr = node.attribute("chance"))) {
		uint32_t chance = pugi::cast<uint32_t>(attr.value());
		if(chance > 100) {
			chance = 100;
		}
		sb.chance = chance;
	}

	if((attr = node.attribute("range"))) {
		uint32_t range = pugi::cast<uint32_t>(attr.value());
		if(range > (Map::maxViewportX * 2)) {
			range = Map::maxViewportX * 2;
		}
		sb.range = range;
	}

	if((attr = node.attribute("min"))) {
		sb.minCombatValue = pugi::cast<int32_t>(attr.value());
	}

	if((attr = node.attribute("max"))) {
		sb.maxCombatValue = pugi::cast<int32_t>(attr.value());

		//normalize values
		if(std::abs(sb.minCombatValue) > std::abs(sb.maxCombatValue)) {
			int32_t value = sb.maxCombatValue;
			sb.maxCombatValue = sb.minCombatValue;
			sb.minCombatValue = value;
		}
	}

	sb.spell = g_spells->getSpellByName(name);
	if(sb.spell) {
		return true;
	}

	CombatSpell* combatSpell = nullptr;
	bool needTarget = false;
	bool needDirection = false;

	if(isScripted) {
		if((attr = node.attribute("direction"))) {
			needDirection = attr.as_bool();
		}

		if((attr = node.attribute("target"))) {
			needTarget = attr.as_bool();
		}

		combatSpell = new CombatSpell(nullptr, needTarget, needDirection);
		if(!combatSpell->loadScript("data/" + g_spells->getScriptBaseName() + "/scripts/" + scriptName)) {
			delete combatSpell;
			return false;
		}

		if(!combatSpell->loadScriptCombat()) {
			delete combatSpell;
			return false;
		}

		combatSpell->getCombat()->setPlayerCombatValues(FORMULA_VALUE, sb.minCombatValue, 0, sb.maxCombatValue, 0);
	} else {
		Combat* combat = new Combat;
		sb.combatSpell = true;

		if((attr = node.attribute("length"))) {
			int32_t length = pugi::cast<int32_t>(attr.value());
			if(length > 0) {
				int32_t spread = 3;

				//need direction spell
				if((attr = node.attribute("spread"))) {
					spread = std::max<int32_t>(0, pugi::cast<int32_t>(attr.value()));
				}

				AreaCombat* area = new AreaCombat();
				area->setupArea(length, spread);
				combat->setArea(area);

				needDirection = true;
			}
		}

		if((attr = node.attribute("radius"))) {
			int32_t radius = pugi::cast<int32_t>(attr.value());

			//target spell
			if((attr = node.attribute("target"))) {
				needTarget = attr.as_bool();
			}

			AreaCombat* area = new AreaCombat();
			area->setupArea(radius);
			combat->setArea(area);
		}

		std::string tmpName = asLowerCaseString(name);

		if(tmpName == "melee") {
			sb.isMelee = true;

			pugi::xml_attribute attackAttribute, skillAttribute;
			if((attackAttribute = node.attribute("attack")) && (skillAttribute = node.attribute("skill"))) {
				sb.minCombatValue = 0;
				sb.maxCombatValue = -Weapons::getMaxMeleeDamage(pugi::cast<int32_t>(skillAttribute.value()), pugi::cast<int32_t>(attackAttribute.value()));
			}

			ConditionType_t conditionType = CONDITION_NONE;
			int32_t minDamage = 0;
			int32_t maxDamage = 0;
			uint32_t tickInterval = 2000;

			if((attr = node.attribute("fire"))) {
				conditionType = CONDITION_FIRE;

				minDamage = pugi::cast<int32_t>(attr.value());
				maxDamage = minDamage;
				tickInterval = 9000;
			} else if((attr = node.attribute("poison"))) {
				conditionType = CONDITION_POISON;

				minDamage = pugi::cast<int32_t>(attr.value());
				maxDamage = minDamage;
				tickInterval = 5000;
			} else if((attr = node.attribute("energy"))) {
				conditionType = CONDITION_ENERGY;

				minDamage = pugi::cast<int32_t>(attr.value());
				maxDamage = minDamage;
				tickInterval = 10000;
			} else if((attr = node.attribute("drown"))) {
				conditionType = CONDITION_DROWN;

				minDamage = pugi::cast<int32_t>(attr.value());
				maxDamage = minDamage;
				tickInterval = 5000;
			} 

			if((attr = node.attribute("tick"))) {
				int32_t value = pugi::cast<int32_t>(attr.value());
				if(value > 0) {
					tickInterval = value;
				}
			}

			if(conditionType != CONDITION_NONE) {
				Condition* condition = getDamageCondition(conditionType, maxDamage, minDamage, 0, tickInterval);
				combat->setCondition(condition);
			}

			sb.range = 1;
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_PHYSICALDAMAGE);
			combat->setParam(COMBATPARAM_BLOCKEDBYARMOR, 1);
			combat->setParam(COMBATPARAM_BLOCKEDBYSHIELD, 1);
		} else if(tmpName == "physical") {
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_PHYSICALDAMAGE);
			combat->setParam(COMBATPARAM_BLOCKEDBYARMOR, 1);
		} else if(tmpName == "bleed") {
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_PHYSICALDAMAGE);
		} else if(tmpName == "poison" || tmpName == "earth") {
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_POISONDAMAGE);
		} else if(tmpName == "fire") {
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_FIREDAMAGE);
		} else if(tmpName == "energy") {
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_ENERGYDAMAGE);
		} else if(tmpName == "drown") {
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_DROWNDAMAGE);
		} else if(tmpName == "lifedrain") {
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_LIFEDRAIN);
		} else if(tmpName == "manadrain") {
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_MANADRAIN);
		} else if(tmpName == "healing") {
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_HEALING);
			combat->setParam(COMBATPARAM_AGGRESSIVE, 0);
		} else if(tmpName == "speed") {
			int32_t speedChange = 0;
			int32_t duration = 10000;

			if((attr = node.attribute("duration"))) {
				duration = pugi::cast<int32_t>(attr.value());
			}

			if((attr = node.attribute("speedchange"))) {
				speedChange = pugi::cast<int32_t>(attr.value());
				if(speedChange < -1000) {
					//cant be slower than 100%
					speedChange = -1000;
				}
			}

			ConditionType_t conditionType;
			if(speedChange > 0) {
				conditionType = CONDITION_HASTE;
				combat->setParam(COMBATPARAM_AGGRESSIVE, 0);
			} else {
				conditionType = CONDITION_PARALYZE;
			}

			ConditionSpeed* condition = dynamic_cast<ConditionSpeed*>(Condition::createCondition(CONDITIONID_COMBAT, conditionType, duration, 0));
			condition->setFormulaVars(speedChange / 1000.0, 0, speedChange / 1000.0, 0);
			combat->setCondition(condition);
		} else if(tmpName == "outfit") {
			int32_t duration = 10000;

			if((attr = node.attribute("duration"))) {
				duration = pugi::cast<int32_t>(attr.value());
			}

			if((attr = node.attribute("monster"))) {
				MonsterType* mType = g_monsters.getMonsterType(attr.as_string());
				if(mType) {
					ConditionOutfit* condition = dynamic_cast<ConditionOutfit*>(Condition::createCondition(CONDITIONID_COMBAT, CONDITION_OUTFIT, duration, 0));
					condition->addOutfit(mType->outfit);
					combat->setParam(COMBATPARAM_AGGRESSIVE, 0);
					combat->setCondition(condition);
				}
			} else if((attr = node.attribute("item"))) {
				Outfit_t outfit;
				outfit.lookTypeEx = pugi::cast<uint16_t>(attr.value());

				ConditionOutfit* condition = dynamic_cast<ConditionOutfit*>(Condition::createCondition(CONDITIONID_COMBAT, CONDITION_OUTFIT, duration, 0));
				condition->addOutfit(outfit);
				combat->setParam(COMBATPARAM_AGGRESSIVE, 0);
				combat->setCondition(condition);
			}
		} else if(tmpName == "invisible") {
			int32_t duration = 10000;

			if((attr = node.attribute("duration"))) {
				duration = pugi::cast<int32_t>(attr.value());
			}

			Condition* condition = Condition::createCondition(CONDITIONID_COMBAT, CONDITION_INVISIBLE, duration, 0);
			combat->setParam(COMBATPARAM_AGGRESSIVE, 0);
			combat->setCondition(condition);
		} else if(tmpName == "drunk") {
			int32_t duration = 10000;

			if((attr = node.attribute("duration"))) {
				duration = pugi::cast<int32_t>(attr.value());
			}

			Condition* condition = Condition::createCondition(CONDITIONID_COMBAT, CONDITION_DRUNK, duration, 0);
			combat->setCondition(condition);
		} else if(tmpName == "firefield") {
			combat->setParam(COMBATPARAM_CREATEITEM, 1492);
		} else if(tmpName == "poisonfield") {
			combat->setParam(COMBATPARAM_CREATEITEM, 1496);
		} else if(tmpName == "energyfield") {
			combat->setParam(COMBATPARAM_CREATEITEM, 1495);
		} else if(tmpName == "firecondition" ||
				tmpName == "poisoncondition" ||
				tmpName == "energycondition" ||
				tmpName == "drowncondition") {
			ConditionType_t conditionType = CONDITION_NONE;
			uint32_t tickInterval = 2000;

			if(tmpName == "firecondition") {
				conditionType = CONDITION_FIRE;
				tickInterval = 10000;
			} else if(tmpName == "poisoncondition" || tmpName == "earthcondition") {
				conditionType = CONDITION_POISON;
				tickInterval = 5000;
			} else if(tmpName == "energycondition") {
				conditionType = CONDITION_ENERGY;
				tickInterval = 10000;
			} else if(tmpName == "drowncondition") {
				conditionType = CONDITION_DROWN;
				tickInterval = 5000;
			}

			if((attr = node.attribute("tick"))) {
				int32_t value = pugi::cast<int32_t>(attr.value());
				if(value > 0) {
					tickInterval = value;
				}
			}

			int32_t minDamage = std::abs(sb.minCombatValue);
			int32_t maxDamage = std::abs(sb.maxCombatValue);
			int32_t startDamage = 0;

			if((attr = node.attribute("start"))) {
				int32_t value = std::abs(pugi::cast<int32_t>(attr.value()));
				if(value <= minDamage) {
					startDamage = value;
				}
			}

			Condition* condition = getDamageCondition(conditionType, maxDamage, minDamage, startDamage, tickInterval);
			combat->setCondition(condition);
		} else if(tmpName == "strength") {
			//
		} else if(tmpName == "effect") {
			//
		} else {
			std::cout << "[Error - Monsters::deserializeSpell] - " << description << " - Unknown spell name: " << name << std::endl;
			delete combat;
			return false;
		}

		combat->setPlayerCombatValues(FORMULA_VALUE, sb.minCombatValue, 0, sb.maxCombatValue, 0);
		combatSpell = new CombatSpell(combat, needTarget, needDirection);

		for(pugi::xml_node attributeNode = node.first_child(); attributeNode; attributeNode = attributeNode.next_sibling()) {
			if((attr = attributeNode.attribute("key"))) {
				std::string tmpStrValue = asLowerCaseString(attr.as_string());
				if(tmpStrValue == "shooteffect") {
					if((attr = attributeNode.attribute("value"))) {
						ShootType_t shoot = getShootType(attr.as_string());
						if(shoot != NM_SHOOT_UNK) {
							combat->setParam(COMBATPARAM_DISTANCEEFFECT, shoot);
						} else {
							std::cout << "[Warning - Monsters::deserializeSpell] " << description << " - Unknown shootEffect: " << attr.as_string() << std::endl;
						}
					}
				} else if(tmpStrValue == "areaeffect") {
					if((attr = attributeNode.attribute("value"))) {
						MagicEffectClasses effect = getMagicEffect(attr.as_string());
						if(effect != NM_ME_UNK) {
							combat->setParam(COMBATPARAM_EFFECT, effect);
						} else {
							std::cout << "[Warning - Monsters::deserializeSpell] " << description << " - Unknown areaEffect: " << attr.as_string() << std::endl;
						}
					}
				} else {
					std::cout << "[Warning - Monsters::deserializeSpells] Effect type \"" << attr.as_string() << "\" does not exist." << std::endl;
				}
			}
		}
	}

	sb.spell = combatSpell;
	return true;
}

#define SHOW_XML_WARNING(desc) std::cout << "[Warning - Monsters::loadMonster] " << desc << ". " << file << std::endl;
#define SHOW_XML_ERROR(desc) std::cout << "[Error - Monsters::loadMonster] " << desc << ". " << file << std::endl;

bool Monsters::loadMonster(const std::string& file, const std::string& monster_name, bool reloading /*= false*/)
{
	MonsterType* mType = nullptr;
	bool new_mType = true;

	if(reloading) {
		uint32_t id = getIdByName(monster_name);
		if(id != 0) {
			mType = getMonsterType(id);
			if(mType != nullptr) {
				new_mType = false;
				mType->reset();
			}
		}
	}

	if(new_mType) {
		mType = new MonsterType();
	}

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(file.c_str());
	if(!result) {
		if(new_mType) {
			delete mType;
		}
		std::cout << "[Error - Monsters::loadMonster] Failed to load " << file << ": " << result.description() << std::endl;
		return false;
	}

	pugi::xml_node monsterNode = doc.child("monster");
	if(!monsterNode) {
		if(new_mType) {
			delete mType;
		}
		std::cout << "[Error - Monsters::loadMonster] Missing monster node in: " << file << std::endl;
		return false;
	}

	pugi::xml_attribute attr;
	if(!(attr = monsterNode.attribute("name"))) {
		if(new_mType) {
			delete mType;
		}
		std::cout << "[Error - Monsters::loadMonster] Missing name in: " << file << std::endl;
		return false;
	}
	mType->name = attr.as_string();

	if((attr = monsterNode.attribute("nameDescription"))) {
		mType->nameDescription = attr.as_string();
	} else {
		mType->nameDescription = "a " + mType->name;
		toLowerCaseString(mType->nameDescription);
	}

	if((attr = monsterNode.attribute("race"))) {
		std::string tmpStrValue = asLowerCaseString(attr.as_string());
		uint16_t tmpInt = pugi::cast<uint16_t>(attr.value());
		if(tmpStrValue == "venom" || tmpInt == 1) {
			mType->race = RACE_VENOM;
		} else if(tmpStrValue == "blood" || tmpInt == 2) {
			mType->race = RACE_BLOOD;
		} else if(tmpStrValue == "undead" || tmpInt == 3) {
			mType->race = RACE_UNDEAD;
		} else if(tmpStrValue == "fire" || tmpInt == 4) {
			mType->race = RACE_FIRE;
		} else {
			SHOW_XML_WARNING("Unknown race type " << attr.as_string());
		}
	}

	if((attr = monsterNode.attribute("experience"))) {
		mType->experience = pugi::cast<uint64_t>(attr.value());
	}

	if((attr = monsterNode.attribute("speed"))) {
		mType->base_speed = pugi::cast<int32_t>(attr.value());
	}

	if((attr = monsterNode.attribute("manacost"))) {
		mType->manaCost = pugi::cast<uint32_t>(attr.value());
	}

	pugi::xml_node node;
	if((node = monsterNode.child("health"))) {
		if((attr = node.attribute("now"))) {
			mType->health = pugi::cast<int32_t>(attr.value());
		} else {
			SHOW_XML_ERROR("Missing health now");
		}

		if((attr = node.attribute("max"))) {
			mType->health_max = pugi::cast<int32_t>(attr.value());
		} else {
			SHOW_XML_ERROR("Missing health max");
		}
	}

	if((node = monsterNode.child("flags"))) {
		for(pugi::xml_node flagNode = node.first_child(); flagNode; flagNode = flagNode.next_sibling()) {
			attr = flagNode.first_attribute();
			const char* attrName = attr.name();
			if(strcasecmp(attrName, "summonable") == 0) {
				mType->isSummonable = attr.as_bool();
			} else if(strcasecmp(attrName, "attackable") == 0) {
				mType->isAttackable = attr.as_bool();
			} else if(strcasecmp(attrName, "hostile") == 0) {
				mType->isHostile = attr.as_bool();
			} else if(strcasecmp(attrName, "illusionable") == 0) {
				mType->isIllusionable = attr.as_bool();
			} else if(strcasecmp(attrName, "convinceable") == 0) {
				mType->isConvinceable = attr.as_bool();
			} else if(strcasecmp(attrName, "pushable") == 0) {
				mType->pushable = attr.as_bool();
			} else if(strcasecmp(attrName, "canpushitems") == 0) {
				mType->canPushItems = attr.as_bool();
			} else if(strcasecmp(attrName, "canpushcreatures") == 0) {
				mType->canPushCreatures = attr.as_bool();
			} else if(strcasecmp(attrName, "staticattack") == 0) {
				uint32_t staticAttack = pugi::cast<uint32_t>(attr.value());
				if(staticAttack > 100) {
					SHOW_XML_WARNING("staticattack greater than 100");
					staticAttack = 100;
				}

				mType->staticAttackChance = staticAttack;
			} else if(strcasecmp(attrName, "lightlevel") == 0) {
				mType->lightLevel = pugi::cast<int32_t>(attr.value());
			} else if(strcasecmp(attrName, "lightcolor") == 0) {
				mType->lightColor = pugi::cast<int32_t>(attr.value());
			} else if(strcasecmp(attrName, "targetdistance") == 0) {
				mType->targetDistance = std::max<int32_t>(1, pugi::cast<int32_t>(attr.value()));
			} else if(strcasecmp(attrName, "runonhealth") == 0) {
				mType->runAwayHealth = pugi::cast<int32_t>(attr.value());
			} else {
				SHOW_XML_WARNING("Unknown flag attribute: " << attrName);
			}
		}

		//if a monster can push creatures,
		// it should not be pushable
		if(mType->canPushCreatures && mType->pushable) {
			mType->pushable = false;
		}
	}

	if((node = monsterNode.child("targetchange"))) {
		if((attr = node.attribute("speed")) || (attr = node.attribute("interval"))) {
			mType->changeTargetSpeed = std::max<int32_t>(1, pugi::cast<int32_t>(attr.value()));
		} else {
			SHOW_XML_WARNING("Missing targetchange speed");
		}

		if((attr = node.attribute("chance"))) {
			mType->changeTargetChance = pugi::cast<int32_t>(attr.value());
		} else {
			SHOW_XML_WARNING("Missing targetchange chance");
		}
	}

	if((node = monsterNode.child("strategy"))) {
		if((attr = node.attribute("attack"))) {
			//mType->attackStrength = pugi::cast<int32_t>(attr.value());
		}

		if((attr = node.attribute("defense"))) {
			//mType->defenseStrength = pugi::cast<int32_t>(attr.value());
		}
	}

	if((node = monsterNode.child("look"))) {
		if((attr = node.attribute("type"))) {
			mType->outfit.lookType = pugi::cast<uint16_t>(attr.value());

			if((attr = node.attribute("head"))) {
				mType->outfit.lookHead = pugi::cast<uint16_t>(attr.value());
			}

			if((attr = node.attribute("body"))) {
				mType->outfit.lookBody = pugi::cast<uint16_t>(attr.value());
			}

			if((attr = node.attribute("legs"))) {
				mType->outfit.lookLegs = pugi::cast<uint16_t>(attr.value());
			}

			if((attr = node.attribute("feet"))) {
				mType->outfit.lookFeet = pugi::cast<uint16_t>(attr.value());
			}

			if((attr = node.attribute("addons"))) {
				mType->outfit.lookAddons = pugi::cast<uint16_t>(attr.value());
			}
		} else if((attr = node.attribute("typeex"))) {
			mType->outfit.lookTypeEx = pugi::cast<uint16_t>(attr.value());
		} else {
			SHOW_XML_WARNING("Missing look type/typeex");
		}

		if((attr = node.attribute("corpse"))) {
			mType->lookcorpse = pugi::cast<uint16_t>(attr.value());
		}
	}

	if((node = monsterNode.child("attacks"))) {
		for(pugi::xml_node attackNode = node.first_child(); attackNode; attackNode = attackNode.next_sibling()) {
			spellBlock_t sb;
			if(deserializeSpell(attackNode, sb, monster_name)) {
				mType->spellAttackList.push_back(sb);
			} else {
				SHOW_XML_WARNING("Cant load spell");
			}
		}
	}

	if((node = monsterNode.child("defenses"))) {
		if((attr = node.attribute("defense"))) {
			mType->defense = pugi::cast<int32_t>(attr.value());
		}

		if((attr = node.attribute("armor"))) {
			mType->armor = pugi::cast<int32_t>(attr.value());
		}

		for(pugi::xml_node defenseNode = node.first_child(); defenseNode; defenseNode = defenseNode.next_sibling()) {
			spellBlock_t sb;
			if(deserializeSpell(defenseNode, sb, monster_name)) {
				mType->spellDefenseList.push_back(sb);
			} else {
				SHOW_XML_WARNING("Cant load spell");
			}
		}
	}

	if((node = monsterNode.child("immunities"))) {
		for(pugi::xml_node immunityNode = node.first_child(); immunityNode; immunityNode = immunityNode.next_sibling()) {
			if((attr = immunityNode.attribute("name"))) {
				std::string tmpStrValue = asLowerCaseString(attr.as_string());
				if(tmpStrValue == "physical") {
					mType->damageImmunities |= COMBAT_PHYSICALDAMAGE;
				} else if(tmpStrValue == "energy") {
					mType->damageImmunities |= COMBAT_ENERGYDAMAGE;
					mType->conditionImmunities |= CONDITION_ENERGY;
				} else if(tmpStrValue == "fire") {
					mType->damageImmunities |= COMBAT_FIREDAMAGE;
					mType->conditionImmunities |= CONDITION_FIRE;
				} else if(tmpStrValue == "poison" ||
							tmpStrValue == "earth") {
					mType->damageImmunities |= COMBAT_POISONDAMAGE;
					mType->conditionImmunities |= CONDITION_POISON;
				} else if(tmpStrValue == "drown") {
					mType->damageImmunities |= COMBAT_DROWNDAMAGE;
					mType->conditionImmunities |= CONDITION_DROWN;
				} else if(tmpStrValue == "lifedrain") {
					mType->damageImmunities |= COMBAT_LIFEDRAIN;
				} else if(tmpStrValue == "paralyze") {
					mType->conditionImmunities |= CONDITION_PARALYZE;
				} else if(tmpStrValue == "outfit") {
					mType->conditionImmunities |= CONDITION_OUTFIT;
				} else if(tmpStrValue == "drunk") {
					mType->conditionImmunities |= CONDITION_DRUNK;
				} else if(tmpStrValue == "invisible" || tmpStrValue == "invisibility") {
					mType->conditionImmunities |= CONDITION_INVISIBLE;
				} else {
					SHOW_XML_WARNING("Unknown immunity name " << attr.as_string());
				}
			} else if((attr = immunityNode.attribute("physical"))) {
				if(attr.as_bool()) {
					mType->damageImmunities |= COMBAT_PHYSICALDAMAGE;
				}
			} else if((attr = immunityNode.attribute("energy"))) {
				if(attr.as_bool()) {
					mType->damageImmunities |= COMBAT_ENERGYDAMAGE;
					mType->conditionImmunities |= CONDITION_ENERGY;
				}
			} else if((attr = immunityNode.attribute("fire"))) {
				if(attr.as_bool()) {
					mType->damageImmunities |= COMBAT_FIREDAMAGE;
					mType->conditionImmunities |= CONDITION_FIRE;
				}
			} else if((attr = immunityNode.attribute("poison")) || (attr = immunityNode.attribute("earth"))) {
				if(attr.as_bool()) {
					mType->damageImmunities |= COMBAT_POISONDAMAGE;
					mType->conditionImmunities |= CONDITION_POISON;
				}
			} else if((attr = immunityNode.attribute("drown"))) {
				if(attr.as_bool()) {
					mType->damageImmunities |= COMBAT_DROWNDAMAGE;
					mType->conditionImmunities |= CONDITION_DROWN;
				}
			} else if((attr = immunityNode.attribute("lifedrain"))) {
				if(attr.as_bool()) {
					mType->damageImmunities |= COMBAT_LIFEDRAIN;
				}
			} else if((attr = immunityNode.attribute("paralyze"))) {
				if(attr.as_bool()) {
					mType->conditionImmunities |= CONDITION_PARALYZE;
				}
			} else if((attr = immunityNode.attribute("outfit"))) {
				if(attr.as_bool()) {
					mType->conditionImmunities |= CONDITION_OUTFIT;
				}
			} else if((attr = immunityNode.attribute("drunk"))) {
				if(attr.as_bool()) {
					mType->conditionImmunities |= CONDITION_DRUNK;
				}
			} else if((attr = immunityNode.attribute("invisible")) || (attr = immunityNode.attribute("invisibility"))) {
				if(attr.as_bool()) {
					mType->conditionImmunities |= CONDITION_INVISIBLE;
				}
			} else {
				SHOW_XML_WARNING("Unknown immunity");
			}
		}
	}

	if((node = monsterNode.child("voices"))) {
		if((attr = node.attribute("speed")) || (attr = node.attribute("interval"))) {
			mType->yellSpeedTicks = pugi::cast<uint32_t>(attr.value());
		} else {
			SHOW_XML_WARNING("Missing voices speed");
		}

		if((attr = node.attribute("chance"))) {
			mType->yellChance = pugi::cast<uint32_t>(attr.value());
		} else {
			SHOW_XML_WARNING("Missing voices chance");
		}

		for(pugi::xml_node voiceNode = node.first_child(); voiceNode; voiceNode = voiceNode.next_sibling()) {
			voiceBlock_t vb;
			if((attr = voiceNode.attribute("sentence"))) {
				vb.text = attr.as_string();
			} else {
				SHOW_XML_WARNING("Missing voice sentence");
			}

			if((attr = voiceNode.attribute("yell"))) {
				vb.yellText = attr.as_bool();
			} else {
				vb.yellText = false;
			}
			mType->voiceVector.push_back(vb);
		}
	}

	if((node = monsterNode.child("loot"))) {
		for(pugi::xml_node lootNode = node.first_child(); lootNode; lootNode = lootNode.next_sibling()) {
			LootBlock lootBlock;
			if(loadLootItem(lootNode, lootBlock)) {
				mType->lootItems.push_back(lootBlock);
			} else {
				SHOW_XML_WARNING("Cant load loot");
			}
		}
	}

	if((node = monsterNode.child("elements"))) {
		for(pugi::xml_node elementNode = node.first_child(); elementNode; elementNode = elementNode.next_sibling()) {
			if((attr = elementNode.attribute("physicalPercent"))) {
				mType->elementMap[COMBAT_PHYSICALDAMAGE] = pugi::cast<int32_t>(attr.value());
			} else if((attr = elementNode.attribute("poisonPercent")) || (attr = elementNode.attribute("earthPercent"))) {
				mType->elementMap[COMBAT_POISONDAMAGE] = pugi::cast<int32_t>(attr.value());
			} else if((attr = elementNode.attribute("firePercent"))) {
				mType->elementMap[COMBAT_FIREDAMAGE] = pugi::cast<int32_t>(attr.value());
			} else if((attr = elementNode.attribute("energyPercent"))) {
				mType->elementMap[COMBAT_ENERGYDAMAGE] = pugi::cast<int32_t>(attr.value());
			} else {
				SHOW_XML_WARNING("Unknown element percent");
			}
		}
	}

	if((node = monsterNode.child("summons"))) {
		if((attr = node.attribute("maxSummons"))) {
			mType->maxSummons = std::min<int32_t>(pugi::cast<int32_t>(attr.value()), 100);
		} else {
			SHOW_XML_WARNING("Missing summons maxSummons");
		}

		for(pugi::xml_node summonNode = node.first_child(); summonNode; summonNode = summonNode.next_sibling()) {
			int32_t chance = 100;
			int32_t speed = 1000;

			if((attr = summonNode.attribute("speed")) || (attr = summonNode.attribute("interval"))) {
				speed = pugi::cast<int32_t>(attr.value());
			}

			if((attr = summonNode.attribute("chance"))) {
				chance = pugi::cast<int32_t>(attr.value());
			}

			if((attr = summonNode.attribute("name"))) {
				summonBlock_t sb;
				sb.name = attr.as_string();
				sb.speed = speed;
				sb.chance = chance;
				mType->summonList.push_back(sb);
			} else {
				SHOW_XML_WARNING("Missing summon name");
			}
		}
	}

	if((node = monsterNode.child("script"))) {
		for(pugi::xml_node eventNode = node.first_child(); eventNode; eventNode = eventNode.next_sibling()) {
			if((attr = eventNode.attribute("name"))) {
				mType->scriptList.push_back(attr.as_string());
			} else {
				SHOW_XML_WARNING("Missing name for script event");
			}
		}
	}

	static uint32_t id = 0;
	if(new_mType) {
		std::string lowername = monster_name;
		toLowerCaseString(lowername);

		monsterNames[lowername] = ++id;
		monsters[id] = mType;
	}
	return true;
}

bool Monsters::loadLootItem(const pugi::xml_node& node, LootBlock& lootBlock)
{
	pugi::xml_attribute attr;
	if((attr = node.attribute("id"))) {
		lootBlock.id = pugi::cast<int32_t>(attr.value());
	}

	if(lootBlock.id == 0) {
		return false;
	}

	if((attr = node.attribute("countmax"))) {
		lootBlock.countmax = std::max<int32_t>(1, pugi::cast<int32_t>(attr.value()));
	} else {
		lootBlock.countmax = 1;
	}

	if((attr = node.attribute("chance")) || (attr = node.attribute("chance1"))) {
		lootBlock.chance = std::min<int32_t>(MAX_LOOTCHANCE, pugi::cast<int32_t>(attr.value()));
	} else {
		lootBlock.chance = MAX_LOOTCHANCE;
	}

	if(Item::items[lootBlock.id].isContainer()) {
		loadLootContainer(node, lootBlock);
	}

	//optional
	if((attr = node.attribute("subtype"))) {
		lootBlock.subType = pugi::cast<int32_t>(attr.value());
	}

	if((attr = node.attribute("actionId"))) {
		lootBlock.actionId = pugi::cast<int32_t>(attr.value());
	}

	if((attr = node.attribute("text"))) {
		lootBlock.text = attr.as_string();
	}
	return true;
}

void Monsters::loadLootContainer(const pugi::xml_node& node, LootBlock& lBlock)
{
	for(pugi::xml_node subNode = node.first_child(); subNode; subNode = subNode.next_sibling()) {
		LootBlock lootBlock;
		if(loadLootItem(subNode, lootBlock)) {
			lBlock.childLoot.push_back(lootBlock);
		}
	}
}

MonsterType* Monsters::getMonsterType(const std::string& name)
{
	uint32_t mId = getIdByName(name);
	if(mId == 0)
		return nullptr;

	return getMonsterType(mId);
}

MonsterType* Monsters::getMonsterType(uint32_t mid)
{
	MonsterMap::iterator it = monsters.find(mid);
	if(it != monsters.end())
		return it->second;
	else
		return nullptr;
}

uint32_t Monsters::getIdByName(const std::string& name)
{
	std::string lower_name = name;
	std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), tolower);
	MonsterNameMap::iterator it = monsterNames.find(lower_name);
	if(it != monsterNames.end())
		return it->second;
	else
		return 0;
}

Monsters::~Monsters()
{
	for(MonsterMap::iterator it = monsters.begin(); it != monsters.end(); it++)
		delete it->second;
}
