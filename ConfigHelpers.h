#pragma once

#include <string>
#include <regex>
#include <stdio.h>
#include <ostream>

using namespace std;

enum STRINGHELPER_WEAPON_CONSTANTS
{
	CLOSETAG_LEN = 11,
	MINWEAPON_LEN = 18
};

//quick exception type so we can differentiate between error exception and not found exception
class MatchNotFoundException : public exception
{
public:
	MatchNotFoundException() : exception() {}
	MatchNotFoundException(const std::string& str) : exception(str.c_str()) {}
};

class StringHelpers
{
public:
	template <typename T>
	static T GetValueFromTaggedAttribute(const string& configLine, const string& attributeName, bool makeSureClosedOnSameLine);

	//Extract value from lines in the format of MySettingName = somevalue
	template <typename T>
	static T GetValueFromSettingEqualsLine(const string& targetStr, const string& attributeName);

	/*static int CopyNextWeaponEntryString(const string & weaponConfigString, string& outNewWweaponString);*/

	static const STRINGHELPER_WEAPON_CONSTANTS CLOSE_WEAPON_LENGTH = CLOSETAG_LEN;
	static const STRINGHELPER_WEAPON_CONSTANTS MIN_WEAPON_LEN = MINWEAPON_LEN;


private:
	//include T in arguments so compiler can deduce which overload to call automatically
	template <typename T>
	static void ReturnValueOfTFromString(const string& convertArg, T& returnConverted);

};

//different types may have different default values. built in types like ints or float will just be zero, so throw an exception
//if it fails instead of returning default.
template<typename T>
inline T StringHelpers::GetValueFromTaggedAttribute(const string& configLine, const string& attributeName, bool makeSureClosedOnSameLine)
{
	if (configLine.empty())
		throw exception("configLine was empty");
	else if (attributeName.empty())
		throw exception("attributeName was empty");


	//use regex instead of manually checking stuff.
	//^ = start of line, \s* = 0 or more whitespaces, <, \s*, attributeName, \s*, then capture all characters before the > and store in a group, >
	//if we need to make sure closed on same line, check for the closing brace too. $ means match at end of line
	std::regex regexPattern( "^\\s*<\\s*(" + attributeName + ")\\s*([^\\s^>]+)\\s*>\\s*" + (makeSureClosedOnSameLine ? "\\s*<\\s*/\\s*" + attributeName + "\\s*>\\s*$" : "$") );

	//will be captured in group #2 if it matched
	std::smatch regexMatchResults;

	if (!std::regex_search(configLine, regexMatchResults, regexPattern))
		throw MatchNotFoundException("configLine did not match the expected pattern");
	else if (!regexMatchResults.ready() || regexMatchResults.empty() || regexMatchResults.size() != 3)	/* should be 3 matches, attribute value is last*/
		throw MatchNotFoundException("configLine did not match the expected pattern");

	T retVal;

	ReturnValueOfTFromString(regexMatchResults[2].str(), retVal);

	return retVal;
}

//generic case, won't compile if they don't have a T constructor accepting string
template <typename T>
inline void StringHelpers::ReturnValueOfTFromString(const string& convertArg, T& returnConverted)
{
	if (convertArg.empty())
		throw exception("convertArg was empty");

	returnConverted = T(convertArg);
}

//string case
template <>
inline void StringHelpers::ReturnValueOfTFromString<string>(const string& convertArg, string& returnConverted)
{
	if (convertArg.empty())
		throw exception("convertArg was empty");

	returnConverted = convertArg;
}

//int case
template<>
inline void StringHelpers::ReturnValueOfTFromString<int>(const string& convertArg, int& returnConverted)
{

	try
	{
		returnConverted = std::stoi(convertArg);
	}
	catch (exception& ex)
	{
		throw ex;
	}

}

//float case
template<>
inline void StringHelpers::ReturnValueOfTFromString<float>(const string& convertArg, float& returnConverted)
{
	

	try
	{
		returnConverted = std::stof(convertArg);
	}
	catch (exception& ex)
	{
		throw ex;
	}

}


//get a float value from settings line
template<>
inline float StringHelpers::GetValueFromSettingEqualsLine<float>(const string & targetStr, const string & attributeName)
{

	//use regex instead of manually checking stuff.
	//^ = start of line, \s* = 0 or more whitespaces, =, \s*, attributeName, \s*, then capture all characters before the end of the line that arent a space. $ means match at end of line
	std::regex regexPattern("^\\s*" + attributeName + "\\s*=\\s*([^\\s]+)\\s*$");

	//will be captured in group #2 if it matched
	std::smatch regexMatchResults;

	//search the target line
	if (!std::regex_search(targetStr, regexMatchResults, regexPattern))
		throw exception("configLine did not match the expected pattern");
	else if (!regexMatchResults.ready() || regexMatchResults.empty() || regexMatchResults.size() != 2)	/* should be 2 matches, attribute value is last*/
		throw exception("configLine did not match the expected pattern");
	else
	{
		string floatValueInString = regexMatchResults[1].str();
		float returnValue = 0;
		try
		{
			returnValue = std::stof(floatValueInString);
			return returnValue;
		}
		catch (exception& ex)
		{
			//eh just throw it
			throw;
		}
	}

}

//get an t value from settings lineint
template<>
inline int StringHelpers::GetValueFromSettingEqualsLine<int>(const string & targetStr, const string & attributeName)
{

	//use regex instead of manually checking stuff.
	//^ = start of line, \s* = 0 or more whitespaces, =, \s*, attributeName, \s*, then capture all characters before the end of the line that arent a space. $ means match at end of line
	std::regex regexPattern("^\\s*" + attributeName + "\\s*=\\s*([^\\s]+)\\s*$");

	//will be captured in group #2 if it matched
	std::smatch regexMatchResults;

	if (!std::regex_search(targetStr, regexMatchResults, regexPattern))
		throw exception("configLine did not match the expected pattern");
	else if (!regexMatchResults.ready() || regexMatchResults.empty() || regexMatchResults.size() != 2)	/* should be 2 matches, attribute value is last*/
		throw exception("configLine did not match the expected pattern");
	else
	{
		string floatValueInString = regexMatchResults[1].str();
		int returnValue = 0;
		try
		{
			returnValue = std::stoi(floatValueInString);
			return returnValue;
		}
		catch (exception& ex)
		{
			//eh just throw it
			throw;
		}
	}

}

//get a float value from settings line
template<typename T>
inline T StringHelpers::GetValueFromSettingEqualsLine(const string & targetStr, const string & attributeName)
{

	//use regex instead of manually checking stuff.
	//^ = start of line, \s* = 0 or more whitespaces, =, \s*, attributeName, \s*, then capture all characters before the end of the line that arent a space. $ means match at end of line
	std::regex regexPattern("^\\s*" + attributeName + "\\s*=\\s*([^\\s]+)\\s*$");

	//will be captured in group #2 if it matched
	std::smatch regexMatchResults;

	if (!std::regex_search(targetStr, regexMatchResults, regexPattern))
		throw exception("configLine did not match the expected pattern");
	else if (!regexMatchResults.ready() || regexMatchResults.empty() || regexMatchResults.size() != 3)	/* should be 3 matches, attribute value is last*/
		throw exception("configLine did not match the expected pattern");

}