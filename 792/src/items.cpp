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

#include <libxml/xmlschemas.h>

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
	std::string filename = "data/items/items.xml";
	xmlDocPtr doc = xmlParseFile(filename.c_str());

	std::string strValue;
	std::string endString;

	std::vector<int> intVector;
	std::vector<int> endVector;

	if(!doc)
		return false;

	xmlNodePtr root = xmlDocGetRootElement(doc);
	if(xmlStrcmp(root->name,(const xmlChar*)"items") != 0)
	{
		xmlFreeDoc(doc);
		return false;
	}

	xmlNodePtr itemNode = root->children;
	while(itemNode)
	{
		if(xmlStrcmp(itemNode->name,(const xmlChar*)"item") == 0)
		{
			if(readXMLString(itemNode, "id", strValue))
			{
				intVector = vectorAtoi(explodeString(strValue, ";"));
				size_t vec_size = intVector.size();
				for(size_t i = 0; i < vec_size; i++)
					this->parseItemNode(itemNode, intVector[i]);
			}
			else if(readXMLString(itemNode, "fromid", strValue) && readXMLString(itemNode, "toid", endString))
			{
				intVector = vectorAtoi(explodeString(strValue, ";"));
				endVector = vectorAtoi(explodeString(endString, ";"));

				if(!intVector.empty() && intVector.size() == endVector.size())
				{
					size_t vec_size = intVector.size();
					for(size_t i = 0; i < vec_size; i++)
					{
						while(intVector[i] <= endVector[i])
							parseItemNode(itemNode, intVector[i]++);
					}
				}
			}
			else
				std::clog << "Warning: [Items::loadFromXml] - No itemid found" << std::endl;
		}
		itemNode = itemNode->next;
	}
	xmlFreeDoc(doc);

	//Lets do some checks...
	for(uint32_t i = 0; i < items->size(); ++i)
	{
		const ItemType* it = items->getElement(i);
		if(!it)
			continue;

		//check bed items
		if((it->transformToFree != 0 || it->transformToOnUse[PLAYERSEX_FEMALE] != 0 || it->transformToOnUse[PLAYERSEX_MALE] != 0) && it->type != ITEM_TYPE_BED)
		{
			std::clog << "Warning: [Items::loadFromXml] Item " << it->id << " is not set as a bed-type." << std::endl;
		}
	}
	return true;
}

