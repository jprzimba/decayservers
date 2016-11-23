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
#include "vocation.h"
#include <iostream>
#include <cmath>

#include "tools.h"

Vocations::Vocations()
{
	//
}

Vocations::~Vocations()
{
	for(VocationsMap::iterator it = vocationsMap.begin(); it != vocationsMap.end(); ++it)
		delete it->second;
	vocationsMap.clear();
}

bool Vocations::loadFromXml()
{
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file("data/XML/vocations.xml");
	if(!result) {
		std::cout << "[Error - Vocations::loadFromXml] Failed to load data/XML/vocations.xml: " << result.description() << std::endl;
		return false;
	}

	for (pugi::xml_node vocationNode = doc.child("vocations").first_child(); vocationNode; vocationNode = vocationNode.next_sibling()) {
		pugi::xml_attribute attr;
		if(!(attr = vocationNode.attribute("id"))) {
			std::cout << "[Warning - Vocations::loadFromXml] Missing vocation id" << std::endl;
			continue;
		}

		uint16_t id = pugi::cast<uint16_t>(attr.value());
		Vocation* voc = new Vocation();
		vocationsMap[id] = voc;

		if((attr = vocationNode.attribute("name"))) {
			voc->name = attr.as_string();
		}

		if((attr = vocationNode.attribute("description"))) {
			voc->description = attr.as_string();
		}

		if((attr = vocationNode.attribute("gaincap"))) {
			voc->gainCap = pugi::cast<uint32_t>(attr.value());
		}

		if((attr = vocationNode.attribute("gainhp"))) {
			voc->gainHP = pugi::cast<uint32_t>(attr.value());
		}

		if((attr = vocationNode.attribute("gainmana"))) {
			voc->gainMana = pugi::cast<uint32_t>(attr.value());
		}

		if((attr = vocationNode.attribute("gainhpticks"))) {
			voc->gainHealthTicks = pugi::cast<uint32_t>(attr.value());
		}

		if((attr = vocationNode.attribute("gainhpamount"))) {
			voc->gainHealthAmount = pugi::cast<uint32_t>(attr.value());
		}

		if((attr = vocationNode.attribute("gainmanaticks"))) {
			voc->gainManaTicks = pugi::cast<uint32_t>(attr.value());
		}

		if((attr = vocationNode.attribute("gainmanaamount"))) {
			voc->gainManaAmount = pugi::cast<uint32_t>(attr.value());
		}

		if((attr = vocationNode.attribute("manamultiplier"))) {
			voc->manaMultiplier = pugi::cast<float>(attr.value());
		}

		if((attr = vocationNode.attribute("attackspeed"))) {
			voc->attackSpeed = pugi::cast<uint32_t>(attr.value());
		}

		if((attr = vocationNode.attribute("soulmax"))) {
			voc->soulMax = pugi::cast<uint16_t>(attr.value());
		}

		if((attr = vocationNode.attribute("gainsoulticks"))) {
			voc->gainSoulTicks = pugi::cast<uint16_t>(attr.value());
		}

		if((attr = vocationNode.attribute("fromvoc"))) {
			voc->fromVocation = pugi::cast<uint32_t>(attr.value());
		}

		for (pugi::xml_node childNode = vocationNode.first_child(); childNode; childNode = childNode.next_sibling()) {
			if(strcasecmp(childNode.name(), "skill") == 0) {
				pugi::xml_attribute skillIdAttribute = childNode.attribute("id");
				if(skillIdAttribute) {
					uint16_t skill_id = pugi::cast<uint16_t>(skillIdAttribute.value());
					if(skill_id <= SKILL_LAST) {
						voc->skillMultipliers[skill_id] = pugi::cast<float>(childNode.attribute("multiplier").value());
					} else {
						std::cout << "[Notice - Vocations::loadFromXml] No valid skill id: " << skill_id << " for vocation: " << id << std::endl;
					}
				} else {
					std::cout << "[Notice - Vocations::loadFromXml] Missing skill id for vocation: " << id << std::endl;
				}
			} else if(strcasecmp(childNode.name(), "formula") == 0) {
				pugi::xml_attribute meleeDamageAttribute = childNode.attribute("meleeDamage");
				if(meleeDamageAttribute) {
					voc->meleeDamageMultipler = pugi::cast<float>(meleeDamageAttribute.value());
				}

				pugi::xml_attribute distDamageAttribute = childNode.attribute("distDamage");
				if(distDamageAttribute) {
					voc->distDamageMultipler = pugi::cast<float>(distDamageAttribute.value());
				}

				pugi::xml_attribute defenseAttribute = childNode.attribute("defense");
				if(defenseAttribute) {
					voc->defenseMultipler = pugi::cast<float>(defenseAttribute.value());
				}

				pugi::xml_attribute armorAttribute = childNode.attribute("armor");
				if(armorAttribute) {
					voc->armorMultipler = pugi::cast<float>(armorAttribute.value());
				}
			}
		}
	}
	return true;
}

