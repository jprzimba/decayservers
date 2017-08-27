//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// The database of items.
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

#include "items.h"
#include "spells.h"
#include "condition.h"
#include "weapons.h"

#include <iostream>
#include <string>

uint32_t Items::dwMajorVersion = 0;
uint32_t Items::dwMinorVersion = 0;
uint32_t Items::dwBuildNumber = 0;

extern Spells* g_spells;

ItemType::ItemType()
{
	article = "";
	group = ITEM_GROUP_NONE;
	type = ITEM_TYPE_NONE;
	stackable = false;
	useable	= false;
	moveable = true;
	alwaysOnTop = false;
	alwaysOnTopOrder = 0;
	pickupable = false;
	rotable = false;
	rotateTo = 0;
	hasHeight = false;

	floorChangeDown = true;
	floorChangeNorth = false;
	floorChangeSouth = false;
	floorChangeEast = false;
	floorChangeWest = false;

	blockSolid = false;
	blockProjectile = false;
	blockPathFind = false;

	wieldInfo = 0;
	minReqLevel = 0;
	minReqMagicLevel = 0;

	runeMagLevel = 0;
	runeLevel = 0;

	speed = 0;
	id = 0;
	clientId = 100;
	maxItems = 8;  // maximum size if this is a container
	weight = 0;  // weight of the item, e.g. throwing distance depends on it
	showCount = true;
	weaponType = WEAPON_NONE;
	slot_position = SLOTP_RIGHT | SLOTP_LEFT | SLOTP_AMMO;
	ammoType = AMMO_NONE;
	ammoAction = AMMOACTION_NONE;
	shootType = (ShootType_t)0;
	magicEffect = NM_ME_NONE;
	attack = 0;
	defense = 0;
	extraDefense = 0;
	armor = 0;
	decayTo = -1;
	decayTime = 0;
	stopTime = false;
	corpseType = RACE_NONE;
	fluidSource  = FLUID_NONE;

	allowDistRead = false;

	isVertical = false;
	isHorizontal = false;
	isHangable = false;

	lightLevel = 0;
	lightColor = 0;

	maxTextLen = 0;
	canReadText = false;
	canWriteText = false;
	writeOnceItemId = 0;

	transformEquipTo = 0;
	transformDeEquipTo = 0;
	showDuration = false;
	showCharges = false;
	showAttributes = false;
	charges	= 0;
	hitChance = 0;
	maxHitChance = 0;
	breakChance = 0;
	shootRange = 1;

	condition = NULL;
	combatType = COMBAT_NONE;

	replaceable = true;

	bedPartnerDir = NORTH;
	transformToOnUse[PLAYERSEX_MALE] = 0;
	transformToOnUse[PLAYERSEX_FEMALE] = 0;
	transformToFree = 0;

	levelDoor = 0;
}

ItemType::~ItemType()
{
	delete condition;
}

Items::Items() // : items(8000)
{
	this->items = new Array<ItemType*>(8000);
}

Items::~Items()
{
	clear();
}

void Items::clear()
{
	//TODO. clear items?
}

bool Items::reload()
{
	//TODO?
	return false;
}

