#pragma once

#include <stdio.h>
#include <stdint.h>
#include <fstream>
#include <cstdlib>
#include <vector>
#include "WeaponEntry.h"
#include <sstream>
#include "SettingsMgr.h"


using namespace std;

//Defines success and failure indicators returned from readWEaponConfigFile
enum ReadWeaponConfigResults
{
	ZeroWeaponsParsed = -4,
	FailedToReadConfigFile = -3,
	ConfigFilePathEmpty = -2,
	AttributeNotUniqueToFiremode = -1,
	Success = 0

};

//Thrown when user specified a non-firemode-unique attribute in Settings.xml
class AttributeNotUniqueException : public exception
{
public:
	AttributeNotUniqueException() : exception() {}
	AttributeNotUniqueException(const std::string& str) : exception(str.c_str()) {}
};

//Stores the container of WeaponCOnfigEntries and provides an interface to parse weapon config files & access to stored WEaopnConfigEntries.
class WeaponConfig
{

public:

	ReadWeaponConfigResults readWeaponConfigFile(const string& cfgFilePath);

	int getWeaponConfigEntryCount() const { return configWeaponEntries.size(); }

	WeaponConfigEntry& getWeaponConfigAtIndex(int index) { if (index >= configWeaponEntries.size()) throw exception("Index out of range"); else return configWeaponEntries[index]; }

private:

	//implementation of parsing weapon entries
	int parseWeaponEntries(const std::vector<string>& wpnStrings);

	int SplitWeaponEntryIntoLinesAndParse(const std::string& weaponCfgString, WeaponConfigEntry& curWpnObj);

	//the actual weapon entries
	std::vector<WeaponConfigEntry> configWeaponEntries;

	friend class ConfigFileManager;

};