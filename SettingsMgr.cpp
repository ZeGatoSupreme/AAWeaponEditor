#pragma once

#include "SettingsMgr.h"
#include "ConfigHelpers.h"
#include <stdint.h>
#include <map>
#include <string.h>
#include <sstream>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <stdio.h>
#include <vector>
#include <Windows.h>
#include "tinyxml2.h"
#include "DefaultSettings.h"
#include <memory>
#include "LoggerMgr.h"

using namespace tinyxml2;



const string SettingsMgr::XML_SETTINGS_FILE_NAME = "Settings.xml";

std::map<string, SETTING_VALUE_TYPE> SettingsMgr::attributesToEdit;

//precision to which we will check when comparing floats
const float SettingsMgr::floatEpsilon = 0.000000001;


//default no args constructor
SettingsMgr::SettingsMgr() :
	curDirectory(""), isReady(false), keybindprefix(""),
	vkkeycodespath(""), aaKeybindKey(0), nextWeaponKey(0), prevWeaponKey(0), loggingEnabled(false)
{

}


//This constructor needs to be refactored, old code was a mess and this just tries to wallpaper it over.
//constructor which takes a string path to the directory the exe was run from
SettingsMgr::SettingsMgr(const string& directory) :
	curDirectory(directory), isReady(false), keybindprefix(""),
	vkkeycodespath(""), aaKeybindKey(0), nextWeaponKey(0), prevWeaponKey(0), loggingEnabled(false)
{

	if (directory.empty())
	{
		setCurDir("");
		std::cout << "Could not get directory path the program is executing in. Pls move to a normal and/or shorter folder, run as admin, try again. Using default settings.\n";
		return;
	}

	string searchConfigFileInDir = directory;

	searchConfigFileInDir.push_back('\\');
	searchConfigFileInDir.append(XML_SETTINGS_FILE_NAME);


	//boiler plate file searching code from msdn
	HANDLE hSearchHandle;
	WIN32_FIND_DATA tempResult;
	LPWIN32_FIND_DATA sResult = &tempResult;

	hSearchHandle = FindFirstFile(searchConfigFileInDir.c_str(), sResult);

	if (hSearchHandle == INVALID_HANDLE_VALUE)
	{
		//Try to create the settings file if it is missing
		if (!createSettingsFile())
		{
			std::cout << "Settings file not in directory and couldn't create settings file, run as admin, check folder, verify persmissions, then try again." << endl;
			exit(-1);
		}
		else
		{
			hSearchHandle = FindFirstFile(searchConfigFileInDir.c_str(), sResult); 
			if (hSearchHandle == INVALID_HANDLE_VALUE)
			{
				std::cout << "Created Settings.xml however FindFirstFile could not locate it. Clearly something is off. Verify permissions, run as admin, yadda yadda, try again." << std::endl;
				exit(-1);
			}

			std::cout << "Settings.xml not found, created file with default settings." << std::endl;

		}

	}

	//Make sure they don't randomly have a file which begins with Settings.xml but is not the Settings.xml
	//such as "Settings.xml Herp Derp.txt"

	bool foundSettings = false;

	while (hSearchHandle != INVALID_HANDLE_VALUE && !foundSettings)
	{
		//we only want an exact match
		if (!XML_SETTINGS_FILE_NAME.compare(sResult->cFileName))
		{
			//Found our file
			foundSettings = true;
			FindClose(hSearchHandle);
			break;
		}
		else
		{
			if (!FindNextFile(hSearchHandle, sResult))
			{
				FindClose(hSearchHandle);
				hSearchHandle = INVALID_HANDLE_VALUE;
			}
		}

	}

	if (!foundSettings)
	{
		//They don't have a file named Settings.xml or the search failed.
		//Either way, try to create the default file and use default vals

		if (!createSettingsFile())
		{
			std::cout << "Couldn't create settings file, and Settings.xml not in directory. Run as admin and/or move to normal folder." << endl;
			exit(-1);
		}

		std::cout << "Settings.xml not found, created file with default settings." << std::endl;
	}

	bool readSettingsSuccess = false;

	string xmlSettingsPath("");

	try
	{
		xmlSettingsPath = directory;

		xmlSettingsPath.push_back('\\');

		xmlSettingsPath.append(XML_SETTINGS_FILE_NAME);

		if (!(readSettingsSuccess = tryParseXMLSettings(xmlSettingsPath)))
		{
			std::cout << "Problem reading Settings.xml, using default values and creatings new Settings.xml\n";
		}
	}
	catch (exception& ex)
	{
		readSettingsSuccess = false;
		std::cout << "Problem reading Settings.xml, using default values and creatings new Settings.xml\n";
	}

	//Attempt to create and read default Settings.xml if needed

	if (!readSettingsSuccess)
	{
		try
		{
			if (!createSettingsFile())
			{
				std::cout << "Could not create default Settings.xml file. Please make sure you don't have it open in notepad or anything, and that program is running as admin. Do some basic troubleshooting basically. Change dir if needed.";
				exit(-1);
			}

			//Created default settings.xml, try to read it
			if (!tryParseXMLSettings(xmlSettingsPath))
			{
				std::cout << "Created default Settings.xml but could not read it. Please make sure program is run as admin, no programs are interfering with it, and that the path name is not some weird path.";
				exit(-1);
			}

			std::cout << "Settings.xml not found, created file with default settings." << std::endl;
		}
		catch (exception& ex)
		{
			std::cout << "Could not create and read default Settings.xml. Please make sure program is run as admin, no other programs are interfering with it, and that you don't have a Settings.xml open in a text editor. Change dir if its a weird path and try again.";
			exit(-1);
		}
	}

	//If we make it here, we have read a settings file sucessfully
	isReady = true;

	//clear current list of attributes to edit, we have new values to insert
	attributesToEdit.clear();

	//For quick access during weapons parsing, store a pair consisting of attribute name and type in a map. We can then check each config line against
	//the attribute name.
	for each (std::shared_ptr<AttributeBase> pAtr in attributeSettings)
	{
		attributesToEdit.insert(std::make_pair(pAtr->AttributeName(), pAtr->AttributeType()));
	}

}


