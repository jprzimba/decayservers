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

#include "group.h"
#include "tools.h"

Group Groups::defGroup = Group();

void Groups::clear()
{
	for(GroupsMap::iterator it = groupsMap.begin(); it != groupsMap.end(); ++it)
		delete it->second;

	groupsMap.clear();
}

bool Groups::reload()
{
	clear();
	return loadFromXml();
}

bool Groups::loadFromXml()
{
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file("data/XML/groups.xml");
	if(!result) {
		std::cout << "[Error - Groups::loadFromXml] Failed to load data/XML/groups.xml: " << result.description() << std::endl;
		return false;
	}

	for(pugi::xml_node groupNode = doc.child("groups").first_child(); groupNode; groupNode = groupNode.next_sibling()) {
		pugi::xml_attribute attr;
		if(attr = groupNode.attribute("id"))
		{
			Group* group = new Group(pugi::cast<uint32_t>(attr.value()));
			
			if(attr = groupNode.attribute("name"))
			{
				group->setFullName(attr.as_string());
				group->setName(asLowerCaseString(attr.as_string()));
			}

			if(attr = groupNode.attribute("flags"))
				group->setFlags(pugi::cast<int64_t>(attr.value()));
			
			if(attr = groupNode.attribute("access"))
				group->setAccess(pugi::cast<int32_t>(attr.value()));

			if(attr = groupNode.attribute("depotLimit"))
				group->setDepotLimit(pugi::cast<int32_t>(attr.value()));
			
			if(attr = groupNode.attribute("maxVips"))
				group->setMaxVips(pugi::cast<int32_t>(attr.value()));

			groupsMap[group->getId()] = group;
		}
		else
		{
			std::cout << "[Warning - Groups::loadFromXml] Missing group id." << std::endl;
			return false;
		}
	}
	return true;
}

Group* Groups::getGroup(uint32_t groupId)
{
	GroupsMap::iterator it = groupsMap.find(groupId);
	if(it != groupsMap.end())
		return it->second;

	std::cout << "[Warning - Groups::getGroup] Group " << groupId << " not found." << std::endl;
	return &defGroup;
}
