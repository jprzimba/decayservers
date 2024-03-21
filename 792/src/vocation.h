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

#ifndef __OTSERV_VOCATION_H__
#define __OTSERV_VOCATION_H__

#include "enums.h"
#include <string>
#include <algorithm>
#include <map>
#include "item.h"

class Vocation
{
	public:
		~Vocation();
		const std::string& getVocName() const {return name;}
		const std::string& getVocDescription() const {return description;}
		uint32_t getReqSkillTries(int32_t skill, int32_t level);
		uint64_t getReqMana(uint32_t magLevel);

		uint32_t getHPGain() const {return gainHP;}
		uint32_t getManaGain() const {return gainMana;}
		uint32_t getCapGain() const {return gainCap;}

		uint32_t getManaGainTicks() const {return gainManaTicks;}
		uint32_t getManaGainAmount() const {return gainManaAmount;}
		uint32_t getHealthGainTicks() const {return gainHealthTicks;}
		uint32_t getHealthGainAmount() const {return gainHealthAmount;}

		uint16_t getSoulMax() const {return std::min<uint32_t>((uint32_t)soulMax, (uint32_t)255);}
		uint16_t getSoulGainTicks() const {return gainSoulTicks;}

		uint32_t getAttackSpeed() const {return attackSpeed;}

		uint32_t getFromVocation() const {return fromVocation;}

		float meleeDamageMultipler, distDamageMultipler, defenseMultipler, armorMultipler;

	protected:
		friend class Vocations;
		Vocation();

		std::string name;
		std::string description;

		uint32_t gainHealthTicks;
		uint32_t gainHealthAmount;
		uint32_t gainManaTicks;
		uint32_t gainManaAmount;
		uint32_t gainCap;
		uint32_t gainMana;
		uint32_t gainHP;

		uint16_t gainSoulTicks;
		uint16_t soulMax;

		uint32_t fromVocation;

		uint32_t attackSpeed;

		static uint32_t skillBase[SKILL_LAST + 1];
		float skillMultipliers[SKILL_LAST + 1];
		float manaMultiplier;

		typedef std::map<uint32_t, uint32_t> cacheMap;
		cacheMap cacheMana;
		cacheMap cacheSkill[SKILL_LAST + 1];
};

typedef std::map<uint32_t, Vocation*> VocationsMap;

class Vocations
{
	public:
		Vocations();
		~Vocations();

		static Vocations* getInstance()
		{
			static Vocations instance;
			return &instance;
		}

		bool loadFromXml();
		Vocation* getVocation(uint32_t vocId);
		int32_t getVocationId(const std::string& name);
		int32_t getPromotedVocation(uint32_t vocationId);

		VocationsMap::iterator getFirstVocation() {return vocationsMap.begin();}
		VocationsMap::iterator getLastVocation() {return vocationsMap.end();}

	private:
		VocationsMap vocationsMap;
		Vocation def_voc;
};

#endif
