#pragma once

#include <Windows.h>
#include "WeaponCFG.h"
#include <iostream>
#include "SettingsMgr.h"
#include <string>

using std::ifstream;
using std::string;

const string NEW_WEAPON_NAME = "newweaps";

class ConfigFileManager
{

public:

	ConfigFileManager();

	bool addNewKeyBind(const std::string&);

	bool findAndStoreWeaponFilePath();
	bool findAndStoreKeybindFilePath();
	bool findAndStoreKeyValueNamesPath();

	const std::string& getApplicationDirectoryPath() const { return appDirectoryPath; }

	const std::string& getWeaponFilePath() const { return weaponFilePath; }
	const std::string& getKeybindFilePath() const { return keybindFilePath; }
	const std::string& getKeyValueNamesFilePath() const { return keyValueNamesFilePath; }
	const std::string& getNewWeaponFilePath() const { return newWeaponFilePath; }

	bool   resetSettingsXmlFile() { return settingMgmt.createSettingsFile(); }

	bool writeOutWeaponConfigFile(const WeaponConfig& rWpnConfig);

	void initialize();


	//bool isSpeedScaleEnabled() const { return settingMgmt.getIsSpeedScaleEditingEnabled(); }

	

protected:

	SettingsMgr settingMgmt;

	string keybindFilePath;
	string weaponFilePath;
	string keyValueNamesFilePath;
	string newWeaponFilePath;

	string appDirectoryPath;

};