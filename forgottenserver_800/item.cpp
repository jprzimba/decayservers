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
#include "otpch.h"
#include <iostream>
#include <iomanip>

#include "item.h"
#include "container.h"
#include "depot.h"

#include "teleport.h"
#include "trashholder.h"
#include "mailbox.h"

#include "luascript.h"
#include "combat.h"

#include "house.h"
#include "beds.h"

#include "actions.h"
#include "configmanager.h"
#include "game.h"
#include "movement.h"

extern Game g_game;
extern ConfigManager g_config;
extern MoveEvents* g_moveEvents;

Items Item::items;
Item* Item::CreateItem(const uint16_t type, uint16_t amount/* = 1*/)
{
	const ItemType& it = Item::items[type];
	if(it.group == ITEM_GROUP_DEPRECATED)
	{
		#ifdef __DEBUG__
		std::clog << "[Error - Item::CreateItem] Item " << it.id << " has been declared as deprecated" << std::endl;
		#endif
		return NULL;
	}

	if(!it.id)
		return NULL;

	Item* newItem = NULL;
	if(it.isDepot())
		newItem = new Depot(type);
	else if(it.isContainer())
		newItem = new Container(type);
	else if(it.isTeleport())
		newItem = new Teleport(type);
	else if(it.isMagicField())
		newItem = new MagicField(type);
	else if(it.isDoor())
		newItem = new Door(type);
	else if(it.isTrashHolder())
		newItem = new TrashHolder(type, it.magicEffect);
	else if(it.isMailbox())
		newItem = new Mailbox(type);
	else if(it.isBed())
		newItem = new BedItem(type);
	else if(it.id >= 2210 && it.id <= 2212)
		newItem = new Item(type - 3, amount);
	else if(it.id == 2215 || it.id == 2216)
		newItem = new Item(type - 2, amount);
	else if(it.id >= 2202 && it.id <= 2206)
		newItem = new Item(type - 37, amount);
	else if(it.id == 2640)
		newItem = new Item(6132, amount);
	else if(it.id == 6301)
		newItem = new Item(6300, amount);
	else
		newItem = new Item(type, amount);

	newItem->addRef();
	return newItem;
}

Item* Item::CreateItem(PropStream& propStream)
{
	uint16_t _id;
	if(!propStream.GET_USHORT(_id))
		return NULL;

	const ItemType& iType = Item::items[_id];
	unsigned char _count = 0;

	if(iType.stackable || iType.isSplash() || iType.isFluidContainer())
	{
		if(!propStream.GET_UCHAR(_count))
			return NULL;
	}

	if(g_config.getBool(ConfigManager::RANDOMIZE_TILES))
	{
		if(_id == 352 || _id == 353)
			_id = 351;
		else if(_id >= 709 && _id <= 711)
			_id = 708;
		else if(_id >= 3154 && _id <= 3157)
			_id = 3153;
		else if(_id >= 4527 && _id <= 4541 || _id == 4756)
			_id = 4526;
		else if(_id >= 4609 && _id <= 4625)
			_id = 4608;
		else if(_id >= 4692 && _id <= 4701)
			_id = 4691;
		else if(_id >= 5711 && _id <= 5726)
			_id = 101;
		else if(_id >= 6580 && _id <= 6593)
			_id = 670;
		else if(_id >= 6683 && _id <= 6686)
			_id = 671;
		else if(_id >= 5406 && _id <= 5410)
			_id = 5405;
		else if(_id >= 6805 && _id <= 6809)
			_id = 6804;
		else if(_id >= 7063 && _id <= 7066)
			_id = 7062;

		if((bool)random_range(0, 1))
		{
			switch(_id)
			{
				case 101:
					_id = random_range(5711, 5726);
					break;
				case 351:
				case 708:
					_id += random_range(1, 3);
					break;
				case 3153:
				case 7062:
					_id += random_range(1, 4);
					break;
				case 670:
					_id = random_range(6580, 6593);
					break;
				case 671:
					_id = random_range(6683, 6686);
					break;
				case 4405:
				case 4422:
					_id += random_range(1, 16);
					break;
				case 4526:
					_id += random_range(1, 15);
					break;
				case 4608:
					_id += random_range(1, 17);
					break;
				case 4691:
					_id += random_range(1, 10);
					break;
				case 5405:
				case 6804:
					_id += random_range(1, 5);
					break;
			}
		}
	}

	return Item::CreateItem(_id, _count);
}

