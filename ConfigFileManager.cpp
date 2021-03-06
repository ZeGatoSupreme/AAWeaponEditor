#pragma once

#include "ConfigFileManager.h"


ConfigFileManager::ConfigFileManager()
	:
	appDirectoryPath(""),
	keybindFilePath(""),
	weaponFilePath(""),
	keyValueNamesFilePath(""),
	newWeaponFilePath("")
{

}



bool ConfigFileManager::writeOutWeaponConfigFile(const WeaponConfig & rWpnConfig)
{
	if (newWeaponFilePath.empty())
		return false;

	//write it back out to the wpn cfg file path, open as binary so it doesnt get cute trying to handle linebreaks for us. This will only ever be run on windows anyways.
	ofstream wpnConfigWriter(newWeaponFilePath, std::ios::binary| std::ofstream::out | std::ofstream::trunc);

	int retryCount = 2;

	while ((wpnConfigWriter.fail() || wpnConfigWriter.bad()) && --retryCount >= 0)
	{
		std::cout << "Couldn't open the output config file for writing, sleeping briefly then trying again." << std::endl;

		wpnConfigWriter.close();

		//Try one more time after sleeping for a brief period (10 ms)
		Sleep(10);

		wpnConfigWriter = ofstream(newWeaponFilePath, std::ofstream::out | std::ofstream::trunc);
	}

	if (retryCount < 0 && (wpnConfigWriter.fail() || wpnConfigWriter.bad()))
		return false;

	std::vector<WeaponConfigEntry>::const_iterator wpnIt = rWpnConfig.configWeaponEntries.begin();

	//go up through index-1 so we dont add an extra \r\n at end of newweaps, if they copy back to original weapons file and then edit again,
	//it will add extra blanks at bottom.
	while (wpnIt != rWpnConfig.configWeaponEntries.end() -1)
	{
		wpnConfigWriter << *wpnIt << "\r\n";
		wpnIt++;
	}

	//Output last (or only, if just 1 weapon) without \r\n
	wpnConfigWriter << *wpnIt;


	wpnConfigWriter.close();

	return true;
}

//back up newweaps in case there are changes in it that user wanted to save but forgot to copy out
bool ConfigFileManager::backupExistingNewWeaps()
{
	if (newWeaponFilePath.empty())
		return false;

	ifstream oldNewWeapsIn(newWeaponFilePath, std::ios::binary | std::ios::ate);

	if (oldNewWeapsIn.fail() || oldNewWeapsIn.bad())
	{
		oldNewWeapsIn.close();
		return false;
	}

	std::string bakNewWeaponsFilePath = newWeaponFilePath;

	bakNewWeaponsFilePath.append(".bak");

	// write it back out to the keybind cfg file
	ofstream wpnConfigWriter(bakNewWeaponsFilePath, std::ofstream::out | std::ofstream::trunc | std::ios::binary);

	if (wpnConfigWriter.fail() || wpnConfigWriter.bad())
	{
		oldNewWeapsIn.close();
		wpnConfigWriter.close();
		return false;
	}

	streamsize oldSize = oldNewWeapsIn.tellg();

	//reset stream buffer position
	oldNewWeapsIn.seekg(0, std::ios::beg);

	std::vector<char> backupVec(oldSize);

	if (!oldNewWeapsIn.read(backupVec.data(), oldSize))
	{
		oldNewWeapsIn.close();
		return false;
	}

	oldNewWeapsIn.close();

	if (!wpnConfigWriter.write(backupVec.data(), oldSize))
	{
		wpnConfigWriter.close();
		return false;
	}
	
	wpnConfigWriter.close();

	return true;

}

void ConfigFileManager::initialize()
{
	//Used to store current directory
	char curPathBuffer[MAX_PATH];

	DWORD curPathBufferLen = 0;

	SetLastError(0);

	//returns length of max path
	curPathBufferLen = GetCurrentDirectory(0, curPathBuffer);

	GetCurrentDirectory(curPathBufferLen, curPathBuffer);

	if (GetLastError())
	{
		std::cout << "Problem getting current directory. Run as admin next time and move to normal or shorter directory.\n";

		exit(-1);
	}

	string curDirString(curPathBuffer);

	if (curDirString.empty())
	{
		std::cout << "Problem getting current directory. Run as admin next time and move to normal or shorter directory.\n";

		exit(-1);
	}
	else
		appDirectoryPath = curDirString;

	//Look for the config files we need
	if (!findAndStoreKeybindFilePath())
	{
		cerr << "Could not find keybind config file, let AA loader redownload config files, make sure app is in that dir, try again." << endl;
		exit(-1);
	}

	if (!findAndStoreKeyValueNamesPath())
	{
		cerr << "Could not find the config file containing key name values, let AA loader redownload config files, make sure app is in that dir, try again." << endl;
		exit(-1);
	}

	if (!findAndStoreWeaponFilePath())
	{
		cerr << "Could not find the weapon config file, let AA loader redownload config files, make sure app is in that dir, try again." << endl;
		exit(-1);
	}

}

