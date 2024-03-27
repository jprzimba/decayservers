//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Quests
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

#include <sstream>

#include "quests.h"
#include "tools.h"

MissionState::MissionState(std::string _description, int32_t _missionID)
{
	description = _description;
 	missionID = _missionID;
}

Mission::Mission(std::string _missionName, int32_t _storageID, int32_t _startValue, int32_t _endValue)
{
	missionName = _missionName;
	endValue = _endValue;
	startValue = _startValue;
	storageID = _storageID;
	mainState = NULL;
}

Mission::~Mission()
{
	for(uint32_t it = 0; it != state.size(); it++)
		delete state[it];
 
	state.clear();
}
 
std::string Mission::getDescription(Player* player)
{
	int32_t value;
	player->getStorageValue(storageID, value);
	if(mainState != NULL)
	{
		std::ostringstream s;
		s << value;
 
		std::string desc = mainState->getMissionDescription();
		replaceString(desc, "|STATE|", s.str());
		replaceString(desc, "\\n", "\n");
		return desc;
	}

	player->getStorageValue(storageID, value);
 
	int32_t current = endValue;
	while(current >= startValue)
	{
		if(value == current)
		{
			StateList::const_iterator sit = state.find(current);
			if(sit != state.end())
				return sit->second->getMissionDescription();
		}
		current--;
	}
	return "An error has occurred, please contact a gamemaster.";
}

bool Mission::isStarted(Player* player) const
{
	if(!player)
		return false;

	int32_t value;
	if(!player->getStorageValue(storageID, value) || value < startValue || value > endValue)
		return false;

	return true;
}
 
bool Mission::isCompleted(Player* player) const
{
	if(!player)
		return false;

	int32_t value;
	if(!player->getStorageValue(storageID, value) || value != endValue)
		return false;
 
	return true;
}

std::string Mission::getName(Player* player)
{
	if(isCompleted(player))
		return missionName + " (completed)";
 
	return missionName;
}

Quest::Quest(std::string _name, uint16_t _id, int32_t _startStorageID, int32_t _startStorageValue)
{
	name = _name;
	id = _id;
	startStorageID = _startStorageID;
	startStorageValue = _startStorageValue;
}

Quest::~Quest()
{
	for(MissionsList::iterator it = missions.begin(), end = missions.end(); it != end; ++it)
		delete (*it);
 
	missions.clear();
}
 
uint16_t Quest::getMissionsCount(Player* player) const
{
	uint16_t count = 0;
	for(MissionsList::const_iterator it = missions.begin(), end = missions.end(); it != end; ++it)
	{
		if((*it)->isStarted(player))
			count++;
	}
	return count;
}
 
bool Quest::isCompleted(Player* player)
{
	for(MissionsList::const_iterator it = missions.begin(), end = missions.end(); it != end; ++it)
	{
		if(!(*it)->isCompleted(player))
			return false;
	}
	return true;
}

bool Quest::isStarted(Player* player) const
{
	if(!player)
		return false;

	int32_t value;
	if(!player->getStorageValue(startStorageID, value) || value < startStorageValue)
		return false;
 
	return true;
}
 
Quests::Quests()
{
	//
}

Quests::~Quests()
{
	for(QuestsList::iterator it = quests.begin(), end = quests.end(); it != end; ++it)
		delete (*it);

	quests.clear();
}
 
bool Quests::reload()
{
	for(QuestsList::iterator it = quests.begin(), end = quests.end(); it != end; ++it)
		delete (*it);

	quests.clear();
	return loadFromXml();
}

bool Quests::loadFromXml()
{
	std::string filename = "data/XML/quests.xml";
	xmlDocPtr doc = xmlParseFile(filename.c_str());
	if(doc)
	{
		xmlNodePtr root, p;
		root = xmlDocGetRootElement(doc);
		if(xmlStrcmp(root->name,(const xmlChar*)"quests") == 0)
		{
			int32_t intValue;
			std::string strValue;
			uint16_t id = 0;
			p = root->children;
			while(p)
			{
				if(xmlStrcmp(p->name, (const xmlChar*)"quest") == 0)
				{
					std::string name;
					uint32_t startStorageID = 0, startStorageValue = 0;
					if(readXMLString(p, "name", strValue))
						name = strValue;
						
					if(readXMLInteger(p, "startstorageid", intValue))
						startStorageID = intValue;

					if(readXMLInteger(p, "startstoragevalue", intValue))
						startStorageValue = intValue;
					
					Quest *quest = new Quest(name, id, startStorageID, startStorageValue);
					xmlNodePtr tmpNode = p->children;
					while(tmpNode)
					{
						if(xmlStrcmp(tmpNode->name, (const xmlChar*)"mission") == 0)
						{
							std::string missionName;
							uint32_t storageID = 0, startValue = 0, endValue = 0;
							if(readXMLString(tmpNode, "name", strValue))
								missionName = strValue;

							if(readXMLInteger(tmpNode, "storageid", intValue))
								storageID = intValue;

							if(readXMLInteger(tmpNode, "startvalue", intValue))
								startValue = intValue;
							
							if(readXMLInteger(tmpNode, "endvalue", intValue))
								endValue = intValue;
							
							xmlNodePtr tmpNode2 = tmpNode->children;

							Mission *mission = new Mission(missionName, storageID, startValue, endValue);
							while(tmpNode2)
							{
								if(xmlStrcmp(tmpNode2->name, (const xmlChar*)"missionstate") == 0)
								{
									std::string description;
									uint32_t missionID;
									if(readXMLInteger(tmpNode2, "id", intValue))
										missionID = intValue;
									if(readXMLString(tmpNode2, "description", strValue))
										description = strValue;
									mission->state[missionID] = new MissionState(description, missionID);
								}
								tmpNode2 = tmpNode2->next;
							}
							quest->missions.push_back(mission);
						}
						tmpNode = tmpNode->next;
					}
					quests.push_back(quest);
				}
				id++;
				p = p->next;
			}
		}
		xmlFreeDoc(doc);
		return true;
	}
	return false;
}

Quest *Quests::getQuestByID(uint16_t id)
{
	for(QuestsList::iterator it = quests.begin(), end = quests.end(); it != end; ++it)
	{
		if((*it)->getID() == id)
			return (*it);
	}
	return NULL;
}

uint16_t Quests::getQuestsCount(Player* player)
{
	uint16_t count = 0;
	for(QuestsList::const_iterator it = quests.begin(), end = quests.end(); it != end; ++it)
	{
		if((*it)->isStarted(player))
			count++;
	}
	return count;
}

bool Quests::isQuestStorage(const uint32_t key, const int32_t value)
{
	for(QuestsList::const_iterator it = quests.begin(), end = quests.end(); it != end; ++it)
	{
		if((*it)->getStartStorageId() == key && (*it)->getStartStorageValue() == value)
			return true;

		for(MissionsList::const_iterator m_it = (*it)->getFirstMission(), m_end = (*it)->getLastMission(); m_it != m_end; ++m_it)
		{
			if((*m_it)->mainState != NULL)
				continue;

			if((*m_it)->getStorageId() == key && value >= (*m_it)->getStartStorageValue() && value <= (*m_it)->getEndStorageValue())
				return true;
		}
	}
	return false;
}
