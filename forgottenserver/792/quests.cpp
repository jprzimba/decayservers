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
	mainState = nullptr;
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
	if(mainState != nullptr)
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
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file("data/XML/quests.xml");
	if(!result) {
		std::cout << "[Error - Quests::loadFromXml] Failed to load data/XML/quests.xml: " << result.description() << std::endl;
		return false;
	}

	uint16_t id = 0;
	for (pugi::xml_node questNode = doc.child("quests").first_child(); questNode; questNode = questNode.next_sibling()) {
			std::string name;
			int32_t startStorageID = 0, startStorageValue = 0;
			name = questNode.attribute("name").as_string();
			startStorageID = pugi::cast<int32_t>(questNode.attribute("startstorageid").value());
			startStorageValue = pugi::cast<int32_t>(questNode.attribute("startstoragevalue").value());
			
			Quest *quest = new Quest(name, id, startStorageID, startStorageValue);

		for (pugi::xml_node missionNode = questNode.first_child(); missionNode; missionNode = missionNode.next_sibling()) {
			std::string missionName, missionState;
			int32_t storageID = 0, startValue = 0, endValue = 0;
			missionName = missionNode.attribute("name").as_string();
			storageID = pugi::cast<int32_t>(missionNode.attribute("storageid").value());
			startValue = pugi::cast<int32_t>(missionNode.attribute("startvalue").value());
			endValue = pugi::cast<int32_t>(missionNode.attribute("endvalue").value());
			missionState = missionNode.attribute("description").as_string();
			
			Mission* mission = new Mission(missionName, storageID, startValue, endValue);

			if(missionState.empty()) {
				for (pugi::xml_node missionStateNode = missionNode.first_child(); missionStateNode; missionStateNode = missionStateNode.next_sibling()) {
					int32_t missionID = pugi::cast<int32_t>(missionStateNode.attribute("id").value());
					mission->state[missionID] = new MissionState(missionStateNode.attribute("description").as_string(), missionID);
				}
			} else {
				mission->mainState = new MissionState(missionState, 0);
			}
			
			quest->addMission(mission);
		}
		id++;
		quests.push_back(quest);
	}
	return true;
}

Quest *Quests::getQuestByID(uint16_t id)
{
	for(QuestsList::iterator it = quests.begin(), end = quests.end(); it != end; ++it)
	{
		if((*it)->getID() == id)
			return (*it);
	}
	return nullptr;
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
			if((*m_it)->mainState != nullptr)
				continue;

			if((*m_it)->getStorageId() == key && value >= (*m_it)->getStartStorageValue() && value <= (*m_it)->getEndStorageValue())
				return true;
		}
	}
	return false;
}