bool ConfigFileManager::findAndStoreWeaponFilePath()
{
	if (appDirectoryPath.empty())
		return false;

	string searchCurrentDirPath = appDirectoryPath;

	searchCurrentDirPath.push_back('\\');

	searchCurrentDirPath.push_back('*');

	bool foundWeaponFile = false;

	//boilerplate msdn file enumeration code

	HANDLE hSearchHandle;

	WIN32_FIND_DATA tempResult;

	LPWIN32_FIND_DATA sResult = &tempResult;

	string curFileName = "";

	string curFileFullPath = "";

	string curFileLine = "";

	ifstream fOpener;

	SetLastError(0);

	hSearchHandle = FindFirstFile(searchCurrentDirPath.c_str(), sResult);

	if (hSearchHandle == INVALID_HANDLE_VALUE || GetLastError() == ERROR_FILE_NOT_FOUND)
	{
		cerr << "Error: Run this program as admin in the game's config directory" << endl;
		exit(-1);
	}
	else
	{
		do
		{

			curFileName = sResult->cFileName;

			//AA config files are randomly named files of length 32
			if (curFileName.size() != 32)
				continue;

			curFileFullPath = appDirectoryPath;

			curFileFullPath.push_back('\\');

			curFileFullPath.append(curFileName);

			fOpener.open(curFileFullPath, ios_base::in);

			if (fOpener.fail())
			{
				cerr << "Couldn't open a file in dir" << endl;
				fOpener.close();
				exit(-1);
			}

			//check for empty file
			if (!fOpener.eof())
				getline(fOpener, curFileLine);
			else
			{
				fOpener.close();
				continue;
			}

			//Skip any blank lines at start of file
			while (!fOpener.eof() && (curFileLine == ""))
				getline(fOpener, curFileLine);

			//look for <Weapon 

			while (!fOpener.eof() && (curFileLine.find("<Weapon ") == string::npos))
				getline(fOpener, curFileLine);

			//If we hit the end of file, go to next in dir
			if (fOpener.eof())
			{
				fOpener.close();
				continue;
			}
			else
			{
				//check for <FireMode at the next line to verify
				getline(fOpener, curFileLine);
				if (curFileLine.find("<FireMode>") == string::npos || fOpener.eof())
				{
					fOpener.close();
					continue;
				}
					

				//ok, this is it
				weaponFilePath = curFileFullPath;
				foundWeaponFile = true;
				fOpener.close();

				newWeaponFilePath = appDirectoryPath;

				newWeaponFilePath.push_back('\\');

				newWeaponFilePath.append(NEW_WEAPON_NAME);
			}

		} while (!foundWeaponFile && FindNextFile(hSearchHandle, sResult));

		FindClose(hSearchHandle);

		if (!foundWeaponFile)
		{
			cerr << "Could not find the file containing <Weapon Weapon>, let AA loader redownload config files, make sure app is in that dir, try again." << endl;
			exit(-1);
		}

		return true;

	}
}

bool ConfigFileManager::findAndStoreKeybindFilePath()
{
	if (appDirectoryPath.empty())
		return false;

	string searchCurrentDirPath = appDirectoryPath;

	searchCurrentDirPath.push_back('\\');

	searchCurrentDirPath.push_back('*');

	bool foundKeybindFile = false;

	//boilerplate msdn file enumeration code

	HANDLE hSearchHandle;

	WIN32_FIND_DATA tempResult;

	LPWIN32_FIND_DATA sResult = &tempResult;

	string curFileName = "";

	string curFileFullPath = "";

	string curFileLine = "";

	ifstream fOpener;

	SetLastError(0);

	hSearchHandle = FindFirstFile(searchCurrentDirPath.c_str(), sResult);

	if (hSearchHandle == INVALID_HANDLE_VALUE || GetLastError() == ERROR_FILE_NOT_FOUND)
	{
		cerr << "Error: Run this program as admin in the game's config directory" << endl;
		exit(-1);
	}
	else
	{
		do
		{

			curFileName = sResult->cFileName;

			//AA config files are randomly named files of length 32
			if (curFileName.size() != 32)
				continue;

			curFileFullPath = appDirectoryPath;

			curFileFullPath.push_back('\\');

			curFileFullPath.append(curFileName);

			fOpener.open(curFileFullPath, ios_base::in);

			if (fOpener.fail())
			{
				cerr << "Couldn't open a file in dir" << endl;
				fOpener.close();
				exit(-1);
			}

			//check for empty file
			if (!fOpener.eof())
				getline(fOpener, curFileLine);
			else
			{
				fOpener.close();
				continue;
			}

			//Skip any blank lines at start of file
			while (!fOpener.eof() && (curFileLine == ""))
				getline(fOpener, curFileLine);

			//look for [KeyBinds]
			//			Unknown0x00

			while (!fOpener.eof() && (curFileLine.find("[KeyBinds]") == string::npos))
				getline(fOpener, curFileLine);

			//If we hit the end of file, go to next in dir
			if (fOpener.eof())
			{
				fOpener.close();
				continue;
			}
			else
			{
				//Check one more line to make sure
				getline(fOpener, curFileLine);
				if (curFileLine.empty() || curFileLine.find("Unknown0x00") == string::npos)
				{
					fOpener.close();
					continue;
				}

				//ok, this is it
				keybindFilePath = curFileFullPath;
				foundKeybindFile = true;
				fOpener.close();
			}

		} while (!foundKeybindFile && FindNextFile(hSearchHandle, sResult));

		FindClose(hSearchHandle);

		if (!foundKeybindFile)
		{
			cerr << "Could not find the file containing [KeyBinds], let AA loader redownload config files, make sure app is in that dir, try again." << endl;
			exit(-1);
		}

		return true;

	}
}

