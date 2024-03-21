//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Various functions.
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
#include "otsystem.h"
#include "boost/asio.hpp"

#include "tools.h"
#include "configmanager.h"
#include "md5.h"
#include "sha1.h"
#include <sstream>
#include <iomanip>

extern ConfigManager g_config;

std::string transformToSHA1(std::string plainText, bool upperCase /*= false*/)
{
	SHA1 sha1;
	unsigned sha1Hash[5];
	std::stringstream hexStream;

	sha1.Input((const unsigned char*)plainText.c_str(), plainText.length());
	sha1.Result(sha1Hash);

	hexStream.flags(std::ios::hex | std::ios::uppercase);
	for(uint32_t i = 0; i < 5; ++i)
		hexStream << std::setw(8) << std::setfill('0') << (uint32_t)sha1Hash[i];

	std::string hexStr = hexStream.str();
	if(!upperCase)
		toLowerCaseString(hexStr);

	return hexStr;
}

std::string transformToMD5(std::string plainText, bool upperCase /*= false*/)
{
	MD5_CTX m_md5;
	std::stringstream hexStream;

	MD5Init(&m_md5, 0);
	MD5Update(&m_md5, (const unsigned char*)plainText.c_str(), plainText.length());
	MD5Final(&m_md5);

	hexStream.flags(std::ios::hex | std::ios::uppercase);
	for(uint32_t i = 0; i < 16; ++i)
		hexStream << std::setw(2) << std::setfill('0') << (uint32_t)m_md5.digest[i];

	std::string hexStr = hexStream.str();
	if(!upperCase)
		toLowerCaseString(hexStr);

	return hexStr;
}

bool passwordTest(const std::string &plain, std::string &hash)
{
	switch(g_config.getNumber(ConfigManager::PASSWORD_TYPE))
	{
		case PASSWORD_TYPE_MD5:
		{
			std::transform(hash.begin(), hash.end(), hash.begin(), upchar);
			return transformToMD5(plain, true) == hash;
			break;
		}

		case PASSWORD_TYPE_SHA1:
		{
			std::transform(hash.begin(), hash.end(), hash.begin(), upchar);
			return transformToSHA1(plain, true) == hash;
			break;
		}

		default:
		{
			return plain == hash;
			break;
		}
	}
	return false;
}

void replaceString(std::string& str, const std::string sought, const std::string replacement)
{
	size_t pos = 0;
	size_t start = 0;
	size_t soughtLen = sought.length();
	size_t replaceLen = replacement.length();
	while((pos = str.find(sought, start)) != std::string::npos)
	{
		str = str.substr(0, pos) + replacement + str.substr(pos + soughtLen);
		start = pos + replaceLen;
	}
}

void trim_right(std::string& source, const std::string& t)
{
	source.erase(source.find_last_not_of(t)+1);
}

void trim_left(std::string& source, const std::string& t)
{
	source.erase(0, source.find_first_not_of(t));
}

void toLowerCaseString(std::string& source)
{
	std::transform(source.begin(), source.end(), source.begin(), tolower);
}

void toUpperCaseString(std::string& source)
{
	std::transform(source.begin(), source.end(), source.begin(), upchar);
}

std::string asLowerCaseString(const std::string& source)
{
	std::string s = source;
	toLowerCaseString(s);
	return s;
}

std::string asUpperCaseString(const std::string& source)
{
	std::string s = source;
	toUpperCaseString(s);
	return s;
}

std::vector<std::string> explodeString(const std::string& inString, const std::string& separator)
{
	std::vector<std::string> returnVector;
	std::string::size_type start = 0;
	std::string::size_type end = 0;

	while((end = inString.find(separator, start)) != std::string::npos)
	{
		returnVector.push_back(inString.substr(start, end - start));
		start = end + separator.size();
	}

	returnVector.push_back(inString.substr(start));
	return returnVector;
}

std::vector<int32_t> vectorAtoi(std::vector<std::string> stringVector)
{
	std::vector<int32_t> returnVector;
	for(std::vector<std::string>::iterator it = stringVector.begin(); it != stringVector.end(); ++it)
		returnVector.push_back(atoi((*it).c_str()));

	return returnVector;
}

bool hasBitSet(uint32_t flag, uint32_t flags)
{
	return ((flags & flag) == flag);
}