//XML version, try to parse Settings.xml
bool		SettingsMgr::tryParseXMLSettings(const string& settingsReadData)
{

	attributeSettings.clear();

	tinyxml2::XMLDocument doc;

	if (doc.LoadFile(settingsReadData.c_str()) != tinyxml2::XMLError::XML_SUCCESS)
		return false;

	//Need to do some basic parsing here
	const tinyxml2::XMLNode* pEle = nullptr;

	const tinyxml2::XMLNode* pRootNode = nullptr;

	const tinyxml2::XMLElement* pEleAsTag = nullptr;

	//Check AAKeyBindKey, NextWeaponKey, PrevWeaponKey before parsing attributes
	uint32_t aaKey, nextKey, prevKey;

	//Ugh need to get root node before finding children elements
	if ((pRootNode = doc.FirstChildElement("WeaponEditorSettings")) == nullptr)
		return false;


	if ((pEle = pRootNode->FirstChildElement("AAKeyBindKey")) == nullptr)
		return false;
	else if ((pEleAsTag = pEle->ToElement()) == nullptr)
		return false;
	else if (pEleAsTag->QueryUnsignedAttribute("key", &aaKey) != tinyxml2::XMLError::XML_SUCCESS)
		return false;

	if ((pEle = pRootNode->FirstChildElement("NextWeaponKey")) == nullptr)
		return false;
	else if ((pEleAsTag = pEle->ToElement()) == nullptr)
		return false;
	else if (pEleAsTag->QueryUnsignedAttribute("key", &nextKey) != tinyxml2::XMLError::XML_SUCCESS)
		return false;

	if ((pEle = pRootNode->FirstChildElement("PrevWeaponKey")) == nullptr)
		return false;
	else if ((pEleAsTag = pEle->ToElement()) == nullptr)
		return false;
	else if (pEleAsTag->QueryUnsignedAttribute("key", &prevKey) != tinyxml2::XMLError::XML_SUCCESS)
		return false;

	if ((pEle = pRootNode->FirstChildElement("Logging")) != nullptr && (pEleAsTag = pEle->ToElement()) != nullptr)
	{
		bool tmpLoglvl = false;
		if (pEleAsTag->QueryBoolAttribute("enabled", &tmpLoglvl) == tinyxml2::XMLError::XML_SUCCESS)
		{
			loggingEnabled = tmpLoglvl;
			if (loggingEnabled)
			{
				std::cout << "Logging enabled." << std::endl;
				if (!LogHelper::getInstance().setLoggingPathAndOpen(getLogFileName()))
				{
					std::cout << "Failed creating log file." << std::endl;
					loggingEnabled = false;
				}
			}
		}
	}

/*	if ((pEle = doc.FirstChildElement("WeaponEditorSettings")) == nullptr)
		return false;
	else */if ((pEle = pRootNode->FirstChildElement("AttributesToEdit")) == nullptr)
		return false;
	else if ((pEle = pEle->FirstChildElement("Attribute")) == nullptr)
		return false;



	std::string attrType;

	while (pEle != nullptr)
	{

		pEleAsTag = pEle->ToElement();

		if (pEleAsTag == nullptr)
		{
			pEle = pEle->NextSibling();
			continue;
		}

		if (pEleAsTag->Attribute("name") == nullptr)
		{
			pEle = pEle->NextSibling();
			continue;
		}
		else if (pEleAsTag->Attribute("type") == nullptr)
		{
			pEle = pEle->NextSibling();
			continue;
		}

		try
		{
			std::shared_ptr<AttributeBase> settingAtr = SettingAttributeFactory::CreateTypedAttribute(*pEleAsTag);

			//Check for already mapped key, already mapped attribute, etc etc
			auto attrMapIt = attributeSettings.begin();

			for (; attrMapIt != attributeSettings.end(); attrMapIt++)
			{
				if ((*attrMapIt)->AttributeName().compare(settingAtr->AttributeName()) == 0)
				{
					std::stringstream ss("");
					ss << "Attribute name " << settingAtr->AttributeName() << " has already been parsed once, duplicate attribute in settings.xml." << std::endl;
					throw InvalidXMLDataException(ss.str());
				}
				else if ((*attrMapIt)->AttributeLowerKey() == settingAtr->AttributeLowerKey())
				{
					std::stringstream ss("");
					ss << "Attribute name " << settingAtr->AttributeName() << " has the same KeyLowerValue as " << (*attrMapIt)->AttributeName() << ", skipping." << std::endl;
					throw InvalidXMLDataException(ss.str());
				}
				else if ((*attrMapIt)->AttributeRaiseKey() == settingAtr->AttributeRaiseKey())
				{
					std::stringstream ss("");
					ss << "Attribute name " << settingAtr->AttributeName() << " has the same KeyRaiseValue as " << (*attrMapIt)->AttributeName() << ", skipping." << std::endl;
					throw InvalidXMLDataException(ss.str());
				}
			}

			attributeSettings.push_back(settingAtr);
		}
		catch (InvalidXMLDataException& ex)
		{
			std::cout << "Could not parse setting attribute with name " << pEleAsTag->Attribute("name") << ", will not be able to edit this weapon attribute." << std::endl;
			std::cout << "Additional details: " << std::string(ex.what()) << std::endl;
		}

		pEle = pEle->NextSibling();

	}

	if (!attributeSettings.size())
	{
		std::cout << "Well, we made it to the end of tryParseXMLSettings, however no attributes were parsed. Your settings.xml either is screwed up, or it doesn't have any attributes. Therefore you fail! Using defaults." << std::endl;
		return false;
	}
	else
	{

		//Store AA reserved key, next & prev weapon keys
		aaKeybindKey = aaKey;
		nextWeaponKey = nextKey;
		prevWeaponKey = prevKey;

		std::cout << "Parsed " << attributeSettings.size() << " attributes from Settings.xml." << std::endl;
		return true;
	}

}