bool ConfigFileManager::findAndStoreKeyValueNamesPath()
{
	if (appDirectoryPath.empty())
		return false;

	string searchCurrentDirPath = appDirectoryPath;

	searchCurrentDirPath.push_back('\\');

	searchCurrentDirPath.push_back('*');

	bool foundKeyValueNameFile = false;

	//boilerplate msdn file enumeration code

	HANDLE hSearchHandle;

	WIN32_FIND_DATA tempResult;

	LPWIN32_FIND_DATA sResult = &tempResult;

	string curFileName = "";

	string curFileFullPath = "";

	string curFileLine = "";

	ifstream fOpener;

	SetLastError(0);

	hSearchHandle = FindFirstFile(searchCurrentDirPath.c_str(), sResult);

	if (hSearchHandle == INVALID_HANDLE_VALUE || GetLastError() == ERROR_FILE_NOT_FOUND)
	{
		cerr << "Error: Run this program as admin in the game's config directory" << endl;
		exit(-1);
	}
	else
	{
		do
		{

			curFileName = sResult->cFileName;

			//AA config files are randomly named files of length 32
			if (curFileName.size() != 32)
				continue;

			curFileFullPath = appDirectoryPath;

			curFileFullPath.push_back('\\');

			curFileFullPath.append(curFileName);

			fOpener.open(curFileFullPath, ios_base::in);

			if (fOpener.fail())
			{
				cerr << "Couldn't open a file in dir" << endl;
				fOpener.close();
				exit(-1);
			}

			//check for empty file
			if (!fOpener.eof())
				getline(fOpener, curFileLine);
			else
			{
				fOpener.close();
				continue;
			}

			//Skip any blank lines at start of file
			while (!fOpener.eof() && (curFileLine == ""))
				getline(fOpener, curFileLine);

			//look for [KeyNames]
			//			0 = Unknown0x00

			while (!fOpener.eof() && (curFileLine.find("[KeyNames]") == string::npos))
				getline(fOpener, curFileLine);

			//If we hit the end of file, go to next in dir
			if (fOpener.eof())
			{
				fOpener.close();
				continue;
			}
			else
			{
				//Check one more line to make sure
				getline(fOpener, curFileLine);
				if (curFileLine.empty() || curFileLine.find("Unknown0x00") == string::npos)
				{
					fOpener.close();
					continue;
				}

				//ok, this is it
				keyValueNamesFilePath = curFileFullPath;
				foundKeyValueNameFile = true;
				fOpener.close();
			}

		} while (!foundKeyValueNameFile && FindNextFile(hSearchHandle, sResult));

		FindClose(hSearchHandle);

		if (!foundKeyValueNameFile)
		{
			cerr << "Could not find the file containing [KeyNames], let AA loader redownload config files, make sure app is in that dir, try again." << endl;
			exit(-1);
		}

		return true;

	}

}

bool ConfigFileManager::addNewKeyBind(const std::string& newKeybindLine)
{
	//text to add to the keybind cfg line
	string fullLine = newKeybindLine;
	fullLine.append("=OnPress:loadweapons newweaps");

	//text to search for in keybind cfg file
	string findKeybindLine = newKeybindLine;
	findKeybindLine.append("=");

	std::vector<string> keybindFileData;

	ifstream kbdReader;

	kbdReader.open(keybindFilePath, ios_base::in);

	if (kbdReader.fail())
	{
		cerr << "problem opening keybind cfg" << endl;

		exit(-1);
	}

	string currentLine;

	bool foundLine = false;

	//Copy all the lines from the keybinds file into a vector of strings, modify the keybind line we want, then read them all out into the keybind file
	while (!kbdReader.eof() && getline(kbdReader, currentLine))
	{

		//found the line we are looking for
		if (!foundLine && (currentLine.find(findKeybindLine) != string::npos))
		{

			//store the new modified keybind line instead
			currentLine = fullLine;
			foundLine = true;
		}

		keybindFileData.push_back(currentLine + '\n');

	}

	kbdReader.close();

	//write it back out to the keybind cfg file
	ofstream kbdWriter(keybindFilePath, std::ofstream::out | std::ofstream::trunc);

	if (kbdWriter.fail())
	{
		cerr << "Error writing new keybind file" << endl;
		exit (-1);
	}

	vector<string>::iterator strIt = keybindFileData.begin();

	while (strIt != keybindFileData.end())
	{
		kbdWriter << *strIt;
		strIt++;
	}

	kbdWriter.close();

	return true;
}