int32_t Items::loadFromOtb(std::string file)
{
	FileLoader f;
	if(!f.openFile(file.c_str(), false, true))
		return f.getError();
	
	uint32_t type;
	NODE node = f.getChildNode(NO_NODE, type);
	
	PropStream props;
	if(f.getProps(node,props))
	{
		//4 byte flags
		//attributes
		//0x01 = version data
		uint32_t flags;
		if(!props.GET_ULONG(flags))
			return ERROR_INVALID_FORMAT;
		attribute_t attr;
		if(!props.GET_VALUE(attr))
			return ERROR_INVALID_FORMAT;
		if(attr == ROOT_ATTR_VERSION)
		{
			datasize_t datalen = 0;
			if(!props.GET_VALUE(datalen))
				return ERROR_INVALID_FORMAT;
			if(datalen != sizeof(VERSIONINFO))
				return ERROR_INVALID_FORMAT;
			VERSIONINFO *vi;
			if(!props.GET_STRUCT(vi))
				return ERROR_INVALID_FORMAT;
			Items::dwMajorVersion = vi->dwMajorVersion; //items otb format file version
			Items::dwMinorVersion = vi->dwMinorVersion; //client version
			Items::dwBuildNumber = vi->dwBuildNumber; //revision
		}
	}
	
	if(Items::dwMajorVersion != 2)
	{
		std::clog << "Not supported items.otb version." << std::endl;
		return ERROR_INVALID_FORMAT;
	}
	if(Items::dwMajorVersion == 0xFFFFFFFF)
		std::clog << "[Warning] Items::loadFromOtb items.otb using generic client version." << std::endl;
	else if(Items::dwMinorVersion < CLIENT_VERSION_792)
	{
		std::clog << "Not supported items.otb client version." << std::endl;
		return ERROR_INVALID_FORMAT;
	}
	node = f.getChildNode(node, type);

	while(node != NO_NODE)
	{
		PropStream props;
		if(!f.getProps(node,props))
			return f.getError();
		flags_t flags;
		ItemType* iType = new ItemType();
		iType->group = (itemgroup_t)type;
		switch(type)
		{
			case ITEM_GROUP_CONTAINER:
				iType->type = ITEM_TYPE_CONTAINER;
				break;
			case ITEM_GROUP_DOOR:
				iType->type = ITEM_TYPE_DOOR;
				break;
			case ITEM_GROUP_MAGICFIELD:
				iType->type = ITEM_TYPE_MAGICFIELD;
				break;
			case ITEM_GROUP_TELEPORT:
				iType->type = ITEM_TYPE_TELEPORT;
				break;
			case ITEM_GROUP_NONE:
			case ITEM_GROUP_GROUND:
			case ITEM_GROUP_SPLASH:
			case ITEM_GROUP_FLUID:
			case ITEM_GROUP_RUNE:
			case ITEM_GROUP_DEPRECATED:
				break;
			default:
				return ERROR_INVALID_FORMAT;
				break;
		}
		//read 4 byte flags
		if(!props.GET_VALUE(flags))
			return ERROR_INVALID_FORMAT;

		iType->blockSolid = hasBitSet(FLAG_BLOCK_SOLID, flags);
		iType->blockProjectile = hasBitSet(FLAG_BLOCK_PROJECTILE, flags);
		iType->blockPathFind = hasBitSet(FLAG_BLOCK_PATHFIND, flags);
		iType->hasHeight = hasBitSet(FLAG_HAS_HEIGHT, flags);
		iType->useable = hasBitSet(FLAG_USEABLE, flags);
		iType->pickupable = hasBitSet(FLAG_PICKUPABLE, flags);
		iType->moveable = hasBitSet(FLAG_MOVEABLE, flags);
		iType->stackable = hasBitSet(FLAG_STACKABLE, flags);
		iType->floorChangeDown = hasBitSet(FLAG_FLOORCHANGEDOWN, flags);
		iType->floorChangeNorth = hasBitSet(FLAG_FLOORCHANGENORTH, flags);
		iType->floorChangeEast = hasBitSet(FLAG_FLOORCHANGEEAST, flags);
		iType->floorChangeSouth = hasBitSet(FLAG_FLOORCHANGESOUTH, flags);
		iType->floorChangeWest = hasBitSet(FLAG_FLOORCHANGEWEST, flags);
		iType->alwaysOnTop = hasBitSet(FLAG_ALWAYSONTOP, flags);
		iType->isVertical = hasBitSet(FLAG_VERTICAL, flags);
		iType->isHorizontal = hasBitSet(FLAG_HORIZONTAL, flags);
		iType->isHangable = hasBitSet(FLAG_HANGABLE, flags);
		iType->allowDistRead = hasBitSet(FLAG_ALLOWDISTREAD, flags);
		iType->rotable = hasBitSet(FLAG_ROTABLE, flags);
		iType->canReadText = hasBitSet(FLAG_READABLE, flags);

		attribute_t attrib;
		datasize_t datalen = 0;
		while(props.GET_VALUE(attrib))
		{
			//size of data
			if(!props.GET_VALUE(datalen))
			{
				delete iType;
				return ERROR_INVALID_FORMAT;
			}
			switch(attrib)
			{
				case ITEM_ATTR_SERVERID:
				{
					if(datalen != sizeof(uint16_t))
						return ERROR_INVALID_FORMAT;
					
					uint16_t serverid;
					if(!props.GET_USHORT(serverid))
						return ERROR_INVALID_FORMAT;
					
					if(serverid > 20000 && serverid < 20100)
						serverid = serverid - 20000;
							
					iType->id = serverid;
					break;
				}
				case ITEM_ATTR_CLIENTID:
				{
					if(datalen != sizeof(uint16_t))
						return ERROR_INVALID_FORMAT;

					uint16_t clientid;
					if(!props.GET_USHORT(clientid))
						return ERROR_INVALID_FORMAT;
					
					iType->clientId = clientid;
					break;
				}
				case ITEM_ATTR_SPEED:
				{
					if(datalen != sizeof(uint16_t))
						return ERROR_INVALID_FORMAT;
					
					uint16_t speed;
					if(!props.GET_USHORT(speed))
						return ERROR_INVALID_FORMAT;
					
					iType->speed = speed;

					break;
				}
				case ITEM_ATTR_LIGHT2:
				{
					if(datalen != sizeof(lightBlock2))
						return ERROR_INVALID_FORMAT;

					lightBlock2* lb2;
					if(!props.GET_STRUCT(lb2))
						return ERROR_INVALID_FORMAT;
				
					iType->lightLevel = lb2->lightLevel;
					iType->lightColor = lb2->lightColor;
					break;
				}
				case ITEM_ATTR_TOPORDER:
				{
					if(datalen != sizeof(uint8_t))
						return ERROR_INVALID_FORMAT;
					
					uint8_t v;
					if(!props.GET_UCHAR(v))
						return ERROR_INVALID_FORMAT;
						
					iType->alwaysOnTopOrder = v;
					break;
				}
				default:
				{
					//skip unknown attributes
					if(!props.SKIP_N(datalen))
						return ERROR_INVALID_FORMAT;
					break;
				}
			}
		}
		
		reverseItemMap[iType->clientId] = iType->id;
		
		// store the found item
		items->addElement(iType, iType->id);
		node = f.getNextNode(node, type);
	}
	
	return ERROR_NONE;
}