//Creates the settings file with default values if it is missing
bool SettingsMgr::createSettingsFile()
{
	string newFileName = getCurDir();
	if (newFileName.empty())
		return false;
	newFileName.push_back('\\');
	newFileName.append(XML_SETTINGS_FILE_NAME);

	ofstream fOut;
	fOut.open(newFileName, ofstream::out | ofstream::trunc | ofstream::binary);

	if (fOut.bad() || fOut.fail())
	{
		fOut.close();
		return false;
	}

	size_t defaultSettingsFileSize = sizeof(defaultSettingsBytes) / sizeof(unsigned char);

	for (size_t i = 0; i < defaultSettingsFileSize; i++)
	{
		fOut << defaultSettingsBytes[i];
	}

	if (fOut.bad() || fOut.fail())
	{
		fOut.close();
		return false;
	}

	fOut.flush();

	fOut.close();

	return true;
}


const string&	 SettingsMgr::getKeybindPrefix() const
{
	return keybindprefix;
}

//this looks for the AA KeyName for the corresponding virtual key code specified as the reserved AA keybind key in Settings.xml
void   SettingsMgr::setVKFilePath(const string& path)
{

	if (!path.empty())
		vkkeycodespath = path;
	else
	{
		vkkeycodespath = "";
		return;
	}

	//Find the AA keybind name of the vk key code
	uint32_t keyCodeToFind = aaKeybindKey;

	string strKeyCode = std::to_string(keyCodeToFind);

	if (strKeyCode.empty())
	{
		std::cout << "You done fucked up in your Settings.xml, enter a val for AA reserved key. Or delete your Settings.xml and let the program recreate them next run.\n" << endl;
		exit(-1);
	}

	ifstream reader;
	reader.open(vkkeycodespath, ios_base::in);

	if (reader.fail())
	{
		std::cout << "You are missing a needed AA config file in this folder.\n" << endl;
		exit(-1);
	}

	if (reader.eof())
	{
		std::cout << "A vital AA config file is empty.\n" << endl;
		reader.close();
		exit(-1);
	}

	string currentLine = "";
	string keyname = "";

	bool foundKeyNamesLine = false;
	
	//Look for [KeyNames]
	while (!reader.eof() && getline(reader, currentLine))
	{
		if (currentLine.find("[KeyNames]") != string::npos)
		{
			//Check for one more line after it just to be safe
			if (!reader.eof() && getline(reader, currentLine))
			{
				if (currentLine.find("Unknown0x00") != string::npos)
				{
					foundKeyNamesLine = true;
				}
			}
			
			break;
		}
	}

	if (!foundKeyNamesLine)
	{
		std::cout << "Missing [KeyNames] section.\n" << endl;
		reader.close();
		exit(-1);
	}

	bool foundKeyName = false;

	//we consumed one line below [KeyNames] so check that first
	if (currentLine.find(strKeyCode) != string::npos)
		foundKeyName = true;

	while (!reader.eof() && getline(reader, currentLine) && !foundKeyName)
	{
		if (currentLine.find(strKeyCode) == string::npos)
			continue;
		else
		{
			foundKeyName = true;
			break;
		}
	}

	//done reading
	reader.close();

	if (!foundKeyName)
	{
		std::cout << "[KeyNames] doesn't have a name for your key code.\n" << endl;
		exit(-1);
	}

	int keyNameIndex = currentLine.find("=");

	if (keyNameIndex == string::npos)
	{
		std::cout << "Your AA cfg file is messed up, let loader redownload.\n" << endl;
		exit(-1);
	}

	keyNameIndex++;

	keyname = currentLine.substr(keyNameIndex);
	if (keyname.empty())
	{
		std::cout << "Your AA cfg file is messed up, let loader redownload.\n" << endl;
		exit(-1);
	}

	//else we got it
	keybindprefix = keyname;

}