bool Item::loadItem(xmlNodePtr node, Container* parent)
{
	if(xmlStrcmp(node->name, (const xmlChar*)"item"))
		return false;

	int32_t intValue;
	std::string strValue;

	Item* item = NULL;
	if(readXMLInteger(node, "id", intValue))
		item = Item::CreateItem(intValue);

	if(!item)
		return false;

	if(readXMLString(node, "attributes", strValue))
	{
		StringVec v, attr = explodeString(";", strValue);
		for(StringVec::iterator it = attr.begin(); it != attr.end(); ++it)
		{
			v = explodeString(",", (*it));
			if(v.size() < 2)
				continue;

			if(atoi(v[1].c_str()) || v[1] == "0")
				item->setAttribute(v[0], atoi(v[1].c_str()));
			else
				item->setAttribute(v[0], v[1]);
		}
	}

	//compatibility
	if(readXMLInteger(node, "subtype", intValue) || readXMLInteger(node, "subType", intValue))
		item->setSubType(intValue);

	if(readXMLInteger(node, "actionId", intValue) || readXMLInteger(node, "actionid", intValue)
		|| readXMLInteger(node, "aid", intValue))
		item->setActionId(intValue);

	if(readXMLInteger(node, "uniqueId", intValue) || readXMLInteger(node, "uniqueid", intValue)
		|| readXMLInteger(node, "uid", intValue))
		item->setUniqueId(intValue);

	if(readXMLString(node, "text", strValue))
		item->setText(strValue);

	if(item->getContainer())
		loadContainer(node, item->getContainer());

	if(parent)
		parent->addItem(item);

	return true;
}

bool Item::loadContainer(xmlNodePtr parentNode, Container* parent)
{
	xmlNodePtr node = parentNode->children;
	while(node)
	{
		if(node->type != XML_ELEMENT_NODE)
		{
			node = node->next;
			continue;
		}

		if(!xmlStrcmp(node->name, (const xmlChar*)"item") && !loadItem(node, parent))
			return false;

		node = node->next;
	}

	return true;
}

Item::Item(const uint16_t type, uint16_t amount/* = 0*/):
	ItemAttributes(), id(type)
{
	count = 1;
	raid = NULL;
	loadedFromMap = false;

	const ItemType& it = items[id];
	if(it.charges)
		setCharges(it.charges);

	setDefaultDuration();
	if(it.isFluidContainer() || it.isSplash())
		setFluidType(amount);
	else if(it.stackable && amount)
		count = amount;
	else if(it.charges && amount)
		setCharges(amount);
}

Item* Item::clone() const
{
	Item* tmp = Item::CreateItem(id, count);
	if(!tmp)
		return NULL;

	if(!attributes || attributes->empty())
		return tmp;

	tmp->createAttributes();
	*tmp->attributes = *attributes;
	return tmp;
}

void Item::copyAttributes(Item* item)
{
	if(item && item->attributes && !item->attributes->empty())
	{
		createAttributes();
		*attributes = *item->attributes;
	}

	eraseAttribute("decaying");
	eraseAttribute("duration");
}

void Item::onRemoved()
{
	if(raid)
	{
		raid->unRef();
		raid = NULL;
	}

	ScriptEnviroment::removeTempItem(this);
	if(getUniqueId())
		ScriptEnviroment::removeUniqueThing(this);
}

void Item::setDefaultSubtype()
{
	count = 1;
	const ItemType& it = items[id];
	if(it.charges)
		setCharges(it.charges);
}

void Item::setID(uint16_t newId)
{
	const ItemType& it = Item::items[newId];
	const ItemType& pit = Item::items[id];
	id = newId;

	uint32_t newDuration = it.decayTime * 1000;
	if(!newDuration && !it.stopTime && it.decayTo == -1)
	{
		eraseAttribute("decaying");
		eraseAttribute("duration");
	}

	eraseAttribute("corpseowner");
	if(newDuration > 0 && (!pit.stopTime || !hasIntegerAttribute("duration")))
	{
		setDecaying(DECAYING_FALSE);
		setDuration(newDuration);
	}
}

bool Item::floorChange(FloorChange_t change/* = CHANGE_NONE*/) const
{
	if(change < CHANGE_NONE)
		return Item::items[id].floorChange[change];

	for(int32_t i = CHANGE_PRE_FIRST; i < CHANGE_LAST; ++i)
	{
		if(Item::items[id].floorChange[i])
			return true;
	}

	return false;
}

Player* Item::getHoldingPlayer()
{
	Cylinder* p = getParent();
	while(p)
	{
		if(p->getCreature())
			return p->getCreature()->getPlayer();

		p = p->getParent();
	}

	return NULL;
}

const Player* Item::getHoldingPlayer() const
{
	return const_cast<Item*>(this)->getHoldingPlayer();
}

uint16_t Item::getSubType() const
{
	const ItemType& it = items[id];
	if(it.isFluidContainer() || it.isSplash())
		return getFluidType();

	if(it.charges)
		return getCharges();

	return count;
}

void Item::setSubType(uint16_t n)
{
	const ItemType& it = items[id];
	if(it.isFluidContainer() || it.isSplash())
		setFluidType(n);
	else if(it.charges)
		setCharges(n);
	else
		count = n;
}