#define RAND_MAX24 16777216
uint32_t rand24b()
{
	return (rand() << 12) ^ (rand()) & (0xFFFFFF);
}

float box_muller(float m, float s)
{
	// normal random variate generator 
	// mean m, standard deviation s 

	float x1, x2, w, y1;
	static float y2;
	static int use_last = 0;

	if(use_last) // use value from previous call
	{
		y1 = y2;
		use_last = 0;
	}
	else
	{
		do
		{
			double r1 = (((float)(rand()) / RAND_MAX));
			double r2 = (((float)(rand()) / RAND_MAX));

			x1 = 2.0 * r1 - 1.0;
			x2 = 2.0 * r2 - 1.0;
			w = x1 * x1 + x2 * x2;
		}
		while(w >= 1.0);

		w = sqrt((-2.0 * log(w)) / w);
		y1 = x1 * w;
		y2 = x2 * w;
		use_last = 1;
	}
	return(m + y1 * s);
}

int random_range(int lowest_number, int highest_number, DistributionType_t type /*= DISTRO_UNIFORM*/)
{
	if(highest_number == lowest_number)
		return lowest_number;
	
	if(lowest_number > highest_number)
	{
		int32_t nTmp = highest_number;
		highest_number = lowest_number;
		lowest_number = nTmp;
	}
	
	int range = highest_number - lowest_number;
	if(type == DISTRO_UNIFORM)
	{
		int r = rand24b() % (range + 1);
		return lowest_number + r;
	}
	else if(type == DISTRO_NORMAL)
	{
		float value = box_muller(0.5, 0.25);

		if(value < 0)
			value = 0;
		else if(value > 1)
			value = 1;

		return lowest_number + (int)((float)range * value);
	}
	else
	{
		float r = 1.f -sqrt((1.f*rand24b())/RAND_MAX24);
		return lowest_number + (int32_t)((float)range * r);
	}
}

// Upcase a char.
char upchar(char c)
{
	if((c >= 97 && c <= 122) || (c <= -1 && c >= -32 ))
		c -= 32;
	return c;
}

bool isNumber(char character)
{
	return (character >= 48 && character <= 57);
}

bool isLowercaseLetter(char character)
{
	return (character >= 97 && character <= 122);
}

bool isUppercaseLetter(char character)
{
	return (character >= 65 && character <= 90);
}

bool isPasswordCharacter(char character)
{
	return (character >= 33 && character <= 47 || character >= 58 && character <= 64 || character >= 91 && character <= 96 || character >= 123 && character <= 126);
}

bool isValidPassword(std::string text)
{
	std::transform(text.begin(), text.end(), text.begin(), (int32_t(*)(int32_t))tolower);
	for(uint32_t size = 0; size <= text.length() - 1; size++)
	{
		if(isLowercaseLetter(text[size]) || isNumber(text[size]) || isPasswordCharacter(text[size]))
			continue;
		else
			return false;
	}
	return true;
}

bool isNumbers(std::string text)
{
	for(uint32_t size = 0; size <= text.length() - 1; size++)
	{
		if(!isNumber(text[size]))
			return false;
	}
	return true;
}

bool isValidName(std::string text, bool forceUppercaseOnFirstLetter/* = true*/)
{
	uint32_t lenBeforeSpace = 1;
	uint32_t lenBeforeSingleQuote = 1;
	uint32_t lenBeforeDash = 1;
	uint32_t repeatedCharacter = 0;
	char lastChar = 32;

	if(forceUppercaseOnFirstLetter)
	{
		if(!isUppercaseLetter(text[0]))
			return false;
	}
	else if(!isLowercaseLetter(text[0]) && !isUppercaseLetter(text[0]))
		return false;

	for(uint32_t i = 1, size = text.length(); i < size; ++i)
	{
		if(text[i] != 32)
		{
			lenBeforeSpace++;

			if(text[i] != 39)
				lenBeforeSingleQuote++;
			else
			{
				if(lenBeforeSingleQuote <= 1 || i == size - 1 || text[i + 1] == 32)
					return false;

				lenBeforeSingleQuote = 0;
			}

			if(text[i] != 45)
				lenBeforeDash++;
			else
			{
				if(lenBeforeDash <= 1 || i == size - 1 || text[i + 1] == 32)
					return false;

				lenBeforeDash = 0;
			}

			if(text[i] == lastChar)
			{
				repeatedCharacter++;
				if(repeatedCharacter > 2)
					return false;
			}
			else
				repeatedCharacter = 0;

			lastChar = text[i];
		}
		else
		{
			if(lenBeforeSpace <= 1 || i == size - 1 || text[i + 1] == 32)
				return false;

			lenBeforeSpace = 0;
			lenBeforeSingleQuote = 0;
			lenBeforeDash = 0;
		}

		if(isLowercaseLetter(text[i]) || text[i] == 32 || text[i] == 39 || text[i] == 45
			|| (isUppercaseLetter(text[i]) && text[i - 1] == 32))
		{
			continue;
		}
		else
			return false;
	}
	return true;
}

