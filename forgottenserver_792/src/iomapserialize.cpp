//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// SQL map serialization
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

#include "iomapserialize.h"
#include "house.h"
#include "configmanager.h"
#include "luascript.h"
#include "game.h"

extern ConfigManager g_config;
extern Game g_game;

bool IOMapSerialize::loadMap(Map* map)
{
	Database* db = Database::getInstance();

	for(HouseMap::iterator it = Houses::getInstance().getHouseBegin(); it != Houses::getInstance().getHouseEnd(); ++it)
	{
		//load tile
		House* house = it->second;
		for(HouseTileList::iterator it = house->getHouseTileBegin(); it != house->getHouseTileEnd(); ++it){
			loadTile(*db, *it);
		}
	}

	return true;
}

bool IOMapSerialize::saveMap(Map* map)
{
	Database* db = Database::getInstance();

	//Start the transaction
	DBTransaction transaction(db);
	if(!transaction.begin())
		return false;

	//clear old tile data
	DBQuery query;
	query << "DELETE FROM `tile_items`;";
	if(!db->executeQuery(query.str()))
		return false;

	query.str("");
	query << "DELETE FROM `tiles`;";
	if(!db->executeQuery(query.str()))
		return false;

	uint32_t tileId = 0;
	for(HouseMap::iterator it = Houses::getInstance().getHouseBegin(); it != Houses::getInstance().getHouseEnd(); ++it)
	{
		//save house items
		House* house = it->second;
		for(HouseTileList::iterator it = house->getHouseTileBegin(); it != house->getHouseTileEnd(); ++it)
		{
			++tileId;
			saveTile(db, tileId, *it);
		}
	}
	//End the transaction
	return transaction.commit();
}

bool IOMapSerialize::saveTile(Database* db, uint32_t tileId, const Tile* tile)
{
	typedef std::list<std::pair<Container*, int32_t> > ContainerStackList;
	typedef ContainerStackList::value_type ContainerStackList_Pair;
	ContainerStackList containerStackList;

	bool storedTile = false;
	int32_t runningID = 0;
	Item* item = NULL;
	Container* container = NULL;

	int32_t parentid = 0;
	std::stringstream streamitems;
	std::string itemsstring;

	DBInsert query_insert(db);
	query_insert.setQuery("INSERT INTO `tile_items` (`tile_id`, `sid`, `pid`, `itemtype`, `count`, `attributes`) VALUES ");
	for(uint32_t i = 0; i < tile->getThingCount(); ++i)
	{
		item = tile->__getThing(i)->getItem();
		if(!item)
			continue;

		if(!(!item->isNotMoveable() ||
			item->getDoor() ||
			(item->getContainer() && item->getContainer()->size() != 0) ||
			(item->canWriteText())
			|| item->getBed()))
			continue;

		//only save beds in houses
		if(item->getBed() && !tile->hasFlag(TILESTATE_HOUSE))
			continue;

		if(!storedTile)
		{
			DBQuery tileListQuery;
			const Position& tilePos = tile->getPosition();
			tileListQuery << "INSERT INTO `tiles` (`id`, `x`, `y`, `z`) VALUES";
			tileListQuery << "(" << tileId << ", " << tilePos.x << ", " << tilePos.y << ", " << tilePos.z << ")";

			if(!db->executeQuery(tileListQuery.str()))
				return false;

			storedTile = true;
		}
		++runningID;

		uint32_t attributesSize;

		PropWriteStream propWriteStream;
		item->serializeAttr(propWriteStream);
		const char* attributes = propWriteStream.getStream(attributesSize);

		streamitems << tileId << ", " << runningID << ", " << parentid << ", " << item->getID() << ", "
			<< (int32_t)item->getSubType() << ", " << db->escapeBlob(attributes, attributesSize);

		if(!query_insert.addRow(streamitems.str()))
			return false;

		streamitems.str("");

		if(item->getContainer())
			containerStackList.push_back(ContainerStackList_Pair(item->getContainer(), runningID));
	}

	while(containerStackList.size() > 0)
	{
		ContainerStackList_Pair csPair = containerStackList.front();
		container = csPair.first;
		parentid = csPair.second;
		containerStackList.pop_front();

		for(ItemList::const_iterator it = container->getItems(); it != container->getEnd(); ++it)
		{
			item = (*it);
			++runningID;
			if(item->getContainer())
				containerStackList.push_back(ContainerStackList_Pair(item->getContainer(), runningID));

			uint32_t attributesSize;

			PropWriteStream propWriteStream;
			item->serializeAttr(propWriteStream);
			const char* attributes = propWriteStream.getStream(attributesSize);

			streamitems << tileId << ", " << runningID << ", " << parentid << ", " << item->getID() << ", "
				<< (int32_t)item->getSubType() << ", " << db->escapeBlob(attributes, attributesSize);

			if(!query_insert.addRow(streamitems.str()))
				return false;

			streamitems.str("");
		}
	}
	if(!query_insert.execute())
		return false;

	return true;
}