Attr_ReadValue Item::readAttr(AttrTypes_t attr, PropStream& propStream)
{
	switch(attr)
	{
		case ATTR_COUNT:
		{
			uint8_t _count;
			if(!propStream.GET_UCHAR(_count))
				return ATTR_READ_ERROR;

			setSubType((uint16_t)_count);
			break;
		}

		case ATTR_ACTION_ID:
		{
			uint16_t aid;
			if(!propStream.GET_USHORT(aid))
				return ATTR_READ_ERROR;

			setAttribute("aid", aid);
			break;
		}

		case ATTR_UNIQUE_ID:
		{
			uint16_t uid;
			if(!propStream.GET_USHORT(uid))
				return ATTR_READ_ERROR;

			setUniqueId(uid);
			break;
		}

		case ATTR_NAME:
		{
			std::string name;
			if(!propStream.GET_STRING(name))
				return ATTR_READ_ERROR;

			setAttribute("name", name);
			break;
		}

		case ATTR_PLURALNAME:
		{
			std::string name;
			if(!propStream.GET_STRING(name))
				return ATTR_READ_ERROR;

			setAttribute("pluralname", name);
			break;
		}

		case ATTR_ARTICLE:
		{
			std::string article;
			if(!propStream.GET_STRING(article))
				return ATTR_READ_ERROR;

			setAttribute("article", article);
			break;
		}

		case ATTR_ATTACK:
		{
			int32_t attack;
			if(!propStream.GET_ULONG((uint32_t&)attack))
				return ATTR_READ_ERROR;

			setAttribute("attack", attack);
			break;
		}

		case ATTR_EXTRAATTACK:
		{
			int32_t attack;
			if(!propStream.GET_ULONG((uint32_t&)attack))
				return ATTR_READ_ERROR;

			setAttribute("extraattack", attack);
			break;
		}

		case ATTR_DEFENSE:
		{
			int32_t defense;
			if(!propStream.GET_ULONG((uint32_t&)defense))
				return ATTR_READ_ERROR;

			setAttribute("defense", defense);
			break;
		}

		case ATTR_EXTRADEFENSE:
		{
			int32_t defense;
			if(!propStream.GET_ULONG((uint32_t&)defense))
				return ATTR_READ_ERROR;

			setAttribute("extradefense", defense);
			break;
		}

		case ATTR_ARMOR:
		{
			int32_t armor;
			if(!propStream.GET_ULONG((uint32_t&)armor))
				return ATTR_READ_ERROR;

			setAttribute("armor", armor);
			break;
		}

		case ATTR_ATTACKSPEED:
		{
			int32_t attackSpeed;
			if(!propStream.GET_ULONG((uint32_t&)attackSpeed))
				return ATTR_READ_ERROR;

			setAttribute("attackspeed", attackSpeed);
			break;
		}

		case ATTR_HITCHANCE:
		{
			int32_t hitChance;
			if(!propStream.GET_ULONG((uint32_t&)hitChance))
				return ATTR_READ_ERROR;

			setAttribute("hitchance", hitChance);
			break;
		}

		case ATTR_SCRIPTPROTECTED:
		{
			uint8_t protection;
			if(!propStream.GET_UCHAR(protection))
				return ATTR_READ_ERROR;

			setAttribute("scriptprotected", protection != 0);
			break;
		}

		case ATTR_TEXT:
		{
			std::string text;
			if(!propStream.GET_STRING(text))
				return ATTR_READ_ERROR;

			setAttribute("text", text);
			break;
		}

		case ATTR_WRITTENDATE:
		{
			int32_t date;
			if(!propStream.GET_ULONG((uint32_t&)date))
				return ATTR_READ_ERROR;

			setAttribute("date", date);
			break;
		}

		case ATTR_WRITTENBY:
		{
			std::string writer;
			if(!propStream.GET_STRING(writer))
				return ATTR_READ_ERROR;

			setAttribute("writer", writer);
			break;
		}

		case ATTR_DESC:
		{
			std::string text;
			if(!propStream.GET_STRING(text))
				return ATTR_READ_ERROR;

			setAttribute("description", text);
			break;
		}

		case ATTR_RUNE_CHARGES:
		{
			uint8_t charges;
			if(!propStream.GET_UCHAR(charges))
				return ATTR_READ_ERROR;

			setSubType((uint16_t)charges);
			break;
		}

		case ATTR_CHARGES:
		{
			uint16_t charges;
			if(!propStream.GET_USHORT(charges))
				return ATTR_READ_ERROR;

			setSubType(charges);
			break;
		}

		case ATTR_DURATION:
		{
			int32_t duration;
			if(!propStream.GET_ULONG((uint32_t&)duration))
				return ATTR_READ_ERROR;

			setAttribute("duration", duration);
			break;
		}

		case ATTR_DECAYING_STATE:
		{
			uint8_t state;
			if(!propStream.GET_UCHAR(state))
				return ATTR_READ_ERROR;

			if((ItemDecayState_t)state != DECAYING_FALSE)
				setAttribute("decaying", (int32_t)DECAYING_PENDING);

			break;
		}

		//these should be handled through derived classes
		//if these are called then something has changed in the items.otb since the map was saved
		//just read the values

		//Depot class
		case ATTR_DEPOT_ID:
		{
			uint16_t depot;
			if(!propStream.GET_USHORT(depot))
				return ATTR_READ_ERROR;

			break;
		}

		//Door class
		case ATTR_HOUSEDOORID:
		{
			uint8_t door;
			if(!propStream.GET_UCHAR(door))
				return ATTR_READ_ERROR;

			break;
		}

		//Teleport class
		case ATTR_TELE_DEST:
		{
			TeleportDest* dest;
			if(!propStream.GET_STRUCT(dest))
				return ATTR_READ_ERROR;

			break;
		}

		//Bed class
		case ATTR_SLEEPERGUID:
		{
			uint32_t sleeper;
			if(!propStream.GET_ULONG(sleeper))
				return ATTR_READ_ERROR;

			break;
		}

		case ATTR_SLEEPSTART:
		{
			uint32_t sleepStart;
			if(!propStream.GET_ULONG(sleepStart))
				return ATTR_READ_ERROR;

			break;
		}

		//Container class
		case ATTR_CONTAINER_ITEMS:
		{
			uint32_t _count;
			propStream.GET_ULONG(_count);
			return ATTR_READ_ERROR;
		}

		//ItemAttributes class
		case ATTR_ATTRIBUTE_MAP:
		{
			bool unique = hasIntegerAttribute("uid"), ret = unserializeMap(propStream);
			if(!unique && hasIntegerAttribute("uid")) // unfortunately we have to do this
				ScriptEnviroment::addUniqueThing(this);

			// this attribute has a custom behavior as well
			if(getDecaying() != DECAYING_FALSE)
				setDecaying(DECAYING_PENDING);

			if(ret)
				break;
		}

		default:
			return ATTR_READ_ERROR;
	}

	return ATTR_READ_CONTINUE;
}