bool checkText(std::string text, std::string str)
{
	std::transform(text.begin(),text.end(), text.begin(), (int32_t(*)(int32_t))tolower);
	std::transform(str.begin(), str.end(), str.begin(), (int32_t(*)(int32_t))tolower);
	if(text == str)
		return true;

	for(uint32_t strlength = 0; strlength <= str.length(); strlength++)
	{
		if(text[strlength] == str[strlength] && text[str.length()] == ' ' && text[str.length()+1] == '"') 
			return true;
	}
	return false;
}

std::string generateRecoveryKey(int32_t fieldCount, int32_t fieldLenght)
{
	std::stringstream key;
	int32_t i(0);
	int32_t j(0);
	int32_t lastNumber = 99;
	int32_t number = 0;
	char character = 0;
	char lastCharacter = 0;
	bool doNumber = false;
	bool madeNumber = false;
	bool madeCharacter = false;
	do
	{
		do
		{
			madeNumber = false;
			madeCharacter = false;
			bool doNumber = random_range(0, 1) != 0;
			if(doNumber)
			{
				number = random_range(2, 9);
				if(number != lastNumber)
				{
					key << number;
					lastNumber = number;
					madeNumber = true;
				}
			}
			else
			{
				character = (char)random_range(65, 90);
				if(character != lastCharacter)
				{
					key << character;
					lastCharacter = character;
					madeCharacter = true;
				}
			}
		}
		while((!madeCharacter && !madeNumber) ? true : ++j && j < fieldLenght);
		if(i < fieldCount - 1)
			key << "-";
		character = 0;
		lastCharacter = 0;
		lastNumber = 99;
		number = 0;
		j = 0;
	}
	while(++i && i < fieldCount);
	return key.str();
}

std::string trimString(std::string& str)
{
	str.erase(str.find_last_not_of(" ") + 1);
	return str.erase(0, str.find_first_not_of(" "));
}

std::string parseParams(tokenizer::iterator &it, tokenizer::iterator end)
{
	std::string tmp;
	if(it == end)
		return "";
	else
	{
		tmp = *it;
		++it;
		if(tmp[0] == '"')
		{
			tmp.erase(0,1);
			while(it != end && tmp[tmp.length() - 1] != '"')
			{
				tmp += " " + *it;
				++it;
			}
			if(tmp.length() > 0 && tmp[tmp.length() - 1] == '"')
				tmp.erase(tmp.length() - 1);
		}
		return tmp;
	}
}

void formatDate(time_t time, char* buffer/* atleast 21 */)
{
	const tm* tms = localtime(&time);
	if(tms)
		sprintf(buffer, "%02d/%02d/%04d  %02d:%02d:%02d", tms->tm_mday, tms->tm_mon + 1, tms->tm_year + 1900, tms->tm_hour, tms->tm_min, tms->tm_sec);
	else
		sprintf(buffer, "UNIX Time : %d", (int)time);
}

void formatDate2(time_t time, char* buffer/* atleast 16 */)
{
	const tm* tms = localtime(&time);
	if(tms)
		strftime(buffer, 12, "%d %b %Y", tms);
	else
		sprintf(buffer, "UNIX Time : %d", (int)time);
}

