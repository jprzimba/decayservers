//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Beds
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

#include "beds.h"

#include "house.h"
#include "iologindata.h"
#include "game.h"
#include "player.h"

extern Game g_game;

BedItem::BedItem(uint16_t _id) : Item(_id)
{
	house = NULL;
	internalRemoveSleeper();
}

Attr_ReadValue BedItem::readAttr(AttrTypes_t attr, PropStream& propStream)
{
	switch(attr)
	{
		case ATTR_SLEEPERGUID:
		{
			uint32_t _guid;
			if(!propStream.GET_ULONG(_guid))
				return ATTR_READ_ERROR;

			if(_guid != 0)
			{
				std::string name;
				if(IOLoginData::getInstance()->getNameByGuid(_guid, name))
				{
					setSpecialDescription(name + " is sleeping there.");
					Beds::getInstance().setBedSleeper(this, _guid);
				}
			}

			sleeperGUID = _guid;
			return ATTR_READ_CONTINUE;
		}

		case ATTR_SLEEPSTART:
		{
			uint32_t sleep_start;
			if(!propStream.GET_ULONG(sleep_start))
				return ATTR_READ_ERROR;

			sleepStart = (uint64_t)sleep_start;
			return ATTR_READ_CONTINUE;
		}

		default:
			break;
	}
	return Item::readAttr(attr, propStream);
}

bool BedItem::serializeAttr(PropWriteStream& propWriteStream)
{
	if(sleeperGUID != 0)
	{
		propWriteStream.ADD_UCHAR(ATTR_SLEEPERGUID);
		propWriteStream.ADD_ULONG(sleeperGUID);
	}

	if(sleepStart != 0)
	{
		propWriteStream.ADD_UCHAR(ATTR_SLEEPSTART);
		propWriteStream.ADD_ULONG((int32_t)sleepStart);
	}
	return true;
}

BedItem* BedItem::getNextBedItem()
{
	Direction dir = Item::items[getID()].bedPartnerDir;
	Position targetPos = getNextPosition(dir, getPosition());

	Tile* tile = g_game.getMap()->getTile(targetPos);
	if(tile != NULL)
		return tile->getBedItem();

	return NULL;
}

bool BedItem::canUse(Player* player)
{
	if(player == NULL || house == NULL || !player->isPremium())
		return false;
	else if(player->hasCondition(CONDITION_INFIGHT))
		return false;
	else if(sleeperGUID != 0)
	{
		if(house->getHouseAccessLevel(player) != HOUSE_OWNER)
		{
			std::string name;
			if(IOLoginData::getInstance()->getNameByGuid(sleeperGUID, name))
			{
				Player* sleeper = new Player(name, NULL);
				if(IOLoginData::getInstance()->loadPlayer(sleeper, name))
				{
					if(house->getHouseAccessLevel(sleeper) <= house->getHouseAccessLevel(player))
					{
						delete sleeper;
						sleeper = NULL;

						return isBed();
					}

					delete sleeper;
					sleeper = NULL;
				}
			}
			return false;
		}
	}
	else
		return isBed();

	return true;
}

void BedItem::sleep(Player* player)
{
	// avoid crashes
	if((house == NULL) || (player == NULL) || player->isRemoved())
		return;

	if(sleeperGUID != 0)
	{
		if(Item::items[getID()].transformToFree != 0 && house->getHouseOwner() == player->getGUID())
			wakeUp(NULL);

		g_game.addMagicEffect(player->getPosition(), NM_ME_POFF);
	}
	else
	{
		BedItem* nextBedItem = getNextBedItem();

		internalSetSleeper(player);
		if(nextBedItem)
			nextBedItem->internalSetSleeper(player);

		// update the BedSleepersMap
		Beds::getInstance().setBedSleeper(this, player->getGUID());

		// make the player walk onto the bed
		player->getTile()->moveCreature(player, getTile());

		// kick player after he sees himself walk onto the bed and it change id
		uint32_t playerId = player->getID();
		Scheduler::getScheduler().addEvent(createSchedulerTask(SCHEDULER_MINTICKS, boost::bind(&Game::kickPlayer, &g_game, playerId, false)));

		// change self and partner's appearance
		updateAppearance(player);
		if(nextBedItem)
			nextBedItem->updateAppearance(player);
	}
}