bool Item::unserializeAttr(PropStream& propStream)
{
	uint8_t attrType = ATTR_END;
	while(propStream.GET_UCHAR(attrType) && attrType != ATTR_END)
	{
		switch(readAttr((AttrTypes_t)attrType, propStream))
		{
			case ATTR_READ_ERROR:
				return false;

			case ATTR_READ_END:
				return true;

			default:
				break;
		}
	}

	return true;
}

bool Item::serializeAttr(PropWriteStream& propWriteStream) const
{
	if(isStackable() || isFluidContainer() || isSplash())
	{
		propWriteStream.ADD_UCHAR(ATTR_COUNT);
		propWriteStream.ADD_UCHAR((uint8_t)getSubType());
	}

	if(attributes && !attributes->empty())
	{
		propWriteStream.ADD_UCHAR(ATTR_ATTRIBUTE_MAP);
		serializeMap(propWriteStream);
	}

	return true;
}

bool Item::hasProperty(enum ITEMPROPERTY prop) const
{
	const ItemType& it = items[id];
	switch(prop)
	{
		case BLOCKSOLID:
			if(it.blockSolid)
				return true;

			break;

		case MOVEABLE:
			if(it.moveable && (!isLoadedFromMap() || (!getUniqueId()
				&& (!getActionId() || getContainer()))))
				return true;

			break;

		case HASHEIGHT:
			if(it.hasHeight)
				return true;

			break;

		case BLOCKPROJECTILE:
			if(it.blockProjectile)
				return true;

			break;

		case BLOCKPATH:
			if(it.blockPathFind)
				return true;

			break;

		case ISVERTICAL:
			if(it.isVertical)
				return true;

			break;

		case ISHORIZONTAL:
			if(it.isHorizontal)
				return true;

			break;

		case IMMOVABLEBLOCKSOLID:
			if(it.blockSolid && (!it.moveable || (isLoadedFromMap() &&
				(getUniqueId() || (getActionId() && getContainer())))))
				return true;

			break;

		case IMMOVABLEBLOCKPATH:
			if(it.blockPathFind && (!it.moveable || (isLoadedFromMap() &&
				(getUniqueId() || (getActionId() && getContainer())))))
				return true;

			break;

		case SUPPORTHANGABLE:
			if(it.isHorizontal || it.isVertical)
				return true;

			break;

		case IMMOVABLENOFIELDBLOCKPATH:
			if(!it.isMagicField() && it.blockPathFind && (!it.moveable || (isLoadedFromMap() &&
				(getUniqueId() || (getActionId() && getContainer())))))
				return true;

			break;

		case NOFIELDBLOCKPATH:
			if(!it.isMagicField() && it.blockPathFind)
				return true;

			break;

		default:
			break;
	}

	return false;
}