Direction getDirection(std::string string)
{
	Direction direction = NORTH;

	if(string == "north" || string == "n" || string == "0")
		direction = NORTH;
	else if(string == "east" || string == "e" || string == "1")
		direction = EAST;
	else if(string == "south" || string == "s" || string == "2")
		direction = SOUTH;
	else if(string == "west" || string == "w" || string == "3")
		direction = WEST;
	else if(string == "southwest" || string == "south west" || string == "south-west" || string == "sw" || string == "4")
		direction = SOUTHWEST;
	else if(string == "southeast" || string == "south east" || string == "south-east" || string == "se" || string == "5")
		direction = SOUTHEAST;
	else if(string == "northwest" || string == "north west" || string == "north-west" || string == "nw" || string == "6")
		direction = NORTHWEST;
	else if(string == "northeast" || string == "north east" || string == "north-east" || string == "ne" || string == "7")
		direction = NORTHEAST;

	return direction;
}

Direction getReverseDirection(Direction dir)
{
	Direction _dir = NORTH;
	switch(dir)
	{
		case NORTH:
			_dir = SOUTH;
			break;
		case SOUTH:
			_dir = NORTH;
			break;
		case WEST:
			_dir = EAST;
			break;
		case EAST:
			_dir = WEST;
			break;
		case SOUTHWEST:
			_dir = NORTHEAST;
			break;
		case NORTHWEST:
			_dir = SOUTHEAST;
			break;
		case NORTHEAST:
			_dir = SOUTHWEST;
			break;
		case SOUTHEAST:
			_dir = NORTHWEST;
			break;
		default:
			break;
	}
	return _dir;
}

Position getNextPosition(Direction direction, Position pos)
{
	switch(direction)
	{
		case NORTH:
			pos.y--;
			break;
		case SOUTH:
			pos.y++;
			break;
		case WEST:
			pos.x--;
			break;
		case EAST:
			pos.x++;
			break;
		case SOUTHWEST:
			pos.x--;
			pos.y++;
			break;
		case NORTHWEST:
			pos.x--;
			pos.y--;
			break;
		case NORTHEAST:
			pos.x++;
			pos.y--;
			break;
		case SOUTHEAST:
			pos.x++;
			pos.y++;
			break;
		default:
			break;
	}
	return pos;
}

struct AmmoTypeNames
{
	const char* name;
	Ammo_t ammoType;
};

struct MagicEffectNames
{
	const char* name;
	MagicEffectClasses effect;
};

struct ShootTypeNames
{
	const char* name;
	ShootType_t shoot;
};

struct AmmoActionNames
{
	const char* name;
	AmmoAction_t ammoAction;
};

struct CombatTypeNames
{
	const char* name;
	CombatType_t combat;
};

MagicEffectNames magicEffectNames[] =
{
	{"redspark",		NM_ME_DRAW_BLOOD},
	{"bluebubble",		NM_ME_LOSE_ENERGY},
	{"poff",		NM_ME_POFF},
	{"puff",		NM_ME_POFF},
	{"yellowspark",		NM_ME_BLOCKHIT},
	{"explosionarea",	NM_ME_EXPLOSION_AREA},
	{"explosion",		NM_ME_EXPLOSION_DAMAGE},
	{"firearea",		NM_ME_FIRE_AREA},
	{"yellowbubble",	NM_ME_YELLOW_RINGS},
	{"greenbubble",		NM_ME_POISON_RINGS},
	{"blackspark",		NM_ME_HIT_AREA},
	{"teleport",		NM_ME_TELEPORT},
	{"energyarea",		NM_ME_TELEPORT},
	{"energy",		NM_ME_ENERGY_DAMAGE},
	{"blueshimmer",		NM_ME_MAGIC_ENERGY},
	{"redshimmer",		NM_ME_MAGIC_BLOOD},
	{"greenshimmer",	NM_ME_MAGIC_POISON},
	{"fire",		NM_ME_HITBY_FIRE},
	{"greenspark",		NM_ME_POISON},
	{"mortarea",		NM_ME_MORT_AREA},
	{"greennote",		NM_ME_SOUND_GREEN},
	{"rednote",		NM_ME_SOUND_RED},
	{"poison",		NM_ME_POISON_AREA},
	{"yellownote",		NM_ME_SOUND_YELLOW},
	{"purplenote",		NM_ME_SOUND_PURPLE},
	{"bluenote",		NM_ME_SOUND_BLUE},
	{"whitenote",		NM_ME_SOUND_WHITE},
	{"bubbles",		NM_ME_BUBBLES},
	{"dice",		NM_ME_CRAPS},
	{"giftwraps",		NM_ME_GIFT_WRAPS},
	{"yellowfirework",	NM_ME_FIREWORK_YELLOW},
	{"redfirework",		NM_ME_FIREWORK_RED},
	{"bluefirework",	NM_ME_FIREWORK_BLUE},
};

