#pragma once

#include <stdint.h>
#include <map>
#include <vector>
#include <string.h>
#include <iostream>
#include <fstream>
#include "tinyxml2.h"
#include <memory>
#include "AttributeBase.h"

using namespace std;

//vk key codes https://msdn.microsoft.com/en-us/library/windows/desktop/dd375731%28v=vs.85%29.aspx





class InvalidXMLDataException : public exception
{
public:
	InvalidXMLDataException() : exception() {}
	InvalidXMLDataException(const std::string& str) : exception(str.c_str()) {}
};

class IndexOutOfRangeException : public exception
{
public:
	IndexOutOfRangeException() : exception() {}
	IndexOutOfRangeException(const std::string& str) : exception(str.c_str()) {}
};

class InvalidOperationException : public exception
{
public:
	InvalidOperationException() : exception() {}
	InvalidOperationException(const std::string& str) : exception(str.c_str()) {}
};

class SettingAttributeFactory
{
public:

	//throws InvalidXMLDataException
	static std::shared_ptr<AttributeBase> CreateTypedAttribute(const tinyxml2::XMLElement& attrElement);

private:

	static std::shared_ptr<AttributeBase> ProcessIntAttributeCase(const tinyxml2::XMLElement& attrElement);

	static std::shared_ptr<AttributeBase> ProcessFloatAttributeCase(const tinyxml2::XMLElement& attrElement);


};

//Handles reading application config file

class SettingsMgr
{
public:

	//constructors
	SettingsMgr();

	//param: What directory should we look for an existing settings file in
	SettingsMgr(const string& directory);
	
	//set current folder which will or should contain settings
	void setCurDir(const string& dir) { curDirectory = dir; }

	//get current dir
	string getCurDir() const { return curDirectory; }

	static inline const std::map<string, SETTING_VALUE_TYPE>&  getAttributesToEditMap() { return attributesToEdit; }

	const std::shared_ptr<AttributeBase>& getAttributeObjectByIndex(int index) const 
	{
		if (index >= attributeSettings.size())
			throw IndexOutOfRangeException("Index out of range");
		return attributeSettings[index];
	}

	int getAttributeObjectCount() const { return attributeSettings.size(); }

	//This is the KEY= part in the AA keybinds.cfg, default was what.... NUMPAD5 I believe, but they can change in settings file
	const string&	 getKeybindPrefix() const;

	const bool getIsLoggingEnabled() const { return loggingEnabled; }

	const string getLogFileName() const { return "LogWeaponEditor_LastRun.txt"; }

	void   setVKFilePath(const string&);

	//bool	getSettingKeycodeFromVirtualKeycode(uint32_t inVirtualKey, SETTINGS_INDICES& outSettingKeycode);

	//create settings file with default values
	bool   createSettingsFile();

	uint32_t getNextWeaponKey() const { return nextWeaponKey; }

	uint32_t getPrevWeaponKey() const { return prevWeaponKey; }

	uint32_t getAAKeybindKey() const { return aaKeybindKey; }


	static const string XML_SETTINGS_FILE_NAME;

	//Make this a global static or something at some point, isntead of defining it multiple times across multiple classes
	static const float floatEpsilon;

protected:

	bool		tryParseXMLSettings(const string&);

	string		curDirectory;

	string		keybindprefix;
	string		vkkeycodespath;

	//Have we succesfully opened and read a Settings.xml file
	bool		isReady;

private:

	//static map with definitions for what properties we want to edit
	static std::map<string, SETTING_VALUE_TYPE> attributesToEdit;

	//Store AttributeBase subclasses here, use shared_ptr as we will be iterating and such and I don't want the stl constantly destructing/creating or moving these
	std::vector<std::shared_ptr<AttributeBase>> attributeSettings;

	uint32_t aaKeybindKey;

	uint32_t nextWeaponKey;

	uint32_t prevWeaponKey;

	bool loggingEnabled;

};