bool Items::loadFromXml()
{
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file("data/items/items.xml");
	if(!result) {
		std::clog << "[Error - Items::loadFromXml] Failed to load data/items/items.xml: " << result.description() << std::endl;
		return false;
	}

	for(pugi::xml_node itemNode = doc.child("items").first_child(); itemNode; itemNode = itemNode.next_sibling()) {
		pugi::xml_attribute idAttribute = itemNode.attribute("id");
		if(idAttribute) {
			parseItemNode(itemNode, pugi::cast<uint32_t>(idAttribute.value()));
		} else {
			pugi::xml_attribute fromIdAttribute = itemNode.attribute("fromid");
			if(fromIdAttribute) {
				pugi::xml_attribute toIdAttribute = itemNode.attribute("toid");
				if(toIdAttribute) {
					uint16_t id = pugi::cast<uint16_t>(fromIdAttribute.value());
					uint16_t toId = pugi::cast<uint16_t>(toIdAttribute.value());
					while (id <= toId) {
						parseItemNode(itemNode, id++);
					}
				} else {
					std::clog << "[Warning - Items::loadFromXml] fromid (" << fromIdAttribute.value() << ") without toid" << std::endl;
				}
			} else {
				std::clog << "[Warning - Items::loadFromXml] No itemid found" << std::endl;
			}
		}
	}
	return true;
}