ShootTypeNames shootTypeNames[] =
{
	{"spear",		NM_SHOOT_SPEAR},
	{"bolt",		NM_SHOOT_BOLT},
	{"arrow",		NM_SHOOT_ARROW},
	{"fire",		NM_SHOOT_FIRE},
	{"energy",		NM_SHOOT_ENERGY},
	{"poisonarrow",		NM_SHOOT_POISONARROW},
	{"burstarrow",		NM_SHOOT_BURSTARROW},
	{"throwingstar",	NM_SHOOT_THROWINGSTAR},
	{"throwingknife",	NM_SHOOT_THROWINGKNIFE},
	{"smallstone",		NM_SHOOT_SMALLSTONE},
	{"death",		NM_SHOOT_DEATH},
	{"suddendeath",		NM_SHOOT_DEATH},
	{"largerock",		NM_SHOOT_LARGEROCK},
	{"snowball",		NM_SHOOT_SNOWBALL},
	{"powerbolt",		NM_SHOOT_POWERBOLT},
	{"poison",		NM_SHOOT_POISONFIELD},
	{"infernalbolt",	NM_SHOOT_INFERNALBOLT},
};

CombatTypeNames combatTypeNames[] =
{
	{"physical",		COMBAT_PHYSICALDAMAGE},
	{"energy",		COMBAT_ENERGYDAMAGE},
	{"poison",		COMBAT_POISONDAMAGE},
	{"earth",		COMBAT_POISONDAMAGE},
	{"fire",		COMBAT_FIREDAMAGE},
	{"undefined",		COMBAT_UNDEFINEDDAMAGE},
	{"lifedrain",		COMBAT_LIFEDRAIN},
	{"manadrain",		COMBAT_MANADRAIN},
	{"healing",		COMBAT_HEALING},
	{"heal",		COMBAT_HEALING},
	{"drown",		COMBAT_DROWNDAMAGE},
};

AmmoTypeNames ammoTypeNames[] =
{
	{"spear",		AMMO_SPEAR},
	{"bolt",		AMMO_BOLT},
	{"arrow",		AMMO_ARROW},
	{"poisonarrow",		AMMO_ARROW},
	{"burstarrow",		AMMO_ARROW},
	{"throwingstar",	AMMO_THROWINGSTAR},
	{"throwingknife",	AMMO_THROWINGKNIFE},
	{"smallstone",		AMMO_STONE},
	{"largerock",		AMMO_STONE},
	{"snowball",		AMMO_SNOWBALL},
	{"powerbolt",		AMMO_BOLT},
	{"infernalbolt",	AMMO_BOLT},
};

AmmoActionNames ammoActionNames[] =
{
	{"move",		AMMOACTION_MOVE},
	{"moveback",		AMMOACTION_MOVEBACK},
	{"removecharge",	AMMOACTION_REMOVECHARGE},
	{"removecount",		AMMOACTION_REMOVECOUNT}
};

MagicEffectClasses getMagicEffect(const std::string& strValue)
{
	for(uint32_t i = 0; i < sizeof(magicEffectNames) / sizeof(MagicEffectNames); ++i)
	{
		if(strcasecmp(strValue.c_str(), magicEffectNames[i].name) == 0)
			return magicEffectNames[i].effect;
	}
	return NM_ME_UNK;
}

ShootType_t getShootType(const std::string& strValue)
{
	for(uint32_t i = 0; i < sizeof(shootTypeNames) / sizeof(ShootTypeNames); ++i)
	{
		if(strcasecmp(strValue.c_str(), shootTypeNames[i].name) == 0)
			return shootTypeNames[i].shoot;
	}
	return NM_SHOOT_UNK;
}

Ammo_t getAmmoType(const std::string& strValue)
{
	for(uint32_t i = 0; i < sizeof(ammoTypeNames) / sizeof(AmmoTypeNames); ++i)
	{
		if(strcasecmp(strValue.c_str(), ammoTypeNames[i].name) == 0)
			return ammoTypeNames[i].ammoType;
	}
	return AMMO_NONE;
}