double Item::getWeight() const
{
	if(isStackable())
		return items[id].weight * std::max((int32_t)1, (int32_t)count);

	return items[id].weight;
}

std::string Item::getDescription(const ItemType& it, int32_t lookDistance, const Item* item/* = NULL*/,
	int32_t subType/* = -1*/, bool addArticle/* = true*/)
{
	std::stringstream s;
	s << getNameDescription(it, item, subType, addArticle);
	if(item)
		subType = item->getSubType();

	bool dot = true;
	if(it.isRune())
	{
		s << "(";
		if(!it.runeSpellName.empty())
			s << "\"" << it.runeSpellName << "\", ";

		s << "Charges:" << subType << ")";
		if(it.runeLevel > 0 || it.runeMagLevel > 0 || (it.vocationString != "" && it.wieldInfo == 0))
		{
			s << "." << std::endl << "It can only be used";
			if(it.vocationString != "" && it.wieldInfo == 0)
				s << " by " << it.vocationString;

			bool begin = true;
			if(g_config.getBool(ConfigManager::USE_RUNE_REQUIREMENTS) && it.runeLevel > 0)
			{
				begin = false;
				s << " with level " << it.runeLevel;
			}

			if(g_config.getBool(ConfigManager::USE_RUNE_REQUIREMENTS) && it.runeMagLevel > 0)
			{
				begin = false;
				s << " " << (begin ? "with" : "and") << " magic level " << it.runeMagLevel;
			}

			if(g_config.getBool(ConfigManager::USE_RUNE_REQUIREMENTS) && !begin)
				s << " or higher";
		}
	}
	else if(it.weaponType != WEAPON_NONE)
	{
		bool begin = true;
		if(it.weaponType == WEAPON_DIST && it.ammoType != AMMO_NONE)
		{
			begin = false;
			s << " (Range:" << int32_t(item ? item->getShootRange() : it.shootRange);
			if(it.attack || it.extraAttack || (item && (item->getAttack() || item->getExtraAttack())))
			{
				s << ", Atk " << std::showpos << int32_t(item ? item->getAttack() : it.attack);
				if(it.extraAttack || (item && item->getExtraAttack()))
					s << " " << std::showpos << int32_t(item ? item->getExtraAttack() : it.extraAttack) << std::noshowpos;
			}

			if(it.hitChance != -1 || (item && item->getHitChance() != -1))
				s << ", Hit% " << std::showpos << (item ? item->getHitChance() : it.hitChance) << std::noshowpos;
		}
		else if(it.weaponType != WEAPON_AMMO && it.weaponType != WEAPON_WAND)
		{
			if(it.attack || it.extraAttack || (item && (item->getAttack() || item->getExtraAttack())))
			{
				begin = false;
				s << " (Atk:";
				if(it.abilities.elementType != COMBAT_NONE && it.decayTo < 1)
				{
					s << std::max((int32_t)0, int32_t((item ? item->getAttack() : it.attack) - it.abilities.elementDamage));
					if(it.extraAttack || (item && item->getExtraAttack()))
						s << " " << std::showpos << int32_t(item ? item->getExtraAttack() : it.extraAttack) << std::noshowpos;

					s << " physical + " << it.abilities.elementDamage << " " << getCombatName(it.abilities.elementType);
				}
				else
				{
					s << int32_t(item ? item->getAttack() : it.attack);
					if(it.extraAttack || (item && item->getExtraAttack()))
						s << " " << std::showpos << int32_t(item ? item->getExtraAttack() : it.extraAttack) << std::noshowpos;
				}
			}

			if(it.defense || it.extraDefense || (item && (item->getDefense() || item->getExtraDefense())))
			{
				if(begin)
				{
					begin = false;
					s << " (";
				}
				else
					s << ", ";

				s << "Def:" << int32_t(item ? item->getDefense() : it.defense);
				if(it.extraDefense || (item && item->getExtraDefense()))
					s << " " << std::showpos << int32_t(item ? item->getExtraDefense() : it.extraDefense) << std::noshowpos;
			}
		}

		for(uint16_t i = SKILL_FIRST; i <= SKILL_LAST; i++)
		{
			if(!it.abilities.skills[i])
				continue;

			if(begin)
			{
				begin = false;
				s << " (";
			}
			else
				s << ", ";

			s << getSkillName(i) << " " << std::showpos << (int32_t)it.abilities.skills[i] << std::noshowpos;
		}

		if(it.abilities.stats[STAT_MAGICLEVEL])
		{
			if(begin)
			{
				begin = false;
				s << " (";
			}
			else
				s << ", ";

			s << "magic level " << std::showpos << (int32_t)it.abilities.stats[STAT_MAGICLEVEL] << std::noshowpos;
		}

		int32_t show = it.abilities.absorb[COMBAT_FIRST];
		for(uint32_t i = (COMBAT_FIRST + 1); i <= COMBAT_LAST; i++)
		{
			if(it.abilities.absorb[i] == show)
				continue;

			show = 0;
			break;
		}

		if(!show)
		{
			bool tmp = true;
			for(uint32_t i = COMBAT_FIRST; i <= COMBAT_LAST; i++)
			{
				if(!it.abilities.absorb[i])
					continue;

				if(tmp)
				{
					if(begin)
					{
						begin = false;
						s << " (";
					}
					else
						s << ", ";

					tmp = false;
					s << "protection ";
				}
				else
					s << ", ";

				s << getCombatName((CombatType_t)i) << " " << std::showpos << it.abilities.absorb[i] << std::noshowpos << "%";
			}
		}
		else
		{
			if(begin)
			{
				begin = false;
				s << " (";
			}
			else
				s << ", ";

			s << "protection all " << std::showpos << show << std::noshowpos << "%";
		}

		if(it.abilities.speed)
		{
			if(begin)
			{
				begin = false;
				s << " (";
			}
			else
				s << ", ";

			s << "speed " << std::showpos << (int32_t)(it.abilities.speed / 2) << std::noshowpos;
		}

		if(!begin)
			s << ")";
	}
	else if(it.armor || (item && item->getArmor()) || it.showAttributes)
	{
		int32_t tmp = it.armor;
		if(item)
			tmp = item->getArmor();

		bool begin = true;
		if(tmp)
		{
			s << " (Arm:" << tmp;
			begin = false;
		}

		for(uint16_t i = SKILL_FIRST; i <= SKILL_LAST; i++)
		{
			if(!it.abilities.skills[i])
				continue;

			if(begin)
			{
				begin = false;
				s << " (";
			}
			else
				s << ", ";

			s << getSkillName(i) << " " << std::showpos << (int32_t)it.abilities.skills[i] << std::noshowpos;
		}

		if(it.abilities.stats[STAT_MAGICLEVEL])
		{
			if(begin)
			{
				begin = false;
				s << " (";
			}
			else
				s << ", ";

			s << "magic level " << std::showpos << (int32_t)it.abilities.stats[STAT_MAGICLEVEL] << std::noshowpos;
		}

		int32_t show = it.abilities.absorb[COMBAT_FIRST];
		for(int32_t i = (COMBAT_FIRST + 1); i <= COMBAT_LAST; i++)
		{
			if(it.abilities.absorb[i] == show)
				continue;

			show = 0;
			break;
		}

		if(!show)
		{
			bool tmp = true;
			for(int32_t i = COMBAT_FIRST; i <= COMBAT_LAST; i++)
			{
				if(!it.abilities.absorb[i])
					continue;

				if(tmp)
				{
					tmp = false;
					if(begin)
					{
						begin = false;
						s << " (";
					}
					else
						s << ", ";

					s << "protection ";
				}
				else
					s << ", ";

				s << getCombatName((CombatType_t)i) << " " << std::showpos << it.abilities.absorb[i] << std::noshowpos << "%";
			}
		}
		else
		{
			if(begin)
			{
				begin = false;
				s << " (";
			}
			else
				s << ", ";

			s << "protection all " << std::showpos << show << std::noshowpos << "%";
		}

		show = it.abilities.reflect[REFLECT_CHANCE][COMBAT_FIRST];
		for(int32_t i = (COMBAT_FIRST + 1); i <= COMBAT_LAST; i++)
		{
			if(it.abilities.reflect[REFLECT_CHANCE][i] == show)
				continue;

			show = 0;
			break;
		}

		if(!show)
		{
			bool tmp = true;
			for(int32_t i = COMBAT_FIRST; i <= COMBAT_LAST; i++)
			{
				if(!it.abilities.reflect[REFLECT_CHANCE][i])
					continue;

				if(tmp)
				{
					tmp = false;
					if(begin)
					{
						begin = false;
						s << " (";
					}
					else
						s << ", ";

					s << "reflect ";
				}
				else
					s << ", ";

				std::string ss = "no";
				if(it.abilities.reflect[REFLECT_PERCENT][i] > 99)
					ss = "whole";
				else if(it.abilities.reflect[REFLECT_PERCENT][i] >= 75)
					ss = "huge";
				else if(it.abilities.reflect[REFLECT_PERCENT][i] >= 50)
					ss = "medium";
				else if(it.abilities.reflect[REFLECT_PERCENT][i] >= 25)
					ss = "small";
				else if(it.abilities.reflect[REFLECT_PERCENT][i] > 0)
					ss = "tiny";

				s << getCombatName((CombatType_t)i) << " " << std::showpos << it.abilities.reflect[REFLECT_PERCENT][i] << std::noshowpos << "% for " << ss;
			}

			if(!tmp)
				s << " damage";
		}
		else
		{
			if(begin)
			{
				begin = false;
				s << " (";
			}
			else
				s << ", ";

			s << "reflect all " << std::showpos << show << std::noshowpos << "% for mixed damage";
		}

		if(it.abilities.speed)
		{
			if(begin)
			{
				begin = false;
				s << " (";
			}
			else
				s << ", ";

			s << "speed " << std::showpos << (int32_t)(it.abilities.speed / 2) << std::noshowpos;
		}

		if(!begin)
			s << ")";
	}
	else if(it.isContainer())
		s << " (Vol:" << (int32_t)it.maxItems << ")";
	else if(it.isKey())
		s << " (Key:" << (item ? (int32_t)item->getActionId() : 0) << ")";
	else if(it.isFluidContainer())
	{
		if(subType > 0)
			s << " of " << (items[subType].name.length() ? items[subType].name : "unknown");
		else
			s << ". It is empty";
	}
	else if(it.isSplash())
	{
		s << " of ";
		if(subType > 0 && items[subType].name.length())
			s << items[subType].name;
		else
			s << "unknown";
	}
	else if(it.allowDistRead)
	{
		s << std::endl;
		if(item && !item->getText().empty())
		{
			if(lookDistance <= 4)
			{
				if(!item->getWriter().empty())
				{
					s << item->getWriter() << " wrote";
					time_t date = item->getDate();
					if(date > 0)
						s << " on " << formatDate(date);

					s << ": ";
				}
				else
					s << "You read: ";

				std::string text = item->getText();
				s << text;
				if(!text.empty())
				{
					char end = *text.rbegin();
					if(end == '?' || end == '!' || end == '.')
						dot = false;
				}
			}
			else
				s << "You are too far away to read it";
		}
		else
			s << "Nothing is written on it";
	}
	else if(it.levelDoor && item && item->getActionId() >= (int32_t)it.levelDoor && item->getActionId()
		<= ((int32_t)it.levelDoor + g_config.getNumber(ConfigManager::MAXIMUM_DOOR_LEVEL)))
		s << " for level " << item->getActionId() - it.levelDoor;

	if(it.showCharges)
		s << " that has " << subType << " charge" << (subType != 1 ? "s" : "") << " left";

	if(it.showDuration)
	{
		if(item && item->hasIntegerAttribute("duration"))
		{
			int32_t duration = item->getDuration() / 1000;
			s << " that has energy for ";

			if(duration >= 120)
				s << duration / 60 << " minutes left";
			else if(duration > 60)
				s << "1 minute left";
			else
				s << " less than a minute left";
		}
		else
			s << " that is brand-new";
	}

	if(dot)
		s << ".";

	if(it.wieldInfo)
	{
		s << std::endl << "It can only be wielded properly by ";
		if(it.wieldInfo & WIELDINFO_PREMIUM)
			s << "premium ";

		if(it.wieldInfo & WIELDINFO_VOCREQ)
			s << it.vocationString;
		else
			s << "players";

		if(it.wieldInfo & WIELDINFO_LEVEL)
			s << " of level " << (int32_t)it.minReqLevel << " or higher";

		if(it.wieldInfo & WIELDINFO_MAGLV)
		{
			if(it.wieldInfo & WIELDINFO_LEVEL)
				s << " and";
			else
				s << " of";

			s << " magic level " << (int32_t)it.minReqMagicLevel << " or higher";
		}

		s << ".";
	}

	if(lookDistance <= 1 && it.pickupable)
	{
		std::string tmp;
		if(!item)
			tmp = getWeightDescription(it.weight, it.stackable, subType);
		else
			tmp = item->getWeightDescription();

		if(!tmp.empty())
			s << std::endl << tmp;
	}

	if(it.abilities.elementType != COMBAT_NONE && it.decayTo > 0)
	{
		s << std::endl << "It is temporarily enchanted with " << getCombatName(it.abilities.elementType) << " (";
		s << std::max((int32_t)0, int32_t((item ? item->getAttack() : it.attack) - it.abilities.elementDamage));
		if(it.extraAttack || (item && item->getExtraAttack()))
			s << " " << std::showpos << int32_t(item ? item->getExtraAttack() : it.extraAttack) << std::noshowpos;

		s << " physical + " << it.abilities.elementDamage << " " << getCombatName(it.abilities.elementType) << " damage).";
	}

	std::string str;
	if(item && !item->getSpecialDescription().empty())
		str = item->getSpecialDescription();
	else if(!it.description.empty() && lookDistance <= 1)
		str = it.description;

	if(str.empty())
		return s.str();

	if(str.find("|PLAYERNAME|") != std::string::npos)
	{
		std::string tmp = "You";
		if(item)
		{
			if(const Player* player = item->getHoldingPlayer())
				tmp = player->getName();
		}

		replaceString(str, "|PLAYERNAME|", tmp);
	}

	if(str.find("|TIME|") != std::string::npos || str.find("|DATE|") != std::string::npos || str.find(
		"|DAY|") != std::string::npos || str.find("|MONTH|") != std::string::npos || str.find(
		"|YEAR|") != std::string::npos || str.find("|HOUR|") != std::string::npos || str.find(
		"|MINUTES|") != std::string::npos || str.find("|SECONDS|") != std::string::npos ||
		str.find("|WEEKDAY|") != std::string::npos || str.find("|YEARDAY|") != std::string::npos)
	{
		time_t now = time(NULL);
		tm* ts = localtime(&now);

		std::stringstream ss;
		ss << ts->tm_sec;
		replaceString(str, "|SECONDS|", ss.str());

		ss.str("");
		ss << ts->tm_min;
		replaceString(str, "|MINUTES|", ss.str());

		ss.str("");
		ss << ts->tm_hour;
		replaceString(str, "|HOUR|", ss.str());

		ss.str("");
		ss << ts->tm_mday;
		replaceString(str, "|DAY|", ss.str());

		ss.str("");
		ss << (ts->tm_mon + 1);
		replaceString(str, "|MONTH|", ss.str());

		ss.str("");
		ss << (ts->tm_year + 1900);
		replaceString(str, "|YEAR|", ss.str());

		ss.str("");
		ss << ts->tm_wday;
		replaceString(str, "|WEEKDAY|", ss.str());

		ss.str("");
		ss << ts->tm_yday;
		replaceString(str, "|YEARDAY|", ss.str());

		ss.str("");
		ss << ts->tm_hour << ":" << ts->tm_min << ":" << ts->tm_sec;
		replaceString(str, "|TIME|", ss.str());

		ss.str("");
		replaceString(str, "|DATE|", formatDateEx(now));
	}

	s << std::endl << str;
	return s.str();
}

