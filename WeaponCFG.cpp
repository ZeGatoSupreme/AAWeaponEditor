#pragma once

#include "WeaponCFG.h"
#include "SettingsMgr.h"
#include <string>
#include "ConfigHelpers.h"
#include <regex>

using namespace std;


//returns true if we succesfully read and parsed at least one weapon
ReadWeaponConfigResults WeaponConfig::readWeaponConfigFile(const string& cfgFilePath)
{

	if (cfgFilePath.empty())
		return ReadWeaponConfigResults::ConfigFilePathEmpty;

	//try to read the weapon config file
	ifstream readConfigFileStream(cfgFilePath, std::ios::binary | std::ios::ate);

	//make sure the filestream is ok
	if (readConfigFileStream.fail())
	{
		readConfigFileStream.close();
		return ReadWeaponConfigResults::FailedToReadConfigFile;
	}

	std::regex weaponMatchRegex("\\s*<\\s*[W|w]eapon ");

	//create a filestream to read the settings file
	ifstream readSettingFileStream(cfgFilePath, std::ios::binary | std::ios::ate);

	//make sure the filestream is ok
	if (readSettingFileStream.fail())
	{
		readSettingFileStream.close();
		return ReadWeaponConfigResults::FailedToReadConfigFile;
	}

	//get fiole size
	streamsize fileSize = readSettingFileStream.tellg();

	//reset stream buffer position
	readSettingFileStream.seekg(0, std::ios::beg);

	//store the file data here
	std::vector<char> readTheFile(fileSize);

	//read it into vector backing buffer
	if (!readSettingFileStream.read(readTheFile.data(), fileSize))
	{
		readSettingFileStream.close();
		return ReadWeaponConfigResults::FailedToReadConfigFile;
	}

	//close it...
	readSettingFileStream.close();

	//construct a string from the read data
	string settingsFileData = string(readTheFile.begin(), readTheFile.end());

	//break up the weapons as regex will overflow if input string is too large
	std::vector<string> individualWeaponEntries;

	string newWeaponString;

	//Peel off individual <weapon></weapon> entries from the giant string, and parse each one. Continue while the giant string
	//has more data to read & we can find a weapon tag.
	while (settingsFileData.length())
	{
		//returns index of first character of match in string, or npos if not found
		size_t wpnStartPos = settingsFileData.find("<Weapon ");
		size_t wpnEndPos = settingsFileData.find("</Weapon>");

		//We're done if no match
		if (wpnStartPos == string::npos || wpnEndPos == string::npos)
			break;

		//Create a copy of the found weapon string. Start at the first found < in <weapon and stop at the < in </weapon> plus the length of /weapon>
		newWeaponString = settingsFileData.substr(wpnStartPos, wpnEndPos - wpnStartPos + StringHelpers::CLOSE_WEAPON_LENGTH);

		//store the new string
		individualWeaponEntries.push_back(newWeaponString);

		//erase what we just copied from the giant cfg string
		settingsFileData = settingsFileData.erase(wpnStartPos, wpnEndPos - wpnStartPos + StringHelpers::CLOSE_WEAPON_LENGTH);
	}

	//call our parsing function, passing in the vector of individual weapon config entries
	try
	{
		int nParsed = parseWeaponEntries(individualWeaponEntries);

		if (nParsed <= 0)
			return ReadWeaponConfigResults::ZeroWeaponsParsed;
	}
	catch (AttributeNotUniqueException& ex)
	{
		return ReadWeaponConfigResults::AttributeNotUniqueToFiremode;
	}

	//return the number of weapon configs we parsed
	return ReadWeaponConfigResults::Success;

}


//implementation of parsing weapon entries
//@wpnStrings Contains a vector of individual weapon config entry strings
int WeaponConfig::parseWeaponEntries(const std::vector<string>& wpnStrings)
{

	//make sure we have this at the start of a weapon before going on to more complicated regexes,
	//even though we have already checked this before calling parseWeaponEntries
	std::regex weaponMatchRegex("\\s*<\\s*[W|w]eapon ");

	//current weapon object instance
	WeaponConfigEntry currentWeapon;

	//alert the user this could take a second or two
	std::cout << "Parsing weapon config entries, depending on the size this can take a second, please wait until done..\n";


	//std::regex weaponCfgPattern("^\\s*<\\s*Weapon\\s*[^\\s>]+\\s*>\\s*\\r?\\n?$(.|[\\r\\n\\t])*</\\s*Weapon\\s*>\\s*\\r?\\n?$");

	//Loop through each weapon config string and attempt to parse it.
	//*wpnStrIt = string holding individual weapon config
	for (auto wpnStrIt = wpnStrings.begin(); wpnStrIt != wpnStrings.end(); wpnStrIt++)
	{

		//regex match <Weapon  or <weapon or any variation of those with extra whitespace before
		if (std::regex_search(*wpnStrIt, weaponMatchRegex))
		{

			//try to get the weapon name from the current line
			string weaponName;

			try
			{
				//call helper function to extract weapon config value for attribute "Weapon"
				weaponName = StringHelpers::GetValueFromTaggedAttribute<string>(*wpnStrIt, "Weapon", false);
			}
			catch (exception& ex)
			{
				continue;
			}

			//create a new weapon entry and store it in currentWeapon for further validation
			currentWeapon = WeaponConfigEntry(weaponName);

			//look for firemodes, returns 0 if there is a problem with the weapon config
			int nFiremodes = SplitWeaponEntryIntoLinesAndParse(*wpnStrIt, currentWeapon);

			//If we have found 1 or more firemodes, store the weapon
			if (nFiremodes > 0)
				configWeaponEntries.push_back(currentWeapon);
			else
			{
				cout << "Unable to read a firemode from weapon " + weaponName + ", weapon will be skipped." << std::endl;
			}

			continue;

		}
		else
		{
			continue;
		}

	}

	std::cout << "Done parsing weapon entries. We found " << configWeaponEntries.size() << " valid weapons to edit.\n";

	return configWeaponEntries.size();
}