bool IOMapSerialize::loadTile(Database& db, Tile* tile)
{
	typedef std::map<int,std::pair<Item*,int> > ItemMap;
	ItemMap itemMap;

	const Position& tilePos = tile->getPosition();

	DBQuery query;
	query.str("");
	query << "SELECT `tiles`.`id` FROM `tiles` WHERE `x` = " << tilePos.x << " AND `y` = " << tilePos.y << " AND `z` = " << tilePos.z;

	DBResult* result;
	if(!(result = db.storeQuery(query.str())))
		return false;

	int32_t tileId = result->getDataInt("id");
	db.freeResult(result);
	query.str("");
	query << "SELECT * FROM `tile_items` WHERE `tile_id` = " << tileId << " ORDER BY `sid` DESC;";
	if((result = db.storeQuery(query.str())))
	{
		Item* item = NULL;
		do
		{
			int32_t sid = result->getDataInt("sid");
			int32_t pid = result->getDataInt("pid");
			int32_t type = result->getDataInt("itemtype");
			int32_t count = result->getDataInt("count");
			item = NULL;

			unsigned long attrSize = 0;
			const char* attr = result->getDataStream("attributes", attrSize);
			PropStream propStream;
			propStream.init(attr, attrSize);

			const ItemType& iType = Item::items[type];
			if(iType.moveable || /* or object in a container*/ pid != 0)
			{
				//create a new item
				item = Item::CreateItem(type, count);
				if(item)
				{
					if(!item->unserializeAttr(propStream))
						std::clog << "WARNING: Serialize error in IOMapSerialize::loadTile() [1]" << std::endl;

					if(pid == 0)
					{
						tile->__internalAddThing(item);
						item->__startDecaying();
					}
				}
				else
					continue;
			}
			else
			{
				//find this type in the tile
				for(uint32_t i = 0; i < tile->getThingCount(); ++i)
				{
					Item* findItem = tile->__getThing(i)->getItem();
					if(!findItem)
						continue;

					if(findItem->getID() == type || (iType.isDoor() && findItem->getDoor()) ||
						(iType.isBed() && findItem->getBed()))
					{
						item = findItem;
						break;
					}
				}
			}

			if(item)
			{
				if(!item->unserializeAttr(propStream))
					std::clog << "WARNING: Serialize error in IOMapSerialize::loadTile() [0]" << std::endl;

				item = g_game.transformItem(item, type);

				std::pair<Item*, int32_t> myPair(item, pid);
				itemMap[sid] = myPair;
			}
			else
				std::clog << "WARNING: IOMapSerialize::loadTile() - NULL item at " << tile->getPosition() << " (type = " << type << ", sid = " << sid << ", pid = " << pid << ")." << std::endl;
		}
		while(result->next());
		db.freeResult(result);
	}

	ItemMap::reverse_iterator it;
	ItemMap::iterator it2;
	for(it = itemMap.rbegin(); it != itemMap.rend(); ++it)
	{
		Item* item = it->second.first;
		int32_t pid = it->second.second;
		it2 = itemMap.find(pid);
		if(it2 != itemMap.end())
		{
			if(Container* container = it2->second.first->getContainer())
			{
				container->__internalAddThing(item);
				g_game.startDecay(item);
			}
		}
	}
	return true;
}