void BedItem::wakeUp(Player* player)
{
	// avoid crashes
	if(house == NULL)
		return;

	if(sleeperGUID != 0)
	{
		if(player == NULL)
		{
			std::string name;
			if(IOLoginData::getInstance()->getNameByGuid(sleeperGUID, name))
			{
				Player* _player = new Player(name, NULL);
				if(IOLoginData::getInstance()->loadPlayer(_player, name))
				{
					regeneratePlayer(_player);
					IOLoginData::getInstance()->savePlayer(_player, true);
				}

				delete _player;
				_player = NULL;
			}
		}
		else
		{
			regeneratePlayer(player);
			g_game.addCreatureHealth(player);
		}
	}

	// update the BedSleepersMap
	Beds::getInstance().setBedSleeper(NULL, sleeperGUID);

	BedItem* nextBedItem = getNextBedItem();

	// unset sleep info
	internalRemoveSleeper();
	if(nextBedItem)
		nextBedItem->internalRemoveSleeper();

	// change self and partner's appearance
	updateAppearance(NULL);
	if(nextBedItem)
		nextBedItem->updateAppearance(NULL);
}

void BedItem::regeneratePlayer(Player* player) const
{
	const uint32_t sleptTime = time(NULL) - sleepStart;

	Condition* condition = player->getCondition(CONDITION_REGENERATION, CONDITIONID_DEFAULT);
	if(condition)
	{
		uint32_t regen;
		if(condition->getTicks() != -1)
		{
			regen = std::min<int32_t>((condition->getTicks() / 1000), sleptTime) / 30;
			const int32_t newRegenTicks = condition->getTicks() - (regen * 30000);
			if(newRegenTicks <= 0)
				player->removeCondition(condition);
			else
				condition->setTicks(newRegenTicks);
		}
		else
			regen = sleptTime / 30;

		player->changeHealth(regen);
		player->changeMana(regen);
	}

	const int32_t soulRegen = sleptTime / (60 * 15);
	player->changeSoul(soulRegen);
}

void BedItem::updateAppearance(const Player* player)
{
	const ItemType& it = Item::items[getID()];
	if(it.type == ITEM_TYPE_BED)
	{
		if(player && it.transformToOnUse[player->getSex()] != 0)
		{
			const ItemType& newType = Item::items[it.transformToOnUse[player->getSex()]];
			if(newType.type == ITEM_TYPE_BED)
				g_game.transformItem(this, it.transformToOnUse[player->getSex()]);
		}
		else if(it.transformToFree != 0)
		{
			const ItemType& newType = Item::items[it.transformToFree];
			if(newType.type == ITEM_TYPE_BED)
				g_game.transformItem(this, it.transformToFree);
		}
	}
}

void BedItem::internalSetSleeper(const Player* player)
{
	std::string desc_str = player->getName() + " is sleeping there.";

	setSleeper(player->getGUID());
	setSleepStart(time(NULL));
	setSpecialDescription(desc_str);
}

void BedItem::internalRemoveSleeper()
{
	setSleeper(0);
	setSleepStart(0);
	setSpecialDescription("Nobody is sleeping there.");
}

BedItem* Beds::getBedBySleeper(uint32_t guid)
{
	std::map<uint32_t, BedItem*>::iterator it = BedSleepersMap.find(guid);
	if(it != BedSleepersMap.end())
		return it->second;

	return NULL;
}

void Beds::setBedSleeper(BedItem* bed, uint32_t guid)
{
	BedSleepersMap[guid] = bed;
}