bool Items::parseItemNode(xmlNodePtr itemNode, uint32_t id)
{
	int32_t intValue;
	std::string strValue;

	if(id > 20000 && id < 20100)
	{
		id -= 20000;
		ItemType* iType = new ItemType();
		iType->id = id;

		items->addElement(iType, id);
	}

	ItemType& it = getItemType(id);

	if(readXMLString(itemNode, "name", strValue))
		it.name = strValue;

	if(readXMLString(itemNode, "article", strValue))
		it.article = strValue;

	if(readXMLString(itemNode, "plural", strValue))
		it.pluralName = strValue;

	xmlNodePtr itemAttributesNode = itemNode->children;

	while(itemAttributesNode)
	{
		if(readXMLString(itemAttributesNode, "key", strValue))
		{
			std::string tmpStrValue = asLowerCaseString(strValue);
			if(tmpStrValue == "type")
			{
				if(readXMLString(itemAttributesNode, "value", strValue))
				{
					tmpStrValue = asLowerCaseString(strValue);
					if(tmpStrValue == "key")
						it.group = ITEM_GROUP_KEY;
					else if(tmpStrValue == "magicfield")
					{
						it.type = ITEM_TYPE_MAGICFIELD;
					}
					else if(tmpStrValue == "container")
					{
						it.group = ITEM_GROUP_CONTAINER;
						it.type = ITEM_TYPE_CONTAINER;
					}
					else if(tmpStrValue == "depot")
						it.type = ITEM_TYPE_DEPOT;
					else if(tmpStrValue == "mailbox")
						it.type = ITEM_TYPE_MAILBOX;
					else if(tmpStrValue == "trashholder")
						it.type = ITEM_TYPE_TRASHHOLDER;
					else if(tmpStrValue == "teleport")
						it.type = ITEM_TYPE_TELEPORT;
					else if(tmpStrValue == "door")
						it.type = ITEM_TYPE_DOOR;
					else if(tmpStrValue == "bed")
						it.type = ITEM_TYPE_BED;
					else
						std::clog << "Warning: [Items::loadFromXml] " << "Unknown type " << strValue << std::endl;
				}
			}
			else if(tmpStrValue == "name")
			{
				if(readXMLString(itemAttributesNode, "value", strValue))
					it.name = strValue;
			}
			else if(tmpStrValue == "article")
			{
				if(readXMLString(itemAttributesNode, "value", strValue))
					it.article = strValue;
			}
			else if(tmpStrValue == "plural")
			{
				if(readXMLString(itemAttributesNode, "value", strValue))
					it.pluralName = strValue;
			}
			else if(tmpStrValue == "description")
			{
				if(readXMLString(itemAttributesNode, "value", strValue))
					it.description = strValue;
			}
			else if(tmpStrValue == "runespellname")
			{
				if(readXMLString(itemAttributesNode, "value", strValue))
					it.runeSpellName = strValue;
			}
			else if(tmpStrValue == "weight")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
					it.weight = intValue / 100.f;
			}
			else if(tmpStrValue == "showcount")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
					it.showCount = (intValue != 0);
			}
			else if(tmpStrValue == "armor")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
					it.armor = intValue;
			}
			else if(tmpStrValue == "defense")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
					it.defense = intValue;
			}
			else if(tmpStrValue == "extradef")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
					it.extraDefense = intValue;
			}
			else if(tmpStrValue == "attack")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
					it.attack = intValue;
			}
			else if(tmpStrValue == "rotateto")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
					it.rotateTo = intValue;
			}
			else if(tmpStrValue == "moveable" || tmpStrValue == "movable")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
					it.moveable = (intValue == 1);
			}
			else if(tmpStrValue == "blockprojectile")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
					it.blockProjectile = (intValue == 1);
			}
			else if(tmpStrValue == "corpsetype")
			{
				tmpStrValue = asLowerCaseString(strValue);
				if(readXMLString(itemAttributesNode, "value", strValue))
				{
					tmpStrValue = asLowerCaseString(strValue);
					if(tmpStrValue == "venom")
						it.corpseType = RACE_VENOM;
					else if(tmpStrValue == "blood")
						it.corpseType = RACE_BLOOD;
					else if(tmpStrValue == "undead")
						it.corpseType = RACE_UNDEAD;
					else if(tmpStrValue == "fire")
						it.corpseType = RACE_FIRE;
					else
						std::clog << "Warning: [Items::loadFromXml] " << "Unknown corpseType " << strValue << std::endl;
				}
			}
			else if(tmpStrValue == "containersize")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
					it.maxItems = intValue;
			}
			else if(tmpStrValue == "fluidsource")
			{
				if(readXMLString(itemAttributesNode, "value", strValue))
				{
					tmpStrValue = asLowerCaseString(strValue);
					if(tmpStrValue == "water")
						it.fluidSource = FLUID_WATER;
					else if(tmpStrValue == "blood")
						it.fluidSource = FLUID_BLOOD;
					else if(tmpStrValue == "beer")
						it.fluidSource = FLUID_BEER;
					else if(tmpStrValue == "slime")
						it.fluidSource = FLUID_SLIME;
					else if(tmpStrValue == "lemonade")
						it.fluidSource = FLUID_LEMONADE;
					else if(tmpStrValue == "milk")
						it.fluidSource = FLUID_MILK;
					else if(tmpStrValue == "mana")
						it.fluidSource = FLUID_MANA;
					else if(tmpStrValue == "life")
						it.fluidSource = FLUID_LIFE;
					else if(tmpStrValue == "oil")
						it.fluidSource = FLUID_OIL;
					else if(tmpStrValue == "urine")
						it.fluidSource = FLUID_URINE;
					else if(tmpStrValue == "coconut")
						it.fluidSource = FLUID_COCONUTMILK;
					else if(tmpStrValue == "wine")
						it.fluidSource = FLUID_WINE;
					else if(tmpStrValue == "mud")
						it.fluidSource = FLUID_MUD;
					else if(tmpStrValue == "fruitjuice")
						it.fluidSource = FLUID_FRUITJUICE;
					else if(tmpStrValue == "lava")
						it.fluidSource = FLUID_LAVA;
					else if(tmpStrValue == "rum")
						it.fluidSource = FLUID_RUM;
					else if(tmpStrValue == "swamp")
						it.fluidSource = FLUID_SWAMP;
					else
						std::clog << "Warning: [Items::loadFromXml] " << "Unknown fluidSource " << strValue << std::endl;
				}
			}
			else if(tmpStrValue == "writeable")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
				{
					it.canWriteText = (intValue != 0);
						it.canReadText = (intValue != 0);
				}
			}
			else if(tmpStrValue == "maxtextlen")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
					it.maxTextLen = intValue;
			}
			else if(tmpStrValue == "writeonceitemid")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
					it.writeOnceItemId = intValue;
			}
			else if(tmpStrValue == "weapontype")
			{
				if(readXMLString(itemAttributesNode, "value", strValue))
				{
					tmpStrValue = asLowerCaseString(strValue);
					if(tmpStrValue == "sword")
						it.weaponType = WEAPON_SWORD;
					else if(tmpStrValue == "club")
						it.weaponType = WEAPON_CLUB;
					else if(tmpStrValue == "axe")
						it.weaponType = WEAPON_AXE;
					else if(tmpStrValue == "shield")
						it.weaponType = WEAPON_SHIELD;
					else if(tmpStrValue == "distance")
						it.weaponType = WEAPON_DIST;
					else if(tmpStrValue == "wand")
						it.weaponType = WEAPON_WAND;
					else if(tmpStrValue == "ammunition")
						it.weaponType = WEAPON_AMMO;
					else
						std::clog << "Warning: [Items::loadFromXml] " << "Unknown weaponType " << strValue << std::endl;
				}
			}
			else if(tmpStrValue == "slottype")
			{
				if(readXMLString(itemAttributesNode, "value", strValue))
				{
					tmpStrValue = asLowerCaseString(strValue);
					if(tmpStrValue == "head")
						it.slot_position |= SLOTP_HEAD;
					else if(tmpStrValue == "body")
						it.slot_position |= SLOTP_ARMOR;
					else if(tmpStrValue == "legs")
						it.slot_position |= SLOTP_LEGS;
					else if(tmpStrValue == "feet")
						it.slot_position |= SLOTP_FEET;
					else if(tmpStrValue == "backpack")
						it.slot_position |= SLOTP_BACKPACK;
					else if(tmpStrValue == "two-handed")
						it.slot_position |= SLOTP_TWO_HAND;
					else if(tmpStrValue == "necklace")
						it.slot_position |= SLOTP_NECKLACE;
					else if(tmpStrValue == "ring")
						it.slot_position |= SLOTP_RING;
					else
						std::clog << "Warning: [Items::loadFromXml] " << "Unknown slotType " << strValue << std::endl;
				}
			}
			else if(tmpStrValue == "ammotype")
			{
				if(readXMLString(itemAttributesNode, "value", strValue))
				{
					it.ammoType = getAmmoType(strValue);
					if(it.ammoType == AMMO_NONE)
						std::clog << "Warning: [Items::loadFromXml] " << "Unknown ammoType " << strValue << std::endl;
				}
			}
			else if(tmpStrValue == "shoottype")
			{
				if(readXMLString(itemAttributesNode, "value", strValue))
				{
					ShootType_t shoot = getShootType(strValue);
					if(shoot != NM_SHOOT_UNK)
						it.shootType = shoot;
					else
						std::clog << "Warning: [Items::loadFromXml] " << "Unknown shootType " << strValue << std::endl;
				}
			}
			else if(tmpStrValue == "effect")
			{
				if(readXMLString(itemAttributesNode, "value", strValue))
				{
					MagicEffectClasses effect = getMagicEffect(strValue);
					if(effect != NM_ME_UNK)
						it.magicEffect = effect;
					else
						std::clog << "Warning: [Items::loadFromXml] " << "Unknown effect " << strValue << std::endl;
				}
			}
			else if(tmpStrValue == "range")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
					it.shootRange = intValue;
			}
			else if(tmpStrValue == "stopduration")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
					it.stopTime = (intValue != 0);
			}
			else if(tmpStrValue == "decayto")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
					it.decayTo = intValue;
			}
			else if(tmpStrValue == "transformequipto")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
					it.transformEquipTo = intValue;
			}
			else if(tmpStrValue == "transformdeequipto")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
					it.transformDeEquipTo = intValue;
			}
			else if(tmpStrValue == "duration")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
					it.decayTime = intValue;
			}
			else if(tmpStrValue == "showduration")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
					it.showDuration = (intValue != 0);
			}
			else if(tmpStrValue == "charges")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
					it.charges = intValue;
			}
			else if(tmpStrValue == "showcharges")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
					it.showCharges = (intValue != 0);
			}
			else if(tmpStrValue == "showattributes")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
					it.showAttributes = (intValue != 0);
			}
			else if(tmpStrValue == "breakchance")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
					it.breakChance = std::min<int32_t>(100, std::max<int32_t>(0, intValue));
			}
			else if(tmpStrValue == "ammoaction")
			{
				if(readXMLString(itemAttributesNode, "value", strValue))
				{
					it.ammoAction = getAmmoAction(strValue);
					if(it.ammoAction == AMMOACTION_NONE)
						std::clog << "Warning: [Items::loadFromXml] " << "Unknown ammoAction " << strValue << std::endl;
				}
			}
			else if(tmpStrValue == "hitchance")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
					it.hitChance = std::min<int32_t>(100, std::max<int32_t>(-100, intValue));
			}
			else if(tmpStrValue == "maxhitchance")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
					it.maxHitChance = std::min<int32_t>(100, std::max<int32_t>(-100, intValue));
			}
			else if(tmpStrValue == "invisible")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
					it.getAbilities()->invisible = (intValue != 0);
			}
			else if(tmpStrValue == "speed")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
					it.getAbilities()->speed = intValue;
			}
			else if(tmpStrValue == "healthgain")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
				{
					it.getAbilities()->regeneration = true;
					it.getAbilities()->healthGain = intValue;
				}
			}
			else if(tmpStrValue == "healthticks")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
				{
					it.getAbilities()->regeneration = true;
					it.getAbilities()->healthTicks = intValue;
				}
			}
			else if(tmpStrValue == "managain")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
				{
					it.getAbilities()->regeneration = true;
					it.getAbilities()->manaGain = intValue;
				}
			}
			else if(tmpStrValue == "manaticks")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
				{
					it.getAbilities()->regeneration = true;
					it.getAbilities()->manaTicks = intValue;
				}
			}
			else if(tmpStrValue == "manashield")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
					it.getAbilities()->manaShield = (intValue != 0);
			}
			else if(tmpStrValue == "skillsword")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
					it.getAbilities()->skills[SKILL_SWORD] = intValue;
			}
			else if(tmpStrValue == "skillaxe")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
					it.getAbilities()->skills[SKILL_AXE] = intValue;
			}
			else if(tmpStrValue == "skillclub")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
					it.getAbilities()->skills[SKILL_CLUB] = intValue;
			}
			else if(tmpStrValue == "skilldist")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
					it.getAbilities()->skills[SKILL_DIST] = intValue;
			}
			else if(tmpStrValue == "skillfish")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
					it.getAbilities()->skills[SKILL_FISH] = intValue;
			}
			else if(tmpStrValue == "skillshield")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
					it.getAbilities()->skills[SKILL_SHIELD] = intValue;
			}
			else if(tmpStrValue == "skillfist")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
					it.getAbilities()->skills[SKILL_FIST] = intValue;
			}
			else if(tmpStrValue == "maxhitpoints")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
					it.getAbilities()->stats[STAT_MAXHITPOINTS] = intValue;
			}
			else if(tmpStrValue == "maxhitpointspercent")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
					it.getAbilities()->stats[STAT_MAXHITPOINTS] = intValue;
			}
			else if(tmpStrValue == "maxmanapoints")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
					it.getAbilities()->stats[STAT_MAXMANAPOINTS] = intValue;
			}
			else if(tmpStrValue == "maxmanapointspercent")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
					it.getAbilities()->statsPercent[STAT_MAXMANAPOINTS] = intValue;
			}
			else if(tmpStrValue == "soulpoints")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
					it.getAbilities()->stats[STAT_SOULPOINTS] = intValue;
			}
			else if(tmpStrValue == "soulpointspercent")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
					it.getAbilities()->statsPercent[STAT_SOULPOINTS] = intValue;
			}
			else if(tmpStrValue == "magicpoints")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
					it.getAbilities()->stats[STAT_MAGICPOINTS] = intValue;
			}
			else if(tmpStrValue == "magicpointspercent")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
					it.getAbilities()->statsPercent[STAT_MAGICPOINTS] = intValue;
			}
			else if(tmpStrValue == "absorbpercentall" || tmpStrValue == "absorbpercentallelements")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
				{
					for(uint32_t i = COMBAT_FIRST; i <= COMBAT_COUNT; i++)
						it.getAbilities()->absorbPercent[i] += intValue;
				}
			}
			else if(tmpStrValue == "absorbpercentelements")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
				{
					it.getAbilities()->absorbPercent[combatTypeToIndex(COMBAT_ENERGYDAMAGE)] += intValue;
					it.getAbilities()->absorbPercent[combatTypeToIndex(COMBAT_FIREDAMAGE)] += intValue;
					it.getAbilities()->absorbPercent[combatTypeToIndex(COMBAT_POISONDAMAGE)] += intValue;
				}
			}
			else if(tmpStrValue == "absorbpercentmagic")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
				{
					it.getAbilities()->absorbPercent[combatTypeToIndex(COMBAT_ENERGYDAMAGE)] += intValue;
					it.getAbilities()->absorbPercent[combatTypeToIndex(COMBAT_FIREDAMAGE)] += intValue;
					it.getAbilities()->absorbPercent[combatTypeToIndex(COMBAT_POISONDAMAGE)] += intValue;
				}
			}
			else if(tmpStrValue == "absorbpercentenergy")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
					it.getAbilities()->absorbPercent[combatTypeToIndex(COMBAT_ENERGYDAMAGE)] += intValue;
			}
			else if(tmpStrValue == "absorbpercentfire")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
					it.getAbilities()->absorbPercent[combatTypeToIndex(COMBAT_FIREDAMAGE)] += intValue;
			}
			else if(tmpStrValue == "absorbpercentpoison" || tmpStrValue == "absorbpercentearth")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
					it.getAbilities()->absorbPercent[combatTypeToIndex(COMBAT_POISONDAMAGE)] += intValue;
			}
			else if(tmpStrValue == "absorbpercentlifedrain")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
					it.getAbilities()->absorbPercent[combatTypeToIndex(COMBAT_LIFEDRAIN)] += intValue;
			}
			else if(tmpStrValue == "absorbpercentmanadrain")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
					it.getAbilities()->absorbPercent[combatTypeToIndex(COMBAT_MANADRAIN)] += intValue;
			}
			else if(tmpStrValue == "absorbpercentdrown")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
					it.getAbilities()->absorbPercent[combatTypeToIndex(COMBAT_DROWNDAMAGE)] += intValue;
			}
			else if(tmpStrValue == "absorbpercentphysical")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
					it.getAbilities()->absorbPercent[combatTypeToIndex(COMBAT_PHYSICALDAMAGE)] += intValue;
			}
			else if(tmpStrValue == "suppressdrunk")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
					it.getAbilities()->conditionSuppressions |= CONDITION_DRUNK;
			}
			else if(tmpStrValue == "suppressenergy")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
					it.getAbilities()->conditionSuppressions |= CONDITION_ENERGY;
			}
			else if(tmpStrValue == "suppressfire")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
					it.getAbilities()->conditionSuppressions |= CONDITION_FIRE;
			}
			else if(tmpStrValue == "suppresspoison")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
					it.getAbilities()->conditionSuppressions |= CONDITION_POISON;
			}
			else if(tmpStrValue == "suppresslifedrain")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
					it.getAbilities()->conditionSuppressions |= CONDITION_LIFEDRAIN;
			}
			else if(tmpStrValue == "suppressdrown")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
				it.getAbilities()->conditionSuppressions |= CONDITION_DROWN;
			}
			else if(tmpStrValue == "field")
			{
				it.group = ITEM_GROUP_MAGICFIELD;
				it.type = ITEM_TYPE_MAGICFIELD;
				if(readXMLString(itemAttributesNode, "value", strValue))
				{
					CombatType_t combatType = COMBAT_NONE;
					ConditionDamage* conditionDamage = NULL;

					tmpStrValue = asLowerCaseString(strValue);
					if(tmpStrValue == "fire")
					{
						conditionDamage = new ConditionDamage(CONDITIONID_COMBAT, CONDITION_FIRE);
						combatType = COMBAT_FIREDAMAGE;
					}
					else if(tmpStrValue == "energy")
					{
						conditionDamage = new ConditionDamage(CONDITIONID_COMBAT, CONDITION_ENERGY);
						combatType = COMBAT_ENERGYDAMAGE;
					}
					else if(tmpStrValue == "poison")
					{
						conditionDamage = new ConditionDamage(CONDITIONID_COMBAT, CONDITION_POISON);
						combatType = COMBAT_POISONDAMAGE;
					}
					else if(tmpStrValue == "drown")
					{
						conditionDamage = new ConditionDamage(CONDITIONID_COMBAT, CONDITION_DROWN);
						combatType = COMBAT_DROWNDAMAGE;
					}
					else if(tmpStrValue == "physical")
					{
						//conditionDamage = new ConditionDamage(CONDITIONID_COMBAT, CONDITION_BLEEDING);
						combatType = COMBAT_PHYSICALDAMAGE;
					}
					else
						std::clog << "Warning: [Items::loadFromXml] " << "Unknown field value " << strValue << std::endl;

					if(combatType != COMBAT_NONE)
					{
						it.combatType = combatType;
						it.condition = conditionDamage;
						uint32_t ticks = 0;
						int32_t damage = 0;
						int32_t start = 0;
						int32_t count = 1;

						xmlNodePtr fieldAttributesNode = itemAttributesNode->children;
						while(fieldAttributesNode)
						{
							if(readXMLString(fieldAttributesNode, "key", strValue))
							{
								tmpStrValue = asLowerCaseString(strValue);
								if(tmpStrValue == "ticks")
								{
									if(readXMLInteger(fieldAttributesNode, "value", intValue))
										ticks = std::max<uint32_t>(0, intValue);
								}

								if(tmpStrValue == "count")
								{
									if(readXMLInteger(fieldAttributesNode, "value", intValue))
										count = std::max<int32_t>(1, intValue);
								}

								if(tmpStrValue == "start")
								{
									if(readXMLInteger(fieldAttributesNode, "value", intValue))
										start = std::max<int32_t>(0, intValue);
								}

								if(tmpStrValue == "damage")
								{
									if(readXMLInteger(fieldAttributesNode, "value", intValue))
									{
										damage = -intValue;
										if(start > 0)
										{
											std::list<int32_t> damageList;
											ConditionDamage::generateDamageList(damage, start, damageList);

											for(std::list<int32_t>::iterator it = damageList.begin(); it != damageList.end(); ++it)
												conditionDamage->addDamage(1, ticks, -*it);

											start = 0;
										}
										else
											conditionDamage->addDamage(count, ticks, damage);
									}
								}
							}
							fieldAttributesNode = fieldAttributesNode->next;
						}

						if(conditionDamage->getTotalDamage() > 0)
							conditionDamage->setParam(CONDITIONPARAM_FORCEUPDATE, 1);
					}
				}
			}
			else if(tmpStrValue == "replaceable")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
					it.replaceable = (intValue != 0);
			}
			else if(tmpStrValue == "partnerdirection")
			{
				if(readXMLString(itemAttributesNode, "value", strValue))
					it.bedPartnerDir = getDirection(strValue);
			}
			else if(tmpStrValue == "leveldoor")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
					it.levelDoor = intValue;
			}
			else if(tmpStrValue == "leveldoor")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
					it.levelDoor = intValue;
			}
			else if(tmpStrValue == "maletransformto" || tmpStrValue == "malesleeper")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
				{
					it.transformToOnUse[PLAYERSEX_MALE] = intValue;
					ItemType& other = getItemType(intValue);
					if(other.transformToFree == 0)
						other.transformToFree = it.id;

					if(it.transformToOnUse[PLAYERSEX_FEMALE] == 0)
						it.transformToOnUse[PLAYERSEX_FEMALE] = intValue;
				}
			}
			else if(tmpStrValue == "femaletransformto" || tmpStrValue == "femalesleeper")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
					it.transformToOnUse[PLAYERSEX_FEMALE] = intValue;

				ItemType& other = getItemType(intValue);
				if(other.transformToFree == 0)
					other.transformToFree = it.id;

				if(it.transformToOnUse[PLAYERSEX_MALE] == 0)
					it.transformToOnUse[PLAYERSEX_MALE] = intValue;
			}
			else if(tmpStrValue == "transformto")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
					it.transformToFree = intValue;
			}
			else if(tmpStrValue == "elementearth" || tmpStrValue == "elementpoison")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
				{
					it.getAbilities()->elementDamage = intValue;
					it.getAbilities()->elementType = COMBAT_POISONDAMAGE;
				}
			}
			else if(tmpStrValue == "elementfire")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
				{
					it.getAbilities()->elementDamage = intValue;
					it.getAbilities()->elementType = COMBAT_FIREDAMAGE;
				}
			}
			else if(tmpStrValue == "elementenergy")
			{
				if(readXMLInteger(itemAttributesNode, "value", intValue))
				{
					it.getAbilities()->elementDamage = intValue;
					it.getAbilities()->elementType = COMBAT_ENERGYDAMAGE;
				}
			}
			else
				std::clog << "Warning: [Items::loadFromXml] Unknown key value " << strValue << std::endl;
		}
		itemAttributesNode = itemAttributesNode->next;
	}
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

