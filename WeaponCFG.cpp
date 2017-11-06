#pragma once

#include "WeaponCFG.h"
#include "SettingsMgr.h"
#include <string>
#include "ConfigHelpers.h"
#include <regex>
#include "LoggerMgr.h"

using namespace std;


WeaponConfig::WeaponConfig()
	: configWeaponEntries()
{
}

//returns true if we succesfully read and parsed at least one weapon
ReadWeaponConfigResults WeaponConfig::readWeaponConfigFile(const string& cfgFilePath)
{

	if (cfgFilePath.empty())
		return ReadWeaponConfigResults::ConfigFilePathEmpty;

	std::regex weaponMatchRegex("\\s*<\\s*[W|w]eapon ");

	//create a filestream to read the settings file, start at end to get size, read as binary to avoid formatting bs
	ifstream readSettingFileStream(cfgFilePath, std::ios::ate | std::ios::binary);

	//make sure the filestream is ok
	if (readSettingFileStream.fail())
	{
		readSettingFileStream.close();
		return ReadWeaponConfigResults::FailedToReadConfigFile;
	}

	//size
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

	LogHelper& logInstance = LogHelper::getInstance();

	//Loop through each weapon config string and attempt to parse it.
	//*wpnStrIt = string holding individual weapon config
	for (auto wpnStrIt = wpnStrings.begin(); wpnStrIt != wpnStrings.end(); wpnStrIt++)
	{

		if (logInstance.getLoggingState())
		{
			logInstance << "Dumping next weapon string." << "\r\n\r\n";
			logInstance << *wpnStrIt;
		}

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
				std::cout << "Unable to read a firemode from weapon " + weaponName + ", weapon will be skipped." << std::endl;
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

	//Break the current weapon up into lines, windows uses \r\n for line separators, that pain in the ass bill gates
	string splitTokens = "\r\n";

	//index of whatever we are looking for
	size_t findTokenResult = 0;

	//last found index so we can measure the start and end of lines in our loop
	size_t lastTokenResult = 0;

	//flag for if we are in the middle of a firemode or not
	bool insideFiremode = false;

	//flag for whether we have found an attribute from Settings.xml in the current weapon or not, so we can read its current value
	bool foundAttributeToEdit = false;

	//make sure we have this at the start of a firemode, its a regex that looks for <Firemode with tolerance for various spacings and caps
	std::regex firemodeMatchRegex("^\\s*<\\s*[F|f]ire[M|m]ode\\s*>[\r][\n]$");

	//will only match if line has something other than spacing chars
	std::regex emptyLineInsideFiremodeCheck("[^\\s]+");

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
		std::cout << "Weapon name " + curWpnObj.getWeaponName() << " only has one line, skipping." << std::endl;

		return 0;
	}
	else
	{
		//bump our found index past current /r/n
		findTokenResult += 2;

		//store it as the 'opening' of the next line
		lastTokenResult = findTokenResult;
	}

	LogHelper& logInstance = LogHelper::getInstance();

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
		std::cout << "Weapon name " + curWpnObj.getWeaponName() << " doesnt contain any firemode tags, skipping." << std::endl;

		return 0;
	}

	//Check for closing tag
	std::string wpnCloseTag = weaponLines[weaponLines.size() - 1];

	bool isLastWeaponInList = false;

	if (wpnCloseTag.find("</Weapon>") == string::npos)
	{

		//it is possible this is the last weapon in the list, and it won't have a \r\n following it.
		std::string checkLastWeaponStr = weaponCfgString.substr(lastTokenResult);
		if (checkLastWeaponStr.find("</Weapon>") == string::npos)
		{

			//Missing a closeing /Weapon
			std::cout << "Weapon name " + curWpnObj.getWeaponName() << " is missing a closing <\/Weapon> tag, skipping." << std::endl;

			return 0;
		}
		else
			isLastWeaponInList = true;
	}

	//If this is the last weapon in list, we will not have stored the closing weapon tag in the for loop above.
	if (!isLastWeaponInList)
	{
		//remove the last /weapon line, we will generate that ourselves.
		weaponLines.erase(weaponLines.begin() + weaponLines.size() - 1);
	}

	//Now make sure the first line in our collection is a <Firemode>
	if (!std::regex_search(weaponLines[0], firemodeMatchRegex))
	{
		std::cout << "Weapon name " + curWpnObj.getWeaponName() << " did not have a firemode tag after the opening weapon tag, skipping." << std::endl;
		return 0;
	}

	//Flag for whether we have counted how many tabs to insert before attribute tags when we output this firemode
	bool countedTabs = false;

	if (logInstance.getLoggingState())
	{
		logInstance << "Dumping weapon entry lines before firemode parsing: " << curWpnObj.getWeaponName() << "\r\n\r\n";
		for (auto strIt = weaponLines.begin(); strIt != weaponLines.end(); strIt++)
			logInstance << *strIt;
		logInstance << "\r\nEnd weapon entry lines for " << curWpnObj.getWeaponName() << "\r\n";
	}

	//we can iterate through the lines now
	for (auto strIt = weaponLines.begin(); strIt != weaponLines.end(); strIt++)
	{
		//If we havent found a firemode, make sure we are startign one
		if (!insideFiremode)
		{
			if (!std::regex_match(*strIt, firemodeMatchRegex))
				return 0;
			else
			{
				insideFiremode = true;
			}
		}
		else
		{
			//check if this is firemode closing brace
			if (std::regex_match(*strIt, firemodeCloseRegex))
			{
				insideFiremode = false;

				firemodesFound++;
			}
		}

		foundAttributeToEdit = false;

		//pull tab count from <IsValid as even empty weapons will have this
		size_t isValidIndex;

		if (!countedTabs && (isValidIndex = strIt->find("<Valid")) != string::npos)
		{
			//count tabs to see how many our attribute tags will need
			int tabCount = 0;

			//edit: H8society sent me a weapon config that had a bunch of spaces in front of tags instead of tab chars.
			//so check for both. The file he sent me had 4 spaces to one tab char. But tab chars are supposed to indent by 'one level'. Screw it.
			if ((*strIt)[0] == ' ')
			{
				for (int i = 0; i < isValidIndex; i++)
				{
					if ((*strIt)[i] == ' ')
						tabCount++;
					else
						break;
				}

				//See if we can still use tabs, if spaces is cleanly divisible by 4.
				if (tabCount % 4 == 0)
					tabCount /= 4;
				else
					curFiremode.useTabsForBreaks = false;

			}
			else if ((*strIt)[0] == '\t')
			{
				for (int i = 0; i < strIt->length(); i++)
				{
					if ((*strIt)[i] == '\t')
						tabCount++;
					else
						break;
				}
			}
			else
			{
				std::cout << "\n no spacing detected in front of <Valid> tag, not going to put any spaces in front of attribute tags that are edited." << std::endl;
			}

			curFiremode.breaksToInsertBeforeAttribute = tabCount;
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
				std::cout << "The Attribute to edit " << atrMapIt->first << " was found more than once in a single firemode while parsing weapon name " << curWpnObj.getWeaponName() << std::endl;
				std::cout << "An Attribute must be unique in a single firemode in order to edit it. Your Settings.xml will be reset and the program will exit, please don't add this Attribute again or you will see this same error message." << std::endl;

				throw AttributeNotUniqueException("Attribute " + atrMapIt->first + " was found more than once in a single Firemode in weapon " + curWpnObj.getWeaponName());
			}


			if (atrMapIt->second == SETTING_VALUE_TYPE::FloatValue)
			{
				float fAtrValue;

				try
				{
					fAtrValue = StringHelpers::GetValueFromTaggedAttribute<float>(*strIt, atrMapIt->first, true);
					foundAttributeToEdit = true;

					if (logInstance.getLoggingState())
					{
						logInstance << "Found attribute in weapon " << curWpnObj.getWeaponName() << " which matches a target attribute." << "\r\n";
						logInstance << "Input line: " << *strIt << "\r\n";
						logInstance << "Attribute name: " << atrMapIt->first << ", attribute value: " << fAtrValue;
					}

					//Add to firemode float settings
					curFiremode.firemodeFloatSettings.insert(std::make_pair(atrMapIt->first, fAtrValue));
					break;
				}
				catch (MatchNotFoundException& ex)
				{
					std::cout << "Unable to extract value for attribute " + atrMapIt->first + " from weapon " + curWpnObj.getWeaponName() << std::endl;
					std::cout << "Problem with this weapon in your config, it will be skipped." << std::endl;
					return 0;
				}
				catch (exception& ex)
				{
					//bigger problem, TODO
					std::cout << "Encountered a problem while parsing weapon " + curWpnObj.getWeaponName() << std::endl;
					std::cout << "Info, if any: " + std::string(ex.what()) << std::endl;
					std::cout << "Weapon will be skipped." << std::endl;
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

					if (logInstance.getLoggingState())
					{
						logInstance << "Found attribute in weapon " << curWpnObj.getWeaponName() << " which matches a target attribute." << "\r\n";
						logInstance << "Input line: " << *strIt << "\r\n";
						logInstance << "Attribute name: " << atrMapIt->first << ", attribute value: " << iAtrValue;
					}

					curFiremode.firemodeIntSettings.insert(std::make_pair(atrMapIt->first, iAtrValue));
					break;
				}
				catch (MatchNotFoundException& ex)
				{
					std::cout << "Unable to extract value for attribute " + atrMapIt->first + " from weapon " + curWpnObj.getWeaponName() << std::endl;
					std::cout << "Problem with this weapon in your config, it will be skipped." << std::endl;
					return 0;
				}
				catch (exception& ex)
				{
					//bigger problem, TODO
					std::cout << "Encountered a problem while parsing weapon " + curWpnObj.getWeaponName() << std::endl;
					std::cout << "Info, if any: " + std::string(ex.what()) << std::endl;
					std::cout << "Weapon will be skipped." << std::endl;
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
			if (std::regex_search(*strIt, emptyLineInsideFiremodeCheck))
			{
				curFiremode.addNonAttributeLine(*strIt);
				if (logInstance.getLoggingState())
				{
					logInstance << "\r\nStored non-attribute line in non-attribute line data for firemode.\r\n";
					logInstance << "\r\nLine: start|" + *strIt << "|end\r\n";
				}
			}
			else if (logInstance.getLoggingState())
			{
				logInstance << "Skipping storing empty firemode line for weapon: " << curWpnObj.getWeaponName() << "\r\n";
				logInstance << "Empty line: " << *strIt << "|end" << "\r\n";
			}

		}

		if (!insideFiremode)
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
	if (insideFiremode)
		return 0;

	return firemodesFound;

}