AmmoAction_t getAmmoAction(const std::string& strValue)
{
	for(uint32_t i = 0; i < sizeof(ammoActionNames) / sizeof(AmmoActionNames); ++i)
	{
		if(strcasecmp(strValue.c_str(), ammoActionNames[i].name) == 0)
			return ammoActionNames[i].ammoAction;
	}
	return AMMOACTION_NONE;
}

std::string getSkillName(uint16_t skillid)
{
	switch(skillid)
	{
		case SKILL_FIST:
			return "fist fighting";

		case SKILL_CLUB:
			return "club fighting";

		case SKILL_SWORD:
			return "sword fighting";

		case SKILL_AXE:
			return "axe fighting";

		case SKILL_DIST:
			return "distance fighting";

		case SKILL_SHIELD:
			return "shielding";

		case SKILL_FISH:
			return "fishing";

		case SKILL__MAGLEVEL:
			return "magic level";

		case SKILL__LEVEL:
			return "level";

		default:
			return "unknown";
	}
}

skills_t getSkillId(std::string param)
{
	if(param == "fist")
		return SKILL_FIST;
	else if(param == "club")
		return SKILL_CLUB;
	else if(param == "sword")
		return SKILL_SWORD;
	else if(param == "axe")
		return SKILL_AXE;
	else if(param == "distance" || param == "dist")
		return SKILL_DIST;
	else if(param == "shielding" || param == "shield")
		return SKILL_SHIELD;
	else if(param == "fishing" || param == "fish")
		return SKILL_FISH;
	else
		return SKILL_FIST;
}

std::string getReason(int32_t reasonId)
{
	switch(reasonId)
	{
		case 0: return "Offensive Name"; break;
		case 1: return "Invalid Name Format"; break;
		case 2: return "Unsuitable Name"; break;
		case 3: return "Name Inciting Rule Violation"; break;
		case 4: return "Offensive Statement"; break;
		case 5: return "Spamming"; break;
		case 6: return "Illegal Advertising"; break;
		case 7: return "Off-Topic Public Statement"; break;
		case 8: return "Non-English Public Statement"; break;
		case 9: return "Inciting Rule Violation"; break;
		case 10: return "Bug Abuse"; break;
		case 11: return "Game Weakness Abuse"; break;
		case 12: return "Using Unofficial Software to Play"; break;
		case 13: return "Hacking"; break;
		case 14: return "Multi-Clienting"; break;
		case 15: return "Account Trading or Sharing"; break;
		case 16: return "Threatening Gamemaster"; break;
		case 17: return "Pretending to Have Influence on Rule Enforcement"; break;
		case 18: return "False Report to Gamemaster"; break;
		case 19: return "Destructive Behaviour"; break;
		case 20: return "Excessive Unjustified Player Killing"; break;
		case 21: return "Invalid Payment"; break;
		case 22: return "Spoiling Auction"; break;
		default: return "Unknown Reason"; break;
	}
}

std::string getAction(int32_t actionId, bool IPBanishment)
{
	std::string action;
	switch(actionId)
	{
		case 0: action = "Notation"; break;
		case 1: action = "Name Report"; break;
		case 2: action = "Banishment"; break;
		case 3: action = "Name Report + Banishment"; break;
		case 4: action = "Banishment + Final Warning"; break;
		case 5: action = "Name Report + Banishment + Final Warning"; break;
		case 6: action = "Statement Report"; break;
		default: action = "Deletion"; break;
	}

	if(IPBanishment)
		action += " + IP Banishment";

	return action;
}

std::string formatDate(time_t _time/* = 0*/)
{
	if(!_time)
		_time = time(NULL);

	const tm* tms = localtime(&_time);
	std::stringstream s;
	if(tms)
		s << tms->tm_mday << "/" << (tms->tm_mon + 1) << "/" << (tms->tm_year + 1900) << " " << tms->tm_hour << ":" << tms->tm_min << ":" << tms->tm_sec;
	else
		s << "UNIX Time: " << (int32_t)_time;

	return s.str();
}

std::string formatDateEx(time_t _time/* = 0*/, std::string format/* = "%d %b %Y, %H:%M:%S"*/)
{
	if(!_time)
		_time = time(NULL);

	const tm* tms = localtime(&_time);
	char buffer[100];
	if(tms)
		strftime(buffer, 25, format.c_str(), tms);
	else
		sprintf(buffer, "UNIX Time: %d", (int32_t)_time);

	return buffer;
}