//Factory method to create an AttributeFloat or AttributeInt, depending on info in the XMLElement
std::shared_ptr<AttributeBase> SettingAttributeFactory::CreateTypedAttribute(const tinyxml2::XMLElement & attrElement)
{

	if (attrElement.Attribute("type") == nullptr)
		throw InvalidXMLDataException("type missing.");
	else if (attrElement.Attribute("name") == nullptr)
		throw InvalidXMLDataException("name missing.");
	else if (attrElement.NoChildren())
		throw InvalidXMLDataException("attribute doesnt have children nodes.");

	std::string elementAttributeName = attrElement.Name();

	//Make sure element is <Attribute> type
	if (elementAttributeName.compare("Attribute"))
		throw InvalidXMLDataException("AttributesToEdit child node was not an Attribute node.");

	std::string elementTypeName = attrElement.Attribute("type");

	//Make sure the element type and Attribute class type match. Again, string::compare returns 0 if equal.
	if (!elementTypeName.compare("int"))
		return ProcessIntAttributeCase(attrElement);
	else if (!elementTypeName.compare("float"))
		return ProcessFloatAttributeCase(attrElement);
	else
		throw InvalidXMLDataException("Atribute node did not have float or int as type.");
}

std::shared_ptr<AttributeBase> SettingAttributeFactory::ProcessIntAttributeCase(const tinyxml2::XMLElement & attrElement)
{

	//grab the name="" field, we checked for null already
	std::string wpnAttributeName = attrElement.Attribute("name");

	//find the minvalue, maxvalue, delta children
	int atrMin, atrMax, atrDelta;


	//Reuse this pointer and check each of those three values
	const tinyxml2::XMLElement* vEle = attrElement.FirstChildElement("MinValue");

	if (vEle == nullptr)
		return false;
	else if (vEle->QueryIntText(&atrMin) != tinyxml2::XMLError::XML_SUCCESS)
		return false;

	if ((vEle = attrElement.FirstChildElement("MaxValue")) == nullptr)
		return false;
	else if (vEle->QueryIntText(&atrMax) != tinyxml2::XMLError::XML_SUCCESS)
		return false;

	if ((vEle = attrElement.FirstChildElement("Delta")) == nullptr)
		return false;
	else if (vEle->QueryIntText(&atrDelta) != tinyxml2::XMLError::XML_SUCCESS)
		return false;

	//Now grab keyraise and keylower

	uint32_t vkRaiseVal, vkLowerVal;

	if ((vEle = attrElement.FirstChildElement("KeyRaiseValue")) == nullptr)
		return false;
	else if (vEle->QueryUnsignedText(&vkRaiseVal) != tinyxml2::XMLError::XML_SUCCESS)
		return false;

	if ((vEle = attrElement.FirstChildElement("KeyLowerValue")) == nullptr)
		return false;
	else if (vEle->QueryUnsignedText(&vkLowerVal) != tinyxml2::XMLError::XML_SUCCESS)
		return false;

	//Finally grab AddToWeaponIfMissing
	bool addIfNotPresent;

	if ((vEle = attrElement.FirstChildElement("AddToWeaponIfMissing")) == nullptr)
		return false;
	else if (vEle->QueryBoolText(&addIfNotPresent) != tinyxml2::XMLError::XML_SUCCESS)
		return false;

	//Ok, we have all the values we need. Create an AttributeInt
	//Not so fast... check min/max

	if (atrMin > atrMax)
	{
		std::stringstream ss("");
		ss << "Attribute " << wpnAttributeName << " had a min value greater than max in Settings.xml.";
		throw InvalidXMLDataException(ss.str());
	}

	//AttributeInt(const std::string& attributeName, bool alwaysAdd, uint32_t raiseKey, uint32_t lowerKey, int minVal, int maxVal, int deltaVal)

	std::shared_ptr<AttributeBase> retRAII = std::make_shared<AttributeInt>(wpnAttributeName, SETTING_VALUE_TYPE::IntValue, addIfNotPresent, vkRaiseVal, vkLowerVal, atrMin, atrMax, atrDelta);

	retRAII->SetIsValid(true);

	return retRAII;
}