Vocation* Vocations::getVocation(uint32_t vocId)
{
	VocationsMap::iterator it = vocationsMap.find(vocId);
	if(it != vocationsMap.end())
		return it->second;
	else
	{
		std::cout << "Warning: [Vocations::getVocation] Vocation " << vocId << " not found." << std::endl;
		return &def_voc;
	}
}

int32_t Vocations::getVocationId(const std::string& name)
{
	for(VocationsMap::iterator it = vocationsMap.begin(); it != vocationsMap.end(); ++it)
	{
		if(strcasecmp(it->second->name.c_str(), name.c_str()) == 0)
			return it->first;
	}
	return -1;
}

int32_t Vocations::getPromotedVocation(uint32_t vocationId)
{
	for(VocationsMap::iterator it = vocationsMap.begin(); it != vocationsMap.end(); ++it)
	{
		if(it->second->fromVocation == vocationId && it->first != vocationId)
			return it->first;
	}
	return 0;
}

uint32_t Vocation::skillBase[SKILL_LAST + 1] = { 50, 50, 50, 50, 30, 100, 20 };

Vocation::Vocation()
{
	name = "none";
	description = "";
	gainHealthTicks = 6;
	gainHealthAmount = 1;
	gainManaTicks = 6;
	gainManaAmount = 1;
	gainSoulTicks = 120;
	soulMax = 100;
	
	fromVocation = 0;
	
	gainCap = 5;
	gainMana = 5;
	gainHP = 5;
	attackSpeed = 1500;
	manaMultiplier = 4.0;
	meleeDamageMultipler = 1.0;
	distDamageMultipler = 1.0;
	defenseMultipler = 1.0;
	armorMultipler = 1.0;
	skillMultipliers[0] = 1.5f;
	skillMultipliers[1] = 2.0f;
	skillMultipliers[2] = 2.0f;
	skillMultipliers[3] = 2.0f;
	skillMultipliers[4] = 2.0f;
	skillMultipliers[5] = 1.5f;
	skillMultipliers[6] = 1.1f;
}

Vocation::~Vocation()
{
	cacheMana.clear();
	for(int32_t i = SKILL_FIRST; i < SKILL_LAST; ++i){
		cacheSkill[i].clear();
	}
}

uint32_t Vocation::getReqSkillTries(int32_t skill, int32_t level)
{
	if(skill < SKILL_FIRST || skill > SKILL_LAST){
		return 0;
	}
	cacheMap& skillMap = cacheSkill[skill];
	cacheMap::iterator it = skillMap.find(level);
	if(it != cacheSkill[skill].end()){
		return it->second;
	}
	uint32_t tries = (unsigned int)(skillBase[skill] * pow((float)skillMultipliers[skill], (float)(level - 11)));
	skillMap[level] = tries;
	return tries;
}

uint64_t Vocation::getReqMana(uint32_t magLevel)
{
	cacheMap::iterator it = cacheMana.find(magLevel);
	if(it != cacheMana.end())
		return it->second;

	uint64_t reqMana = (uint64_t)(400 * pow(manaMultiplier, magLevel-1));
	if(reqMana % 20 < 10)
		reqMana = reqMana - (reqMana % 20);
	else
		reqMana = reqMana - (reqMana % 20) + 20;

	cacheMana[magLevel] = reqMana;
	return reqMana;
}