std::string formatTime(time_t _time/* = 0*/, bool ms/* = false*/)
{
	if(!_time)
		_time = time(NULL);
	else if(ms)
		ms = false;

	const tm* tms = localtime(&_time);
	std::stringstream s;
	if(tms)
	{
		s << tms->tm_hour << ":" << tms->tm_min << ":";
		if(tms->tm_sec < 10)
			s << "0";

		s << tms->tm_sec;
		if(ms)
		{
			timeb t;
			ftime(&t);

			s << "."; // make it format zzz
			if(t.millitm < 10)
				s << "0";

			if(t.millitm < 100)
				s << "0";

			s << t.millitm;
		}
	}
	else
		s << "UNIX Time: " << (int32_t)_time;

	return s.str();
}

bool fileExists(const char* filename)
{
	FILE* f = fopen(filename, "rb");
	bool exists = (f != NULL);
	if(exists)
		fclose(f);

	return exists;
}


bool booleanString(std::string str)
{
	toLowerCaseString(str);
	return (str == "yes" || str == "true" || str == "y" || atoi(str.c_str()) > 0);
}

std::string convertIPAddress(uint32_t ip)
{
	char buffer[17];
	sprintf(buffer, "%d.%d.%d.%d", ip & 0xFF, (ip >> 8) & 0xFF, (ip >> 16) & 0xFF, (ip >> 24));
	return buffer;
}

CombatType_t indexToCombatType(uint32_t v)
{
	if(v == 0)
		return COMBAT_FIRST;

	return (CombatType_t)(1 << (v - 1));
}

CombatType_t getCombatType(const std::string& strValue)
{
	for(uint32_t i = 0; i < sizeof(combatTypeNames) / sizeof(CombatTypeNames); ++i)
	{
		if(strcasecmp(strValue.c_str(), combatTypeNames[i].name) == 0)
			return combatTypeNames[i].combat;
	}
	return COMBAT_NONE;
}

std::string getCombatName(CombatType_t combatType)
{
	for(uint32_t i = 0; i < sizeof(combatTypeNames) / sizeof(CombatTypeNames); ++i)
	{
		if(combatTypeNames[i].combat == combatType)
			return combatTypeNames[i].name;
	}
	return "unknown";
}

uint32_t combatTypeToIndex(CombatType_t combatType)
{
	switch(combatType)
	{
		case COMBAT_NONE: return 0;
		case COMBAT_PHYSICALDAMAGE: return 1;
		case COMBAT_ENERGYDAMAGE: return 2;
		case COMBAT_POISONDAMAGE: return 3;
		case COMBAT_FIREDAMAGE: return 4;
		case COMBAT_UNDEFINEDDAMAGE: return 5;
		case COMBAT_LIFEDRAIN: return 6;
		case COMBAT_MANADRAIN: return 7;
		case COMBAT_HEALING: return 8;
		case COMBAT_DROWNDAMAGE: return 9;
		default: return 0;
	}
}

std::string getFilePath(FileType_t type, std::string name/* = ""*/)
{
	#ifdef __FILESYSTEM_HIERARCHY_STANDARD__
	std::string path = "/var/lib/tfs/";
	#endif
	std::string path = "data/";
	switch(type)
	{
		case FILE_TYPE_OTHER:
		{
			path += name;
			break;
		}
		case FILE_TYPE_XML:
		{
			path += "XML/" + name;
			break;
		}	
		case FILE_TYPE_LOG:
		{
			#ifndef __FILESYSTEM_HIERARCHY_STANDARD__
			path = "data/logs/" + name;
			#else
			path = "/var/log/tfs/" + name;
			#endif
			break;
		}	
		case FILE_TYPE_CONFIG:
		{
			#if defined(__HOMEDIR_CONF__)
			if(fileExists("~/.tfs/" + name))
				path = "~/.tfs/" + name;
			else
			#endif
			#if defined(__FILESYSTEM_HIERARCHY_STANDARD__)
				path = "/etc/tfs/" + name;
			#else
				path = name;
			#endif
			break;
		}
		default:
			std::clog << "> ERROR: Wrong file type!" << std::endl;
			break;
	}
	return path;
}