std::shared_ptr<AttributeBase> SettingAttributeFactory::ProcessFloatAttributeCase(const tinyxml2::XMLElement & attrElement)
{
	//grab the name="" field, we checked for null already
	std::string wpnAttributeName = attrElement.Attribute("name");

	//find the minvalue, maxvalue, delta children
	float atrMin, atrMax, atrDelta;


	//Reuse this pointer and check each of those three values
	const tinyxml2::XMLElement* vEle = attrElement.FirstChildElement("MinValue");

	if (vEle == nullptr)
		return false;
	else if (vEle->QueryFloatText(&atrMin) != tinyxml2::XMLError::XML_SUCCESS)
		return false;

	if ((vEle = attrElement.FirstChildElement("MaxValue")) == nullptr)
		return false;
	else if (vEle->QueryFloatText(&atrMax) != tinyxml2::XMLError::XML_SUCCESS)
		return false;

	if ((vEle = attrElement.FirstChildElement("Delta")) == nullptr)
		return false;
	else if (vEle->QueryFloatText(&atrDelta) != tinyxml2::XMLError::XML_SUCCESS)
		return false;

	//Now grab keyraise and keylower

	uint32_t vkRaiseVal, vkLowerVal;

	if ((vEle = attrElement.FirstChildElement("KeyRaiseValue")) == nullptr)
		return false;
	else if (vEle->QueryUnsignedText(&vkRaiseVal) != tinyxml2::XMLError::XML_SUCCESS)
		return false;

	if ((vEle = attrElement.FirstChildElement("KeyLowerValue")) == nullptr)
		return false;
	else if (vEle->QueryUnsignedText(&vkLowerVal) != tinyxml2::XMLError::XML_SUCCESS)
		return false;

	//Finally grab AddToWeaponIfMissing
	bool addIfNotPresent;

	if ((vEle = attrElement.FirstChildElement("AddToWeaponIfMissing")) == nullptr)
		return false;
	else if (vEle->QueryBoolText(&addIfNotPresent) != tinyxml2::XMLError::XML_SUCCESS)
		return false;

	//Ok, we have all the values we need. Create an AttributeFloat
	//Not so fast... check min/max. Have to be careful for floating pt tho

	if (atrMin - atrMax > SettingsMgr::floatEpsilon)
	{
		std::stringstream ss("");
		ss << "Attribute " << wpnAttributeName << " had a min value greater than max in Settings.xml.";
		throw InvalidXMLDataException(ss.str());
	}
	else if (atrDelta < 0 || (atrDelta > 0 && atrDelta < SettingsMgr::floatEpsilon))
	{
		std::stringstream ss("");
		ss << "Attribute " << wpnAttributeName << " had a delta value that was either below 0 or greater than 0 but smaller than 0.00000001." << std::endl;
		throw InvalidXMLDataException(ss.str());
	}

	std::shared_ptr<AttributeBase> retRAII = std::make_shared<AttributeFloat>(wpnAttributeName, SETTING_VALUE_TYPE::FloatValue, addIfNotPresent, vkRaiseVal, vkLowerVal, atrMin, atrMax, atrDelta);

	retRAII->SetIsValid(true);

	return retRAII;
}