//Breaks a weapon entry string into its individual lines, and parses each line. Returns the number of parsed firemodes.
//Firemodes are stored in the curWpnObj passed in.
int		WeaponConfig::SplitWeaponEntryIntoLinesAndParse(const std::string& weaponCfgString, WeaponConfigEntry& curWpnObj)
{

	//Break the current weapon up into lines
	string splitTokens = "\r\n";

	//index of whatever we are looking for
	size_t findTokenResult = 0;

	//last found index so we can measure the start and end of lines in our loop
	size_t lastTokenResult = 0;

	//flag for if we are in the middle of a firemode or not
	bool foundFiremode = false;

	//flag for whether we have found an attribute from Settings.xml in the current weapon or not, so we can read its current value
	bool foundAttributeToEdit = false;

	//make sure we have this at the start of a firemode, its a regex that looks for <Firemode with tolerance for various spacings and caps
	std::regex firemodeMatchRegex("^\\s*<\\s*[F|f]ire[M|m]ode\\s*>[\r][\n]$");

	//and this at the end of a firemode
	std::regex firemodeCloseRegex("^\\s*</\\s*[F|f]ire[M|m]ode\\s*>[\r][\n]$");

	//will be captured in group #2 if it matched
	std::smatch regexMatchResults;

	int firemodesFound = 0;

	//Store each individual line from the weapon between <Weapon> and </Weapon>
	std::vector<string> weaponLines;
	
	//temp firemode object
	Firemode curFiremode;

	//move past the first line <Weapon ....
	if ((findTokenResult = weaponCfgString.find_first_of(splitTokens, findTokenResult)) == string::npos)
	{
		cout << "Weapon name " + curWpnObj.getWeaponName() << " only has one line, skipping." << std::endl;

		return 0;
	}
	else
	{
		//bump our found index past current /r/n
		findTokenResult += 2;

		//store it as the 'opening' of the next line
		lastTokenResult = findTokenResult;
	}

	//Split up the remaining lines using a similar process, find the next /r/n, bump the found position past that /r/n, store it as the 'opening'
	//of the line, and find the next /r/n as the end. Repeat while more to read.
	while ((findTokenResult = weaponCfgString.find_first_of(splitTokens, findTokenResult)) != string::npos)
	{
		//bump it past the current /r/n
		findTokenResult += 2;

		//Length of line
		int nStrLen = findTokenResult - lastTokenResult;

		string currentLine = weaponCfgString.substr(lastTokenResult, nStrLen);

		//store the line
		weaponLines.push_back(currentLine);

		//set the end of the last found line as the beginning of the next line
		lastTokenResult = findTokenResult;
	}

	//Need at least <firemode> </firemode>
	if (weaponLines.size() < 2)
	{
		cout << "Weapon name " + curWpnObj.getWeaponName() << " doesnt contain any firemode tags, skipping." << std::endl;

		return 0;
	}

	//THe above will not match the closing </weapon... so even though we have matched it earlier before calling this function, check again for it.
	string wpnCloseTag = weaponCfgString.substr(lastTokenResult);

	if (wpnCloseTag.find("</Weapon>") == string::npos)
	{
		//Missing a closeing /Weapon
		cout << "Weapon name " + curWpnObj.getWeaponName() << " is missing a closing <\/Weapon> tag, skipping." << std::endl;

		return 0;
	}
	
	//Now make sure the first line in our collection is a <Firemode>
	if (!std::regex_search(weaponLines[0], firemodeMatchRegex))
	{
		cout << "Weapon name " + curWpnObj.getWeaponName() << " did not have a firemode tag after the opening weapon tag, skipping." << std::endl;
		return 0;
	}

	//Flag for whether we have counted how many tabs to insert before attribute tags when we output this firemode
	bool countedTabs = false;

	//we can iterate through the lines now
	for (auto strIt = weaponLines.begin(); strIt != weaponLines.end(); strIt++)
	{
		//If we havent found a firemode, make sure we are startign one
		if (!foundFiremode)
		{
			if (!std::regex_match(*strIt, firemodeMatchRegex))
				return 0;
			else
			{
				foundFiremode = true;
			}
		}
		else
		{
			//check if this is firemode closing brace
			if (std::regex_match(*strIt, firemodeCloseRegex))
			{
				foundFiremode = false;

				firemodesFound++;
			}
		}

		foundAttributeToEdit = false;

		//pull tab count from <IsValid as even empty weapons will have this

		if (!countedTabs && strIt->find("</Valid>") != string::npos)
		{
			//count tabs to see how many our attribute tags will need
			int tabCount = 0;

			for (int i = 0; i < strIt->length(); i++)
			{
				if ((*strIt)[i] == '\t')
					tabCount++;
			}

			curFiremode.tabsToInsertBeforeAttribute = tabCount;
			countedTabs = true;
		}

		//see if we can find the current line


		//iterate through the list of attributes we want to edit, check if this line contains the target attribute and current value
		for (auto atrMapIt = SettingsMgr::getAttributesToEditMap().begin(); atrMapIt != SettingsMgr::getAttributesToEditMap().end(); atrMapIt++)
		{
			//check the line for each attribute
			if (strIt->find(atrMapIt->first) == string::npos)
				continue;

			//The firemode should not contain this attribute yet. If it does, the user has specified an attribute to edit that appears multiple times,
			//like something in a <HitData> tag, which we do not support (yet?). Alert the user and reset their Settings.xml
			if (curFiremode.doesAttributeExist(atrMapIt->first))
			{
				cout << "The Attribute to edit " << atrMapIt->first << " was found more than once in a single firemode while parsing weapon name " << curWpnObj.getWeaponName() << std::endl;
				cout << "An Attribute must be unique in a single firemode in order to edit it. Your Settings.xml will be reset and the program will exit, please don't add this Attribute again or you will see this same error message." << std::endl;

				throw AttributeNotUniqueException("Attribute " + atrMapIt->first + " was found more than once in a single Firemode in weapon " + curWpnObj.getWeaponName());
			}


			if (atrMapIt->second == SETTING_VALUE_TYPE::FloatValue)
			{
				float fAtrValue;

				try
				{
					fAtrValue = StringHelpers::GetValueFromTaggedAttribute<float>(*strIt, atrMapIt->first, true);
					foundAttributeToEdit = true;

					//Add to firemode float settings
					curFiremode.firemodeFloatSettings.insert(std::make_pair(atrMapIt->first, fAtrValue));
					break;
				}
				catch (MatchNotFoundException& ex)
				{
					cout << "Unable to extract value for attribute " + atrMapIt->first + " from weapon " + curWpnObj.getWeaponName() << std::endl;
					cout << "Problem with this weapon in your config, it will be skipped." << std::endl;
					return 0;
				}
				catch (exception& ex)
				{
					//bigger problem, TODO
					cout << "Encountered a problem while parsing weapon " + curWpnObj.getWeaponName() << std::endl;
					cout << "Info, if any: " + std::string(ex.what()) << std::endl;
					cout << "Weapon will be skipped." << std::endl;
					return 0;
				}
			}
			else
			{
				int iAtrValue;

				try
				{
					iAtrValue = StringHelpers::GetValueFromTaggedAttribute<int>(*strIt, atrMapIt->first, true);
					foundAttributeToEdit = true;
					//Add to firemode float settings
					curFiremode.firemodeIntSettings.insert(std::make_pair(atrMapIt->first, iAtrValue));
					break;
				}
				catch (MatchNotFoundException& ex)
				{
					cout << "Unable to extract value for attribute " + atrMapIt->first + " from weapon " + curWpnObj.getWeaponName() << std::endl;
					cout << "Problem with this weapon in your config, it will be skipped." << std::endl;
					return 0;
				}
				catch (exception& ex)
				{
					//bigger problem, TODO
					cout << "Encountered a problem while parsing weapon " + curWpnObj.getWeaponName() << std::endl;
					cout << "Info, if any: " + std::string(ex.what()) << std::endl;
					cout << "Weapon will be skipped." << std::endl;
					return 0;
				}
			}
		}

		//Went through attributes we want to edit, got existing values if present. If no attribute, store it as line we wont generate ourselves.
		if (foundAttributeToEdit)
		{
			continue;
		}
		else
		{
			//what the shit windows, either treat /r/n as one line at all times or don't
			//notepad++ showed two spaces between every line in wpn config after output, notepad showed one. So strip off the \r\n and let stl decide what
			//to put there via std::endl when outputting
			std::string saveStr = strIt->substr(0, strIt->find(splitTokens));

			curFiremode.addNonAttributeLine(saveStr);
		}

		if (!foundFiremode)
		{
			//We just finished one fire mode, add it to the weapon and reset the temp firemode object
			curWpnObj.AddFiremode(curFiremode);

			firemodesFound++;

			curFiremode = Firemode();
			
			countedTabs = false;
		}

		continue;

	}

	//if we end in the middle of a firemode, its an invalid config
	if (foundFiremode)
		return 0;

	return firemodesFound;

}
