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

#ifndef __OTSERV_TOOLS_H__
#define __OTSERV_TOOLS_H__

#include "definitions.h"
#include "otsystem.h"
#include "position.h"
#include "const.h"
#include "enums.h"

#include <string>
#include <algorithm>

#include <boost/tokenizer.hpp>
typedef boost::tokenizer<boost::char_separator<char> > tokenizer;

enum FileType_t
{
	FILE_TYPE_XML,
	FILE_TYPE_LOG,
	FILE_TYPE_OTHER,
	FILE_TYPE_CONFIG
};

enum DistributionType_t
{
	DISTRO_UNIFORM,
	DISTRO_SQUARE,
	DISTRO_NORMAL
};

std::string transformToMD5(std::string plainText, bool upperCase = false);
std::string transformToSHA1(std::string plainText, bool upperCase = false);
bool passwordTest(const std::string &plain, std::string &hash);

void replaceString(std::string& str, const std::string sought, const std::string replacement);
void trim_right(std::string& source, const std::string& t);
void trim_left(std::string& source, const std::string& t);
void toLowerCaseString(std::string& source);
void toUpperCaseString(std::string& source);
std::string asLowerCaseString(const std::string& source);
std::string asUpperCaseString(const std::string& source);

std::vector<std::string> explodeString(const std::string& inString, const std::string& separator);
std::vector<int32_t> vectorAtoi(std::vector<std::string> stringVector);
bool hasBitSet(uint32_t flag, uint32_t flags);

std::string generateRecoveryKey(int32_t fieldCount, int32_t fieldLength);

bool isNumber(char character);
bool isPasswordCharacter(char character);
bool isValidName(std::string text, bool forceUppercaseOnFirstLetter = true);
bool isUppercaseLetter(char character);
bool isLowercaseLetter(char character);
bool isValidPassword(std::string text);
bool isNumbers(std::string text);

bool checkText(std::string text, std::string str);

int random_range(int lowest_number, int highest_number, DistributionType_t type = DISTRO_UNIFORM);

Direction getDirection(std::string string);
Direction getReverseDirection(Direction dir);
Position getNextPosition(Direction direction, Position pos);

char upchar(char c);

std::string parseParams(tokenizer::iterator &it, tokenizer::iterator end);
std::string trimString(std::string& str);

std::string formatDate(time_t _time = 0);
std::string formatDateEx(time_t _time = 0, std::string format = "%d %b %Y, %H:%M:%S");
std::string formatTime(time_t _time = 0, bool miliseconds = false);

MagicEffectClasses getMagicEffect(const std::string& strValue);
ShootType_t getShootType(const std::string& strValue);
Ammo_t getAmmoType(const std::string& strValue);
AmmoAction_t getAmmoAction(const std::string& strValue);

std::string getSkillName(uint16_t skillid);
skills_t getSkillId(std::string param);

std::string getReason(int32_t reasonId);
std::string getAction(int32_t actionId, bool IPBanishment);

bool fileExists(const char* filename);
bool booleanString(std::string);

std::string convertIPAddress(uint32_t ip);

CombatType_t getCombatType(const std::string& strValue);
std::string getCombatName(CombatType_t combatType);
CombatType_t indexToCombatType(uint32_t v);
uint32_t combatTypeToIndex(CombatType_t combatType);

std::string getFilePath(FileType_t type, std::string name = "");
#endif