bool Items::parseItemNode(const pugi::xml_node& itemNode, uint32_t id)
{
	if(id > 20000 && id < 20100)
	{
		id -= 20000;
		ItemType* iType = new ItemType();
		iType->id = id;
		items->addElement(iType, id);
	}

	ItemType& it = getItemType(id);
	it.name = itemNode.attribute("name").as_string();

	pugi::xml_attribute articleAttribute = itemNode.attribute("article");
	if(articleAttribute) {
		it.article = articleAttribute.as_string();
	}

	pugi::xml_attribute pluralAttribute = itemNode.attribute("plural");
	if(pluralAttribute) {
		it.pluralName = pluralAttribute.as_string();
	}

	for(pugi::xml_node attributeNode = itemNode.first_child(); attributeNode; attributeNode = attributeNode.next_sibling()) {
		pugi::xml_attribute keyAttribute = attributeNode.attribute("key");
		if(!keyAttribute) {
			continue;
		}

		pugi::xml_attribute valueAttribute = attributeNode.attribute("value");
		if(!valueAttribute) {
			continue;
		}

		std::string tmpStrValue = asLowerCaseString(keyAttribute.as_string());
		if(tmpStrValue == "type") {
			tmpStrValue = asLowerCaseString(valueAttribute.as_string());
			if(tmpStrValue == "key") {
				it.group = ITEM_GROUP_KEY;
			} else if(tmpStrValue == "magicfield") {
				it.type = ITEM_TYPE_MAGICFIELD;
			} else if(tmpStrValue == "container") {
				it.group = ITEM_GROUP_CONTAINER;
				it.type = ITEM_TYPE_CONTAINER;
			} else if(tmpStrValue == "depot") {
				it.type = ITEM_TYPE_DEPOT;
			} else if(tmpStrValue == "mailbox") {
				it.type = ITEM_TYPE_MAILBOX;
			} else if(tmpStrValue == "trashholder") {
				it.type = ITEM_TYPE_TRASHHOLDER;
			} else if(tmpStrValue == "teleport") {
				it.type = ITEM_TYPE_TELEPORT;
			} else if(tmpStrValue == "door") {
				it.type = ITEM_TYPE_DOOR;
			} else if(tmpStrValue == "bed") {
				it.type = ITEM_TYPE_BED;
			} else {
				std::clog << "[Warning - Items::parseItemNode] Unknown type: " << valueAttribute.as_string() << std::endl;
			}
		} else if(tmpStrValue == "description") {
			it.description = valueAttribute.as_string();
		} else if(tmpStrValue == "runespellname") {
			it.runeSpellName = valueAttribute.as_string();
		} else if(tmpStrValue == "weight") {
			it.weight = pugi::cast<uint32_t>(valueAttribute.value()) / 100.f;
		} else if(tmpStrValue == "showcount") {
			it.showCount = valueAttribute.as_bool();
		} else if(tmpStrValue == "armor") {
			it.armor = pugi::cast<int32_t>(valueAttribute.value());
		} else if(tmpStrValue == "defense") {
			it.defense = pugi::cast<int32_t>(valueAttribute.value());
		} else if(tmpStrValue == "extradef") {
			it.extraDefense = pugi::cast<int32_t>(valueAttribute.value());
		} else if(tmpStrValue == "attack") {
			it.attack = pugi::cast<int32_t>(valueAttribute.value());
		} else if(tmpStrValue == "rotateto") {
			it.rotateTo = pugi::cast<int32_t>(valueAttribute.value());
		} else if(tmpStrValue == "moveable" || tmpStrValue == "movable") {
			it.moveable = valueAttribute.as_bool();
		} else if(tmpStrValue == "blockprojectile") {
			it.blockProjectile = valueAttribute.as_bool();
		} else if(tmpStrValue == "corpsetype") {
			tmpStrValue = asLowerCaseString(valueAttribute.as_string());
			if(tmpStrValue == "venom") {
				it.corpseType = RACE_VENOM;
			} else if(tmpStrValue == "blood") {
				it.corpseType = RACE_BLOOD;
			} else if(tmpStrValue == "undead") {
				it.corpseType = RACE_UNDEAD;
			} else if(tmpStrValue == "fire") {
				it.corpseType = RACE_FIRE;
			} else {
				std::clog << "[Warning - Items::parseItemNode] Unknown corpseType: " << valueAttribute.as_string() << std::endl;
			}
		} else if(tmpStrValue == "containersize") {
			it.maxItems = pugi::cast<uint16_t>(valueAttribute.value());
		} else if(tmpStrValue == "fluidsource") {
			tmpStrValue = asLowerCaseString(valueAttribute.as_string());
			if(tmpStrValue == "water") {
				it.fluidSource = FLUID_WATER;
			} else if(tmpStrValue == "blood") {
				it.fluidSource = FLUID_BLOOD;
			} else if(tmpStrValue == "beer") {
				it.fluidSource = FLUID_BEER;
			} else if(tmpStrValue == "slime") {
				it.fluidSource = FLUID_SLIME;
			} else if(tmpStrValue == "lemonade") {
				it.fluidSource = FLUID_LEMONADE;
			} else if(tmpStrValue == "milk") {
				it.fluidSource = FLUID_MILK;
			} else if(tmpStrValue == "mana") {
				it.fluidSource = FLUID_MANA;
			} else if(tmpStrValue == "life") {
				it.fluidSource = FLUID_LIFE;
			} else if(tmpStrValue == "oil") {
				it.fluidSource = FLUID_OIL;
			} else if(tmpStrValue == "urine") {
				it.fluidSource = FLUID_URINE;
			} else if(tmpStrValue == "coconut") {
				it.fluidSource = FLUID_COCONUTMILK;
			} else if(tmpStrValue == "wine") {
				it.fluidSource = FLUID_WINE;
			} else if(tmpStrValue == "mud") {
				it.fluidSource = FLUID_MUD;
			} else if(tmpStrValue == "fruitjuice") {
				it.fluidSource = FLUID_FRUITJUICE;
			} else if(tmpStrValue == "lava") {
				it.fluidSource = FLUID_LAVA;
			} else if(tmpStrValue == "rum") {
				it.fluidSource = FLUID_RUM;
			} else if(tmpStrValue == "swamp") {
				it.fluidSource = FLUID_SWAMP;
			} else {
				std::clog << "[Warning - Items::parseItemNode] Unknown fluidSource: " << valueAttribute.as_string() << std::endl;
			}
		} else if(tmpStrValue == "writeable") {
			it.canWriteText = valueAttribute.as_bool();
			it.canReadText = it.canWriteText;
		} else if(tmpStrValue == "maxtextlen") {
			it.maxTextLen = pugi::cast<uint16_t>(valueAttribute.value());
		} else if(tmpStrValue == "writeonceitemid") {
			it.writeOnceItemId = pugi::cast<uint16_t>(valueAttribute.value());
		} else if(tmpStrValue == "weapontype") {
			tmpStrValue = asLowerCaseString(valueAttribute.as_string());
			if(tmpStrValue == "sword") {
				it.weaponType = WEAPON_SWORD;
			} else if(tmpStrValue == "club") {
				it.weaponType = WEAPON_CLUB;
			} else if(tmpStrValue == "axe") {
				it.weaponType = WEAPON_AXE;
			} else if(tmpStrValue == "shield") {
				it.weaponType = WEAPON_SHIELD;
			} else if(tmpStrValue == "distance") {
				it.weaponType = WEAPON_DIST;
			} else if(tmpStrValue == "wand") {
				it.weaponType = WEAPON_WAND;
			} else if(tmpStrValue == "ammunition") {
				it.weaponType = WEAPON_AMMO;
			} else {
				std::clog << "[Warning - Items::parseItemNode] Unknown weaponType: " << valueAttribute.as_string() << std::endl;
			}
		} else if(tmpStrValue == "slottype") {
			tmpStrValue = asLowerCaseString(valueAttribute.as_string());
			if(tmpStrValue == "head") {
				it.slot_position |= SLOTP_HEAD;
			} else if(tmpStrValue == "body") {
				it.slot_position |= SLOTP_ARMOR;
			} else if(tmpStrValue == "legs") {
				it.slot_position |= SLOTP_LEGS;
			} else if(tmpStrValue == "feet") {
				it.slot_position |= SLOTP_FEET;
			} else if(tmpStrValue == "backpack") {
				it.slot_position |= SLOTP_BACKPACK;
			} else if(tmpStrValue == "two-handed") {
				it.slot_position |= SLOTP_TWO_HAND;
			} else if(tmpStrValue == "necklace") {
				it.slot_position |= SLOTP_NECKLACE;
			} else if(tmpStrValue == "ring") {
				it.slot_position |= SLOTP_RING;
			} else {
				std::clog << "[Warning - Items::parseItemNode] Unknown slotType: " << valueAttribute.as_string() << std::endl;
			}
		} else if(tmpStrValue == "ammotype") {
			it.ammoType = getAmmoType(valueAttribute.as_string());
			if(it.ammoType == AMMO_NONE) {
				std::clog << "[Warning - Items::parseItemNode] Unknown ammoType: " << valueAttribute.as_string() << std::endl;
			}
		} else if(tmpStrValue == "shoottype") {
			ShootType_t shoot = getShootType(valueAttribute.as_string());
			if(shoot != NM_SHOOT_UNK) {
				it.shootType = shoot;
			} else {
				std::clog << "[Warning - Items::parseItemNode] Unknown shootType: " << valueAttribute.as_string() << std::endl;
			}
		} else if(tmpStrValue == "effect") {
			MagicEffectClasses effect = getMagicEffect(valueAttribute.as_string());
			if(effect != NM_ME_UNK) {
				it.magicEffect = effect;
			} else {
				std::clog << "[Warning - Items::parseItemNode] Unknown effect: " << valueAttribute.as_string() << std::endl;
			}
		} else if(tmpStrValue == "range") {
			it.shootRange = pugi::cast<int32_t>(valueAttribute.value());
		} else if(tmpStrValue == "stopduration") {
			it.stopTime = valueAttribute.as_bool();
		} else if(tmpStrValue == "decayto") {
			it.decayTo = pugi::cast<int32_t>(valueAttribute.value());
		} else if(tmpStrValue == "transformequipto") {
			it.transformEquipTo = pugi::cast<uint16_t>(valueAttribute.value());
		} else if(tmpStrValue == "transformdeequipto") {
			it.transformDeEquipTo = pugi::cast<uint16_t>(valueAttribute.value());
		} else if(tmpStrValue == "duration") {
			it.decayTime = pugi::cast<uint32_t>(valueAttribute.value());
		} else if(tmpStrValue == "showduration") {
			it.showDuration = valueAttribute.as_bool();
		} else if(tmpStrValue == "charges") {
			it.charges = pugi::cast<uint32_t>(valueAttribute.value());
		} else if(tmpStrValue == "showcharges") {
			it.showCharges = valueAttribute.as_bool();
		} else if(tmpStrValue == "showattributes") {
			it.showAttributes = valueAttribute.as_bool();
		} else if(tmpStrValue == "breakchance") {
			it.breakChance = std::min<uint32_t>(100, pugi::cast<uint32_t>(valueAttribute.value()));
		} else if(tmpStrValue == "ammoaction") {
			it.ammoAction = getAmmoAction(valueAttribute.as_string());
			if(it.ammoAction == AMMOACTION_NONE) {
				std::clog << "[Warning - Items::parseItemNode] Unknown ammoAction " << valueAttribute.as_string() << std::endl;
			}
		} else if(tmpStrValue == "hitchance") {
			it.hitChance = std::min<int32_t>(100, std::max<int32_t>(-100, pugi::cast<int32_t>(valueAttribute.value())));
		} else if(tmpStrValue == "maxhitchance") {
			it.maxHitChance = std::min<uint32_t>(100, pugi::cast<uint32_t>(valueAttribute.value()));
		} else if(tmpStrValue == "invisible") {
			it.getAbilities()->invisible = valueAttribute.as_bool();
		} else if(tmpStrValue == "speed") {
			it.getAbilities()->speed = pugi::cast<int32_t>(valueAttribute.value());
		} else if(tmpStrValue == "healthgain") {
			Abilities* abilities = it.getAbilities();
			abilities->regeneration = true;
			abilities->healthGain = pugi::cast<uint32_t>(valueAttribute.value());
		} else if(tmpStrValue == "healthticks") {
			Abilities* abilities = it.getAbilities();
			abilities->regeneration = true;
			abilities->healthTicks = pugi::cast<uint32_t>(valueAttribute.value());
		} else if(tmpStrValue == "managain") {
			Abilities* abilities = it.getAbilities();
			abilities->regeneration = true;
			abilities->manaGain = pugi::cast<uint32_t>(valueAttribute.value());
		} else if(tmpStrValue == "manaticks") {
			Abilities* abilities = it.getAbilities();
			abilities->regeneration = true;
			abilities->manaTicks = pugi::cast<uint32_t>(valueAttribute.value());
		} else if(tmpStrValue == "manashield") {
			it.getAbilities()->manaShield = valueAttribute.as_bool();
		} else if(tmpStrValue == "skillsword") {
			it.getAbilities()->skills[SKILL_SWORD] = pugi::cast<int32_t>(valueAttribute.value());
		} else if(tmpStrValue == "skillaxe") {
			it.getAbilities()->skills[SKILL_AXE] = pugi::cast<int32_t>(valueAttribute.value());
		} else if(tmpStrValue == "skillclub") {
			it.getAbilities()->skills[SKILL_CLUB] = pugi::cast<int32_t>(valueAttribute.value());
		} else if(tmpStrValue == "skilldist") {
			it.getAbilities()->skills[SKILL_DIST] = pugi::cast<int32_t>(valueAttribute.value());
		} else if(tmpStrValue == "skillfish") {
			it.getAbilities()->skills[SKILL_FISH] = pugi::cast<int32_t>(valueAttribute.value());
		} else if(tmpStrValue == "skillshield") {
			it.getAbilities()->skills[SKILL_SHIELD] = pugi::cast<int32_t>(valueAttribute.value());
		} else if(tmpStrValue == "skillfist") {
			it.getAbilities()->skills[SKILL_FIST] = pugi::cast<int32_t>(valueAttribute.value());
		} else if(tmpStrValue == "maxhitpoints") {
			it.getAbilities()->stats[STAT_MAXHITPOINTS] = pugi::cast<int32_t>(valueAttribute.value());
		} else if(tmpStrValue == "maxhitpointspercent") {
			it.getAbilities()->statsPercent[STAT_MAXHITPOINTS] = pugi::cast<int32_t>(valueAttribute.value());
		} else if(tmpStrValue == "maxmanapoints") {
			it.getAbilities()->stats[STAT_MAXMANAPOINTS] = pugi::cast<int32_t>(valueAttribute.value());
		} else if(tmpStrValue == "maxmanapointspercent") {
			it.getAbilities()->statsPercent[STAT_MAXMANAPOINTS] = pugi::cast<int32_t>(valueAttribute.value());
		} else if(tmpStrValue == "soulpoints") {
			it.getAbilities()->stats[STAT_SOULPOINTS] = pugi::cast<int32_t>(valueAttribute.value());
		} else if(tmpStrValue == "soulpointspercent") {
			it.getAbilities()->statsPercent[STAT_SOULPOINTS] = pugi::cast<int32_t>(valueAttribute.value());
		} else if(tmpStrValue == "magicpoints" || tmpStrValue == "magiclevelpoints") {
			it.getAbilities()->stats[STAT_MAGICPOINTS] = pugi::cast<int32_t>(valueAttribute.value());
		} else if(tmpStrValue == "magicpointspercent") {
			it.getAbilities()->statsPercent[STAT_MAGICPOINTS] = pugi::cast<int32_t>(valueAttribute.value());
		} else if(tmpStrValue == "absorbpercentall" || tmpStrValue == "absorbpercentallelements") {
			int16_t value = pugi::cast<int16_t>(valueAttribute.value());
			Abilities* abilities = it.getAbilities();
			for(uint32_t i = COMBAT_FIRST; i <= COMBAT_COUNT; i++) {
				abilities->absorbPercent[i] += value;
			}
		} else if(tmpStrValue == "absorbpercentenergy") {
			it.getAbilities()->absorbPercent[combatTypeToIndex(COMBAT_ENERGYDAMAGE)] += pugi::cast<int16_t>(valueAttribute.value());
		} else if(tmpStrValue == "absorbpercentfire") {
			it.getAbilities()->absorbPercent[combatTypeToIndex(COMBAT_FIREDAMAGE)] += pugi::cast<int16_t>(valueAttribute.value());
		} else if(tmpStrValue == "absorbpercentpoison" ||	tmpStrValue == "absorbpercentearth") {
			it.getAbilities()->absorbPercent[combatTypeToIndex(COMBAT_POISONDAMAGE)] += pugi::cast<int16_t>(valueAttribute.value());
		} else if(tmpStrValue == "absorbpercentlifedrain") {
			it.getAbilities()->absorbPercent[combatTypeToIndex(COMBAT_LIFEDRAIN)] += pugi::cast<int16_t>(valueAttribute.value());
		} else if(tmpStrValue == "absorbpercentmanadrain") {
			it.getAbilities()->absorbPercent[combatTypeToIndex(COMBAT_MANADRAIN)] += pugi::cast<int16_t>(valueAttribute.value());
		} else if(tmpStrValue == "absorbpercentdrown") {
			it.getAbilities()->absorbPercent[combatTypeToIndex(COMBAT_DROWNDAMAGE)] += pugi::cast<int16_t>(valueAttribute.value());
		} else if(tmpStrValue == "absorbpercentphysical") {
			it.getAbilities()->absorbPercent[combatTypeToIndex(COMBAT_PHYSICALDAMAGE)] += pugi::cast<int16_t>(valueAttribute.value());
		} else if(tmpStrValue == "suppressdrunk") {
			if(valueAttribute.as_bool()) {
				it.getAbilities()->conditionSuppressions |= CONDITION_DRUNK;
			}
		} else if(tmpStrValue == "suppressenergy") {
			if(valueAttribute.as_bool()) {
				it.getAbilities()->conditionSuppressions |= CONDITION_ENERGY;
			}
		} else if(tmpStrValue == "suppressfire") {
			if(valueAttribute.as_bool()) {
				it.getAbilities()->conditionSuppressions |= CONDITION_FIRE;
			}
		} else if(tmpStrValue == "suppresspoison") {
			if(valueAttribute.as_bool()) {
				it.getAbilities()->conditionSuppressions |= CONDITION_POISON;
			}
		} else if(tmpStrValue == "suppresslifedrain") {
			if(valueAttribute.as_bool()) {
				it.getAbilities()->conditionSuppressions |= CONDITION_LIFEDRAIN;
			}	
		} else if(tmpStrValue == "suppressdrown") {
			if(valueAttribute.as_bool()) {
				it.getAbilities()->conditionSuppressions |= CONDITION_DROWN;
			}
		} else if(tmpStrValue == "field") {
			it.group = ITEM_GROUP_MAGICFIELD;
			it.type = ITEM_TYPE_MAGICFIELD;

			CombatType_t combatType = COMBAT_NONE;
			ConditionDamage* conditionDamage = NULL;

			tmpStrValue = asLowerCaseString(valueAttribute.as_string());
			if(tmpStrValue == "fire") {
				conditionDamage = new ConditionDamage(CONDITIONID_COMBAT, CONDITION_FIRE);
				combatType = COMBAT_FIREDAMAGE;
			} else if(tmpStrValue == "energy") {
				conditionDamage = new ConditionDamage(CONDITIONID_COMBAT, CONDITION_ENERGY);
				combatType = COMBAT_ENERGYDAMAGE;
			} else if(tmpStrValue == "poison") {
				conditionDamage = new ConditionDamage(CONDITIONID_COMBAT, CONDITION_POISON);
				combatType = COMBAT_POISONDAMAGE;
			} else if(tmpStrValue == "drown") {
				conditionDamage = new ConditionDamage(CONDITIONID_COMBAT, CONDITION_DROWN);
				combatType = COMBAT_DROWNDAMAGE;
			} else {
				std::clog << "[Warning - Items::parseItemNode] Unknown field value: " << valueAttribute.as_string() << std::endl;
			}

			if(combatType != COMBAT_NONE) {
				it.combatType = combatType;
				it.condition = conditionDamage;
				uint32_t ticks = 0;
				int32_t damage = 0;
				int32_t start = 0;
				int32_t count = 1;

				for(pugi::xml_node subAttributeNode = attributeNode.first_child(); subAttributeNode; subAttributeNode = subAttributeNode.next_sibling()) {
					pugi::xml_attribute subKeyAttribute = subAttributeNode.attribute("key");
					if(!subKeyAttribute) {
						continue;
					}

					pugi::xml_attribute subValueAttribute = subAttributeNode.attribute("value");
					if(!subValueAttribute) {
						continue;
					}

					tmpStrValue = asLowerCaseString(subKeyAttribute.as_string());
					if(tmpStrValue == "ticks") {
						ticks = pugi::cast<uint32_t>(subValueAttribute.value());
					} else if(tmpStrValue == "count") {
						count = std::max<int32_t>(1, pugi::cast<int32_t>(subValueAttribute.value()));
					} else if(tmpStrValue == "start") {
						start = std::max<int32_t>(0, pugi::cast<int32_t>(subValueAttribute.value()));
					} else if(tmpStrValue == "damage") {
						damage = -pugi::cast<int32_t>(subValueAttribute.value());

						if(start > 0) {
							std::list<int32_t> damageList;
							ConditionDamage::generateDamageList(damage, start, damageList);
							for(int32_t damageValue : damageList) {
								conditionDamage->addDamage(1, ticks, -damageValue);
							}

							start = 0;
						} else {
							conditionDamage->addDamage(count, ticks, damage);
						}
					}
				}

				if(conditionDamage->getTotalDamage() > 0) {
					conditionDamage->setParam(CONDITIONPARAM_FORCEUPDATE, 1);
				}
			}
		} else if(tmpStrValue == "replaceable") {
			it.replaceable = valueAttribute.as_bool();
		} else if(tmpStrValue == "partnerdirection") {
			it.bedPartnerDir = getDirection(valueAttribute.as_string());
		} else if(tmpStrValue == "leveldoor") {
			it.levelDoor = pugi::cast<uint32_t>(valueAttribute.value());
		} else if(tmpStrValue == "maletransformto" || tmpStrValue == "malesleeper") {
			uint16_t value = pugi::cast<uint16_t>(valueAttribute.value());
			it.transformToOnUse[PLAYERSEX_MALE] = value;
			ItemType& other = getItemType(value);

			if(other.transformToFree == 0) {
				other.transformToFree = it.id;
			}

			if(it.transformToOnUse[PLAYERSEX_FEMALE] == 0) {
				it.transformToOnUse[PLAYERSEX_FEMALE] = value;
			}
		} else if(tmpStrValue == "femaletransformto" || tmpStrValue == "femalesleeper") {
			uint16_t value = pugi::cast<uint16_t>(valueAttribute.value());
			it.transformToOnUse[PLAYERSEX_FEMALE] = value;

			ItemType& other = getItemType(value);
			if(other.transformToFree == 0) {
				other.transformToFree = it.id;
			}

			if(it.transformToOnUse[PLAYERSEX_MALE] == 0) {
				it.transformToOnUse[PLAYERSEX_MALE] = value;
			}
		} else if(tmpStrValue == "transformto") {
			it.transformToFree = pugi::cast<uint16_t>(valueAttribute.value());
		} else if(tmpStrValue == "elementearth" || tmpStrValue == "elementpoison") {
			it.getAbilities()->elementDamage = pugi::cast<int16_t>(valueAttribute.value());
			it.getAbilities()->elementType = COMBAT_POISONDAMAGE;
		} else if(tmpStrValue == "elementfire") {
			it.getAbilities()->elementDamage = pugi::cast<int16_t>(valueAttribute.value());
			it.getAbilities()->elementType = COMBAT_FIREDAMAGE;
		} else if(tmpStrValue == "elementenergy") {
			it.getAbilities()->elementDamage = pugi::cast<int16_t>(valueAttribute.value());
			it.getAbilities()->elementType = COMBAT_ENERGYDAMAGE;
		} else {
			std::clog << "[Warning - Items::parseItemNode] Unknown key value: " << keyAttribute.as_string() << std::endl;
		}
	}

	//check bed items
	if((it.transformToFree != 0 || it.transformToOnUse[PLAYERSEX_FEMALE] != 0 || it.transformToOnUse[PLAYERSEX_MALE] != 0) && it.type != ITEM_TYPE_BED) {
		std::clog << "[Warning - Items::parseItemNode] Item " << it.id << " is not set as a bed-type" << std::endl;
	}

	/*
	if(!it.marketName.empty() && it.marketName != it.name) {
		std::clog << "ID: " << it.id << ". Market Name: " << it.marketName << ". Item Name: " << it.name << '.' << std::endl;
	}
	*/

	return true;
}

