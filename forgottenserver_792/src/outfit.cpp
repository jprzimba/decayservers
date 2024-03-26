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
#include "outfit.h"
#include "creature.h"
#include "player.h"
#include "tools.h"

OutfitList::OutfitList()
{
	//
}

OutfitList::~OutfitList()
{
	OutfitListType::iterator it;
	for(it = m_list.begin(); it != m_list.end(); it++)
		delete *it;
	m_list.clear();
}

void OutfitList::addOutfit(const Outfit& outfit)
{
	OutfitListType::iterator it;
	for(it = m_list.begin(); it != m_list.end(); ++it)
	{
		if((*it)->looktype == outfit.looktype)
		{
			(*it)->addons = (*it)->addons | outfit.addons;
			return;
		}
	}
	
	//adding a new outfit
	Outfit* new_outfit = new Outfit;
	new_outfit->looktype = outfit.looktype;
	new_outfit->addons = outfit.addons;
	new_outfit->premium = outfit.premium;
	m_list.push_back(new_outfit);
}

bool OutfitList::remOutfit(const Outfit& outfit)
{
	OutfitListType::iterator it;
	for(it = m_list.begin(); it != m_list.end(); ++it)
	{
		if((*it)->looktype == outfit.looktype)
		{
			if(outfit.addons == 0xFF)
			{
				delete *it;
				m_list.erase(it);
			}
			else
				(*it)->addons = (*it)->addons & (~outfit.addons);

			return true;
		}
	}
	return false;
}

bool OutfitList::isInList(uint32_t looktype, uint32_t addons, bool playerPremium, int32_t playerSex) const
{
	OutfitListType::const_iterator it, it_;
	const OutfitListType& global_outfits = Outfits::getInstance()->getOutfits(playerSex);
	for(it = global_outfits.begin(); it != global_outfits.end(); ++it)
	{
		if((*it)->looktype == looktype)
		{
			for(it_ = m_list.begin(); it_ != m_list.end(); ++it_)
			{
				if((*it_)->looktype == looktype)
				{
					if(((*it_)->addons & addons) == addons)
					{
						if((*it)->premium && playerPremium || !(*it)->premium)
							return true;
					}
					return false;
				}
			}
			return false;
		}
	}
	return false;
}

Outfits::Outfits()
{
	Outfit outfit;
	//build default outfit lists
	outfit.addons = 0;
	outfit.premium = false;
	for(int32_t i = PLAYER_FEMALE_1; i <= PLAYER_FEMALE_7; i++){
		outfit.looktype = i;
		m_female_list.addOutfit(outfit);
	}

	for(int32_t i = PLAYER_MALE_1; i <= PLAYER_MALE_7; i++){
		outfit.looktype = i;
		m_male_list.addOutfit(outfit);
	}

	m_list.resize(10, NULL);
}

Outfits::~Outfits()
{
	OutfitsListVector::iterator it;
	for(it = m_list.begin(); it != m_list.end(); it++){
		delete *it;
	}
	m_list.clear();
}

bool Outfits::loadFromXml()
{
	std::string filename = "data/XML/outfits.xml";
	xmlDocPtr doc = xmlParseFile(filename.c_str());
	if(doc)
	{
		xmlNodePtr root, p;
		root = xmlDocGetRootElement(doc);
		
		if(xmlStrcmp(root->name,(const xmlChar*)"outfits") != 0)
		{
			xmlFreeDoc(doc);
			std::clog << "Warning: outfits.xml not found, using defaults." << std::endl;
			return true;
		}
		
		p = root->children;
		
		while(p)
		{
			std::string str;
			int32_t intVal;
			if(xmlStrcmp(p->name, (const xmlChar*)"outfit") == 0)
			{
				if(readXMLInteger(p, "type", intVal))
				{
					if(intVal > 9 || intVal < 0)
						std::clog << "Warning: No valid outfit type " << intVal << std::endl;
					else
					{
						OutfitList* list;
						if(m_list[intVal] != NULL)
							list = m_list[intVal];
						else
						{
							list = new OutfitList;
							m_list[intVal] = list;
						}
						
						Outfit outfit;
						std::string outfitName;
						bool outfitEnabled = true;

						readXMLString(p, "name", outfitName);
						if(readXMLInteger(p, "looktype", intVal))
							outfit.looktype = intVal;
						else
							std::clog << "[Warning] Missing looktype on outfit: " << outfitName << std::endl;

						if(readXMLInteger(p, "addons", intVal))
							outfit.addons = intVal;
						else
							outfit.addons = 0;

						if(readXMLInteger(p, "premium", intVal))
							outfit.premium = (intVal == 1);

						if(readXMLInteger(p, "enabled", intVal))
							outfitEnabled = (intVal == 1);

						outfitNamesMap[outfit.looktype] = outfitName;
						if(outfitEnabled)
						{
							//This way you can add names for outfits without adding them to default list
							list->addOutfit(outfit);
						}
					}
				}
				else
					std::clog << "Missing outfit type." << std::endl;
			}
			p = p->next;
		}
		xmlFreeDoc(doc);
	}
	return true;
}
