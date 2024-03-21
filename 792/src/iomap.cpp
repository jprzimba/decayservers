//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// OTBM map loader
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

#include "iomap.h"
#include "game.h"
#include "map.h"

#include "tile.h"
#include "item.h"
#include "container.h"
#include "depot.h"
#include "teleport.h"
#include "fileloader.h"
#include "town.h"

#include "beds.h"

typedef uint8_t attribute_t;
typedef uint32_t flags_t;

extern Game g_game;

/*
	OTBM_ROOTV1
	|
	|--- OTBM_MAP_DATA
	|	|
	|	|--- OTBM_TILE_AREA
	|	|	|--- OTBM_TILE
	|	|	|--- OTBM_TILE_SQUARE (not implemented)
	|	|	|--- OTBM_TILE_REF (not implemented)
	|	|	|--- OTBM_HOUSETILE
	|	|
	|	|--- OTBM_SPAWNS (not implemented)
	|	|	|--- OTBM_SPAWN_AREA (not implemented)
	|	|	|--- OTBM_MONSTER (not implemented)
	|	|
	|	|--- OTBM_TOWNS
	|		|--- OTBM_TOWN
	|
	|--- OTBM_ITEM_DEF (not implemented)
*/

bool IOMap::loadMap(Map* map, const std::string& identifier)
{
	int64_t start = OTSYS_TIME();

	FileLoader f;
	if(!f.openFile(identifier.c_str(), false, true))
	{
		std::ostringstream ss;
		ss << "Could not open the file " << identifier << ".";
		setLastErrorString(ss.str());
		return false;
	}
	
	uint32_t type;
	PropStream propStream;

	NODE root = f.getChildNode((NODE)NULL, type);

	if(!f.getProps(root, propStream))
	{
		setLastErrorString("Could not read root property.");
		return false;
	}

	OTBM_root_header* root_header;
	if(!propStream.GET_STRUCT(root_header))
	{
		setLastErrorString("Could not read header.");
		return false;
	}
	
	if(root_header->version != 0)
	{
		setLastErrorString("Unknown OTBM version detected.");
		return false;
	}

	if(root_header->majorVersionItems > (uint32_t)Items::dwMajorVersion)
	{
		setLastErrorString("The map was saved with a different items.otb version, an upgraded items.otb is required.");
		return false;
	}

	if(root_header->minorVersionItems > (uint32_t)Items::dwMinorVersion)
		std::clog << "Warning: [OTBM loader] This map needs an updated items OTB file." <<std::endl;

	std::clog << "> Map size: " << root_header->width << "x" << root_header->height << "." << std::endl;
	map->mapWidth = root_header->width;
	map->mapHeight = root_header->height;

	NODE nodeMap = f.getChildNode(root, type);
	
	if(type != OTBM_MAP_DATA)
	{
		setLastErrorString("Could not read data node.");
		return false;
	}

	if(!f.getProps(nodeMap, propStream))
	{
		setLastErrorString("Could not read map data attributes.");
		return false;
	}

	unsigned char attribute;
	std::string mapDescription;
	std::string tmp;
	while(propStream.GET_UCHAR(attribute))
	{
		switch(attribute)
		{
			case OTBM_ATTR_DESCRIPTION:
				if(!propStream.GET_STRING(mapDescription))
				{
					setLastErrorString("Invalid description tag.");
					return false;
				}
				break;
			case OTBM_ATTR_EXT_SPAWN_FILE:
				if(!propStream.GET_STRING(tmp))
				{
					setLastErrorString("Invalid spawn tag.");
					return false;
				}
				map->spawnfile = identifier.substr(0, identifier.rfind('/') + 1);
				map->spawnfile += tmp;
				break;

			case OTBM_ATTR_EXT_HOUSE_FILE:
				if(!propStream.GET_STRING(tmp))
				{
					setLastErrorString("Invalid house tag.");
					return false;
				}
				map->housefile = identifier.substr(0, identifier.rfind('/') + 1);
				map->housefile += tmp;
				break;

			default:
				setLastErrorString("Unknown header node.");
				return false;
		}
	}
	
	Tile* tile = NULL;

	NODE nodeMapData = f.getChildNode(nodeMap, type);
	while(nodeMapData != NO_NODE)
	{
		if(f.getError() != ERROR_NONE)
		{
			setLastErrorString("Invalid map node.");
			return false;
		}

		if(type == OTBM_TILE_AREA)
		{
			if(!f.getProps(nodeMapData, propStream))
			{
				setLastErrorString("Invalid map node.");
				return false;
			}
			
			OTBM_Tile_area_coords* area_coord;
			if(!propStream.GET_STRUCT(area_coord))
			{
				setLastErrorString("Invalid map node.");
				return false;
			}
			
			int32_t base_x, base_y, base_z;
			base_x = area_coord->_x;
			base_y = area_coord->_y;
			base_z = area_coord->_z;
			
			NODE nodeTile = f.getChildNode(nodeMapData, type);
			while(nodeTile != NO_NODE)
			{
				if(f.getError() != ERROR_NONE)
				{
					setLastErrorString("Could not read node data.");
					return false;
				}

				if(type == OTBM_TILE || type == OTBM_HOUSETILE)
				{
					if(!f.getProps(nodeTile, propStream))
					{
						setLastErrorString("Could not read node data.");
						return false;
					}
					
					unsigned short px, py, pz;
					OTBM_Tile_coords* tile_coord;
					if(!propStream.GET_STRUCT(tile_coord))
					{
						setLastErrorString("Could not read tile position.");
						return false;
					}

					px = base_x + tile_coord->_x;
					py = base_y + tile_coord->_y;
					pz = base_z;
					
					bool isHouseTile = false;
					House* house = NULL;

					if(type == OTBM_TILE)
						tile = new Tile(px, py, pz);
					else if(type == OTBM_HOUSETILE)
					{
						uint32_t _houseid;
						if(!propStream.GET_ULONG(_houseid))
						{
							std::ostringstream ss;
							ss << "[x:" << px << ", y:" << py << ", z:" << pz << "] " << "Could not read house id.";
							setLastErrorString(ss.str());
							return false;
						}

						house = Houses::getInstance().getHouse(_houseid, true);
						if(!house)
						{
							std::ostringstream ss;
							ss << "[x:" << px << ", y:" << py << ", z:" << pz << "] " << "Could not create house id: " << _houseid;
							setLastErrorString(ss.str());
							return false;
						}

						tile = new HouseTile(px, py, pz, house);
						house->addTile(static_cast<HouseTile*>(tile));
						isHouseTile = true;
					}

					map->setTile(px, py, pz, tile);

					//read tile attributes
					unsigned char attribute;
					while(propStream.GET_UCHAR(attribute))
					{
						switch(attribute)
						{
							case OTBM_ATTR_TILE_FLAGS:
							{
								uint32_t flags;
								if(!propStream.GET_ULONG(flags))
								{
									std::ostringstream ss;
									ss << "[x:" << px << ", y:" << py << ", z:" << pz << "] " << "Failed to read tile flags.";
									setLastErrorString(ss.str());
									return false;
								}

								if((flags & TILESTATE_PROTECTIONZONE) == TILESTATE_PROTECTIONZONE)
									tile->setFlag(TILESTATE_PROTECTIONZONE);
								else if((flags & TILESTATE_NOPVPZONE) == TILESTATE_NOPVPZONE)
									tile->setFlag(TILESTATE_NOPVPZONE);
								else if((flags & TILESTATE_PVPZONE) == TILESTATE_PVPZONE)
									tile->setFlag(TILESTATE_PVPZONE);

								if((flags & TILESTATE_NOLOGOUT) == TILESTATE_NOLOGOUT)
									tile->setFlag(TILESTATE_NOLOGOUT);

								break;
							}

							case OTBM_ATTR_ITEM:
							{
								Item* item = Item::CreateItem(propStream);
								if(!item)
								{
									std::ostringstream ss;
									ss << "[x:" << px << ", y:" << py << ", z:" << pz << "] " << "Failed to create item.";
									setLastErrorString(ss.str());
									return false;
								}

								if(isHouseTile && !item->isNotMoveable())
								{
									std::clog << "Warning: [OTBM loader] Moveable item in house id = " << house->getHouseId() << " Item type = " << item->getID() << std::endl;
									delete item;
									item = NULL;
								}
								else
								{
									tile->__internalAddThing(item);
									item->__startDecaying();
									item->setLoadedFromMap(true);
								}
								break;
							}

							default:
								std::ostringstream ss;
								ss << "[x:" << px << ", y:" << py << ", z:" << pz << "] " << "Unknown tile attribute.";
								setLastErrorString(ss.str());
								return false;
						}
					}

					NODE nodeItem = f.getChildNode(nodeTile, type);
					while(nodeItem)
					{
						if(type == OTBM_ITEM)
						{
							PropStream propStream;
							f.getProps(nodeItem, propStream);
							
							Item* item = Item::CreateItem(propStream);
							if(!item)
							{
								std::ostringstream ss;
								ss << "[x:" << px << ", y:" << py << ", z:" << pz << "] " << "Failed to create item.";
								setLastErrorString(ss.str());
								return false;
							}

							if(item->unserializeItemNode(f, nodeItem, propStream))
							{
								if(isHouseTile && !item->isNotMoveable())
								{
									std::clog << "Warning: [OTBM loader] Moveable item in house id = " << house->getHouseId() << " Item type = " << item->getID() << std::endl;
									delete item;
								}
								else
								{
									tile->__internalAddThing(item);
									item->__startDecaying();
									item->setLoadedFromMap(true);

									if(isHouseTile)
									{
										Door* door = item->getDoor();
										if(door && door->getDoorId() != 0)
											house->addDoor(door);

										if(!door)
										{
											BedItem* bed = item->getBed();
											if(bed)
												house->addBed(bed);
										}
									}
								}
							}
							else
							{
								std::ostringstream ss;
								ss << "[x:" << px << ", y:" << py << ", z:" << pz << "] " << "Failed to load item " << item->getID() << ".";
								setLastErrorString(ss.str());
								delete item;
								return false;
							}
						}
						else
						{
							std::ostringstream ss;
							ss << "[x:" << px << ", y:" << py << ", z:" << pz << "] " << "Unknown node type.";
							setLastErrorString(ss.str());
						}

						nodeItem = f.getNextNode(nodeItem, type);
					}
				}
				else
				{
					setLastErrorString("Unknown tile node.");
					return false;
				}

				nodeTile = f.getNextNode(nodeTile, type);
			}
		}
		else if(type == OTBM_TOWNS)
		{
			NODE nodeTown = f.getChildNode(nodeMapData, type);
			while(nodeTown != NO_NODE)
			{
				if(type == OTBM_TOWN)
				{
					if(!f.getProps(nodeTown, propStream))
					{
						setLastErrorString("Could not read town data.");
						return false;
					}
					
					uint32_t townid = 0;
					if(!propStream.GET_ULONG(townid))
					{
						setLastErrorString("Could not read town id.");
						return false;
					}

					Town* town = Towns::getInstance().getTown(townid);
					if(!town)
					{
						town = new Town(townid);
						Towns::getInstance().addTown(townid, town);
					}

					std::string townName = "";
					if(!propStream.GET_STRING(townName))
					{
						setLastErrorString("Could not read town name.");
						return false;
					}

					town->setName(townName);

					OTBM_TownTemple_coords *town_coords;
					if(!propStream.GET_STRUCT(town_coords))
					{
						setLastErrorString("Could not read town coordinates.");
						return false;
					}

					Position pos;
					pos.x = town_coords->_x;
					pos.y = town_coords->_y;
					pos.z = town_coords->_z;
					town->setTemplePos(pos);
				}
				else
				{
					setLastErrorString("Unknown town node.");
					return false;
				}

				nodeTown = f.getNextNode(nodeTown, type);
			}
		}
		else if(type != 15 && type != 16)
		{
			setLastErrorString("Unknown map node.");
			return false;
		}

		nodeMapData = f.getNextNode(nodeMapData, type);
	}
	
	std::clog << "> Map loading time: " << (OTSYS_TIME() - start) / (1000.) << " seconds." << std::endl;
	return true;
}