ItemType& Items::getItemType(int32_t id)
{
	ItemType* iType = items->getElement(id);
	if(iType)
		return *iType;

	#ifdef __DEBUG__
	std::clog << "WARNING! unknown itemtypeid " << id << ". using defaults." << std::endl;
	#endif
	static ItemType dummyItemType; // use this for invalid ids
	return dummyItemType;
}

const ItemType& Items::getItemType(int32_t id) const
{
	ItemType* iType = items->getElement(id);
	if(iType)
		return *iType;

	static ItemType dummyItemType; // use this for invalid ids
	return dummyItemType;
}

const ItemType& Items::getItemIdByClientId(int32_t spriteId) const
{
	uint32_t i = 100;
	ItemType* iType = items->getElement(i);
	while(iType)
	{
		if(iType->clientId == spriteId)
			return *iType;

		iType = items->getElement(++i);
	}

	static ItemType dummyItemType; // use this for invalid ids
	return dummyItemType;
}

int32_t Items::getItemIdByName(const std::string& name)
{
	if(!name.empty())
	{
		uint32_t i = 100;
		ItemType* iType = NULL;
		do
		{
			if((iType = items->getElement(i)) && !strcasecmp(name.c_str(), iType->name.c_str()))
				return i;

			i++;
		}
		while(iType);
	}

	return -1;
}