bool IOMapSerialize::loadHouseInfo(Map* map)
{
	Database* db = Database::getInstance();
	DBQuery query;

	DBResult* result = db->storeQuery("SELECT `id`, `owner`, `paid`, `warnings` FROM `houses`");
	if(!result)
		return false;

	do
	{
		House* house = Houses::getInstance().getHouse(result->getDataInt("id"));
		if(house)
		{
			house->setHouseOwner(result->getDataInt("owner"));
			house->setPaidUntil(result->getDataInt("paid"));
			house->setPayRentWarnings(result->getDataInt("warnings"));
		}
	}
	while(result->next());
	db->freeResult(result);

	for(HouseMap::iterator it = Houses::getInstance().getHouseBegin(); it != Houses::getInstance().getHouseEnd(); ++it)
	{
		House* house = it->second;
		if(house->getHouseOwner() != 0 && house->getHouseId() != 0)
		{
			query.str("");
			query << "SELECT `listid`, `list` FROM `house_lists` WHERE `house_id` = " << house->getHouseId();
			if((result = db->storeQuery(query.str())))
			{
				do
				{
					house->setAccessList(result->getDataInt("listid"), result->getDataString("list"));
				}
				while(result->next());
				db->freeResult(result);
			}
		}
	}
	return true;
}

bool IOMapSerialize::saveHouseInfo(Map* map)
{
	Database* db = Database::getInstance();
	DBQuery query;

	DBTransaction transaction(db);
	if(!transaction.begin())
		return false;

	if(!db->executeQuery("DELETE FROM `houses`;"))
		return false;

	if(!db->executeQuery("DELETE FROM `house_lists`;"))
		return false;

	DBInsert stmt(db);
	stmt.setQuery("INSERT INTO `houses` (`id` , `owner` , `paid`, `warnings`) VALUES ");
	for(HouseMap::iterator it = Houses::getInstance().getHouseBegin(); it != Houses::getInstance().getHouseEnd(); ++it)
	{
		House* house = it->second;

		query << house->getHouseId() << "," << house->getHouseOwner() << "," << house->getPaidUntil() << "," << house->getPayRentWarnings();
		if(!stmt.addRow(query))
			return false;
	}
	
	if(!stmt.execute())
		return false;

	stmt.setQuery("INSERT INTO `house_lists` (`house_id` , `listid` , `list`) VALUES ");
	for(HouseMap::iterator it = Houses::getInstance().getHouseBegin(); it != Houses::getInstance().getHouseEnd(); ++it)
	{
		House* house = it->second;

		std::string listText;
		if(house->getAccessList(GUEST_LIST, listText) && listText != "")
		{
			query << house->getHouseId() << "," << GUEST_LIST << "," << db->escapeString(listText);
			if(!stmt.addRow(query))
				return false;
		}

		if(house->getAccessList(SUBOWNER_LIST, listText) && listText != "")
		{
			query << house->getHouseId() << "," << SUBOWNER_LIST << "," << db->escapeString(listText);
			if(!stmt.addRow(query))
				return false;
		}

		for(HouseDoorList::iterator it = house->getHouseDoorBegin(); it != house->getHouseDoorEnd(); ++it)
		{
			const Door* door = *it;
			if(door->getAccessList(listText) && listText != "")
			{
				query << house->getHouseId() << "," << door->getDoorId() << "," << db->escapeString(listText);
				if(!stmt.addRow(query))
					return false;
			}
		}
	}

	if(!stmt.execute())
		return false;

	return transaction.commit();
}