std::string Item::getNameDescription(const ItemType& it, const Item* item/* = NULL*/, int32_t subType/* = -1*/, bool addArticle/* = true*/)
{
	if(item)
		subType = item->getSubType();

	std::stringstream s;
	if(it.name.length() || (item && item->getName().length()))
	{
		if(subType > 1 && it.stackable && it.showCount)
			s << subType << " " << (item ? item->getPluralName() : it.pluralName);
		else
		{
			if(addArticle)
			{
				if(item && !item->getArticle().empty())
					s << item->getArticle() << " ";
				else if(!it.article.empty())
					s << it.article << " ";
			}

			s << (item ? item->getName() : it.name);
		}
	}
	else
		s << "an item of type " << it.id << ", please report it to gamemaster";

	return s.str();
}

std::string Item::getWeightDescription(double weight, bool stackable, uint32_t count/* = 1*/)
{
	if(weight <= 0)
		return "";

	std::stringstream s;
	if(stackable && count > 1)
		s << "They weigh " << std::fixed << std::setprecision(2) << weight << " oz.";
	else
		s << "It weighs " << std::fixed << std::setprecision(2) << weight << " oz.";

	return s.str();
}

void Item::setActionId(int32_t aid, bool callEvent/* = true*/)
{
	Tile* tile = NULL;
	if(callEvent)
		tile = getTile();

	if(tile && getActionId())
		g_moveEvents->onRemoveTileItem(tile, this);

	setAttribute("aid", aid);
	if(tile)
		g_moveEvents->onAddTileItem(tile, this);
}

void Item::resetActionId(bool callEvent/* = true*/)
{
	if(!getActionId())
		return;

	Tile* tile = NULL;
	if(callEvent)
		tile = getTile();

	eraseAttribute("aid");
	if(tile)
		g_moveEvents->onAddTileItem(tile, this);
}

void Item::setUniqueId(int32_t uid)
{
	if(getUniqueId())
		return;

	setAttribute("uid", uid);
	ScriptEnviroment::addUniqueThing(this);
}

bool Item::canDecay()
{
	if(isRemoved())
		return false;

	if(isLoadedFromMap() && (getUniqueId() || (getActionId() && getContainer())))
		return false;

	const ItemType& it = Item::items[id];
	return it.decayTo >= 0 && it.decayTime;
}

void Item::getLight(LightInfo& lightInfo)
{
	const ItemType& it = items[id];
	lightInfo.color = it.lightColor;
	lightInfo.level = it.lightLevel;
}

void Item::__startDecaying()
{
	g_game.startDecay(this);
}
