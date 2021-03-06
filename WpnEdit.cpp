#pragma once

#include <windows.h>
#include "WeaponCFG.h"
#include "WeaponEntry.h"
#include "ConfigFileManager.h"
#include "SettingsMgr.h"
#include <vector>
#include <stdio.h>
#include <TlHelp32.h>
#include <psapi.h>
#include <map>
#include <tchar.h> //fucking ms

using std::exit;
using namespace std;

HHOOK hKeyboardHook;
HWND activeWindow;

std::map<uint32_t, const std::shared_ptr<AttributeBase>> keysToAttributeMap;

//Global class instances, ConfigFileManager does most of its work in its constructor. Will output to cmd prompt window if any problems occured.
ConfigFileManager newSetup;
WeaponConfig myWeap;

SettingsMgr settingMgmt;

//Current weapon, firemode
WeaponConfigEntry curWpn;
int curFiremodeIndex = 0;
int curWeaponIndex = 0;
Firemode curFm;

//next and prev weapon keys
uint32_t nextWeaponKey, prevWeaponKey, AAKeyCode;

//TODO: Replace all std::couts with multiple << oeprators with printfs

//What can I say, its late and my motivation is dying, just making some separate functions for handling wpn/firemode transitions instead of wrapping in class
#pragma region lazychangeweaponorfiremodehandlers

//per h8's request, have it skip back to to first firemode when going back 1 weapon
void handleLowerWeaponOrFiremode()
{
	if (curWeaponIndex == 0)
	{
		//We are on the first weapon, check to see if we are on t he first firemode or not.

		const WeaponConfigEntry& curWeapon = myWeap.getWeaponConfigAtIndex(curWeaponIndex);

		//see if we are on the first firemode for current weapon
		if (curFiremodeIndex == 0)
		{
			//Wrap around to first firemode of last weapon

			curWeaponIndex = myWeap.getWeaponConfigEntryCount() - 1;

			const WeaponConfigEntry& nCurWeapon = myWeap.getWeaponConfigAtIndex(curWeaponIndex);

			curFiremodeIndex = 0;

			std::cout << "Weapon: " << nCurWeapon.getWeaponName().c_str() << ", FireMode: " << curFiremodeIndex << std::endl;
		}
		else
		{
			//Decrement firemode index
			std::cout << "Weapon: " << curWeapon.getWeaponName().c_str() << ", FireMode: " << --curFiremodeIndex << std::endl;
		}
	}
	else
	{
		//Else we can lower weapon or firemode

		//If we are on the first firemode, switch to prev weapon and set firemode index to prev weapons highest firemode index
		if (curFiremodeIndex == 0)
		{
			//output current weapon and firemode index after decrimenting curWeaponIndex
			const WeaponConfigEntry& curWeapon = myWeap.getWeaponConfigAtIndex(--curWeaponIndex);

			//If we are going backwards, should start at highest firemode index
			//Naw just start at fm zero to have a faster way of navigating
			curFiremodeIndex = 0;

			std::cout << "Weapon: " << curWeapon.getWeaponName().c_str() << ", FireMode: " << curFiremodeIndex << std::endl;
		}
		else
		{
			//Decriment firemode index and output weapon name and firemodeindex
			//output current weapon and firemode index
			const WeaponConfigEntry& curWeapon = myWeap.getWeaponConfigAtIndex(curWeaponIndex);

			std::cout << "Weapon: " << curWeapon.getWeaponName().c_str() << ", FireMode: " << --curFiremodeIndex << std::endl;
		}
	}
}

void handleRaiseWeaponOrFiremode()
{

	const WeaponConfigEntry& curWeapon = myWeap.getWeaponConfigAtIndex(curWeaponIndex);

	int firemodeCount = curWeapon.getFiremodeCount();

	//Check to see if we are on the last weapon
	if (myWeap.getWeaponConfigEntryCount() - 1 == curWeaponIndex) 
	{

		//see if we are on the last firemode for current weapon
		if (firemodeCount - 1 == curFiremodeIndex)
		{

			//Wrap around to first weapon and first firemode
			curWeaponIndex = curFiremodeIndex = 0;

			//get first weapon
			const WeaponConfigEntry& nCurWeapon = myWeap.getWeaponConfigAtIndex(curWeaponIndex);

			//output current weapon and firemode.
			std::cout << "Weapon: " << nCurWeapon.getWeaponName().c_str() << ", FireMode: " << curFiremodeIndex << std::endl;
		}
		else
		{
			//Increment firemode index
			std::cout << "Weapon: " << curWeapon.getWeaponName().c_str() << ", FireMode: " << ++curFiremodeIndex << std::endl;
		}
	}
	else
	{	//Else we can increment the current weapon or firemode


		//If we are on the last firemode, switch to next weapon and reset firemode index to 0
		if (firemodeCount - 1 == curFiremodeIndex)
		{
			curFiremodeIndex = 0;

			//output current weapon and firemode index after incrementing curWeaponIndex
			const WeaponConfigEntry& nCurWeapon = myWeap.getWeaponConfigAtIndex(++curWeaponIndex);

			std::cout << "Weapon: " << nCurWeapon.getWeaponName().c_str() << ", FireMode: " << curFiremodeIndex << std::endl;
		}
		else
		{
			//Increment firemode index and output weapon name and firemodeindex
			//output current weapon and firemode index

			std::cout << "Weapon: " << curWeapon.getWeaponName().c_str() << ", FireMode: " << ++curFiremodeIndex << std::endl;
		}
	}
}
#pragma endregion

//Todo: Do some profiling and see if it makes any real difference using pointers to track current weapon, or if passing back a const reference is fine
 LRESULT CALLBACK processKeyDown (int nCode, WPARAM wParam, LPARAM lParam)
{
    DWORD SHIFT_key=0;
    DWORD CTRL_key=0;
    DWORD ALT_key=0;

	std::map<uint32_t, const std::shared_ptr<AttributeBase>>::iterator keysToAtrIt;

    if  ((nCode == HC_ACTION) &&   ((wParam == WM_SYSKEYDOWN) ||  (wParam == WM_KEYDOWN)))      
    {
        KBDLLHOOKSTRUCT hooked_key =    *((KBDLLHOOKSTRUCT*)lParam);
        DWORD dwMsg = 1;
        dwMsg += hooked_key.scanCode << 16;
        dwMsg += hooked_key.flags << 24;
        char lpszKeyName[1024] = {0};
        lpszKeyName[0] = '[';

        int i = GetKeyNameText(dwMsg,   (lpszKeyName+1),0xFF) + 1;
        lpszKeyName[i] = ']';

        uint32_t key = hooked_key.vkCode;

        SHIFT_key = GetAsyncKeyState(VK_SHIFT);
        CTRL_key = GetAsyncKeyState(VK_CONTROL);
        ALT_key = GetAsyncKeyState(VK_MENU);

		if (key == nextWeaponKey)
		{
			//Todo: move all this to a separate function, or wrap current index and such in a class
			handleRaiseWeaponOrFiremode();
			
		}
		else if (key == prevWeaponKey)
		{
			handleLowerWeaponOrFiremode();
		}
		else if ((keysToAtrIt = keysToAttributeMap.find(key)) != keysToAttributeMap.end()) //Check to see if they pressed a key specified in Settings.xml
		{
			/*curWpn.setFiremodeAttributeValue()*/
			WeaponConfigEntry& curWeapon = myWeap.getWeaponConfigAtIndex(curWeaponIndex);
			
			FiremodeChangeResult changeRes = curWeapon.setFiremodeAttributeValue(curFiremodeIndex, keysToAtrIt->second, key);

			//returns 0 if nothing changed (ie already at min or max and they tried to go beyond limit)
			if (changeRes == FiremodeChangeResult::AttributeChanged)
			{
				//write out the new weapon config file, fire the AA loadweapons keybind
				if (!newSetup.writeOutWeaponConfigFile(myWeap))
				{
					std::cout << "Failed to write out new weapon config file." << std::endl;

					//Get focus in case they have a fullscreen game and only 1 monitor
					GetFocus();

					//Annoy them with a message box
					MessageBox(NULL, "Failed to write out weapon config file, run as admin, make sure you dont have newweaps open, etc etc", "Annoying box", MB_OK);

					PostQuitMessage(-1);
				}

				//Send AA loadweapons newweaps keypress
				keybd_event((unsigned char)AAKeyCode, 0, 0, 0);

				keybd_event((unsigned char)AAKeyCode, 0, KEYEVENTF_KEYUP, 0);

			}
			else if (changeRes == FiremodeChangeResult::AttributeDoesntExistOnWeaponAndNotForcedToAdd)
			{
				//We already output that they tried to change an attribute that does not exist on weapon, and was not set to always be added in settings.xml
				//Don't bother outputting current weapon attribute val as it does not exist.
				return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
			}


			//If we get here they have either changed an attribute and wrote out new wpns or an attribute was at the max/min and they tried to exceed that value.
			//Either way output current weapon attribute value of whatever they changed, or tried to change.
			if (keysToAtrIt->second->AttributeType() == SETTING_VALUE_TYPE::FloatValue)
			{
				std::string attributeName = keysToAtrIt->second->AttributeName();
				//write out new firemode stats
				std::cout << "Weapon " << curWeapon.getWeaponName() << ", Firemode: " << curFiremodeIndex << ", " << attributeName << ": " << std::fixed << curWeapon.getFiremodeByIndex(curFiremodeIndex).getFiremodeAttributeValue<float>(attributeName) << std::endl;
			}
			else if (keysToAtrIt->second->AttributeType() == SETTING_VALUE_TYPE::IntValue)
			{
				std::string attributeName = keysToAtrIt->second->AttributeName();
				//write out new firemode stats
				std::cout << "Weapon " << curWeapon.getWeaponName() << ", Firemode: " << curFiremodeIndex << ", " << attributeName << ": " << curWeapon.getFiremodeByIndex(curFiremodeIndex).getFiremodeAttributeValue<int>(attributeName) << std::endl;
			}

		}


        if (CTRL_key !=0 && key == 'Q' )   
        {
              MessageBox(NULL, "Exiting", "AA In Game Weapon Editor", MB_OK); 
              PostQuitMessage(0);
        }
		SHIFT_key = 0;
		CTRL_key = 0;
		ALT_key = 0;

    }
    return CallNextHookEx(hKeyboardHook,    nCode,wParam,lParam);
}

void MessageLoop()
{
    MSG message;
    while (GetMessage(&message,NULL,0,0)) 
    {
        TranslateMessage( &message );
        DispatchMessage( &message );
    }
}

DWORD WINAPI setKeyBoardHook(LPVOID lpParm)
{
    HINSTANCE hInstance = GetModuleHandle(NULL);
    if (!hInstance) hInstance = LoadLibrary((LPCSTR) lpParm); 
    if (!hInstance) return 1;

    hKeyboardHook = SetWindowsHookEx (  WH_KEYBOARD_LL, (HOOKPROC) processKeyDown,   hInstance,  NULL    );
    MessageLoop();
    UnhookWindowsHookEx(hKeyboardHook);
    return 0;
}

bool findDupeInstance()
{

	//Get our proc name and id first
	DWORD ourProcID = GetCurrentProcessId();
	HANDLE ourProcHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, ourProcID);
	if (ourProcHandle == NULL || ourProcHandle == INVALID_HANDLE_VALUE)
	{
		std::cout << "Failed open a handle to our own process, make sure you run as admin." << std::endl;
		return false;
	}

	TCHAR szOurProcName[MAX_PATH] = TEXT("Ours");

	HMODULE ourModHandles;

	DWORD oNeeded;

	if (!EnumProcessModules(ourProcHandle, &ourModHandles, sizeof(ourModHandles), &oNeeded))
	{
		std::cout << "Failed open a handle to our own process, make sure you run as admin." << std::endl;
		CloseHandle(ourProcHandle);
		return false;
	}

	GetModuleBaseName(ourProcHandle, ourModHandles, szOurProcName, sizeof(szOurProcName) / sizeof(TCHAR));
	CloseHandle(ourProcHandle);

	//Proc name will be stored in szOurProcName now

	//boilerplate msdn code
	//get all procs
	HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapShot == INVALID_HANDLE_VALUE || hSnapShot == NULL)
		return false;

	PROCESSENTRY32 procEntry;
	procEntry.dwSize = sizeof(PROCESSENTRY32);
	if (!Process32First(hSnapShot, &procEntry))
	{
		std::cout << "Failed to snapshot running procs, make sure run as admin." << std::endl;
		CloseHandle(hSnapShot);
		return false;
	}

	HANDLE allProcH = INVALID_HANDLE_VALUE, allModsH = INVALID_HANDLE_VALUE;
	HMODULE theirModules;
	DWORD cbModEnum = 0;
	TCHAR theirBaseModName[MAX_PATH] = TEXT("Theirs");
	bool foundDupe = false;

	do
	{
		if (procEntry.th32ProcessID == ourProcID)
			continue;

		//Those dirty bastards at ms didnt use TCHAR for PROCESSENTRY32::szExeFile
		//So we either need to start manually checking TCHAR type to compare to that, which defeats the purpose of using TCHAR
		//Or keep going until we get another field which does use TCHAR

		allProcH = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, procEntry.th32ProcessID);
		if (allProcH == INVALID_HANDLE_VALUE || allProcH == NULL)
			continue;

		if (!EnumProcessModules(allProcH, &theirModules, sizeof(theirModules), &cbModEnum))
		{
			CloseHandle(allProcH);
			continue;
		}

		GetModuleBaseName(allProcH, theirModules, theirBaseModName, sizeof(theirBaseModName) / sizeof(TCHAR));

		if (_tccmp(theirBaseModName, szOurProcName) == 0)
		{
			//Same name, different proc ids, dupe instance.
			foundDupe = true;
			CloseHandle(allProcH);
			break;
		}

		CloseHandle(allProcH);

	} while (Process32Next(hSnapShot, &procEntry));

	CloseHandle(hSnapShot);

	return foundDupe;

}


int main(int argc, char** argv)
{

	//disable this until w10 gets tested
	//if (findDupeInstance())
	//{
	//	MessageBox(NULL, "You have an instance of wpnedit running. Close that first.", "Dupe instance", MB_OK);
	//	std::cout << "You already have an instance of the wpn editor running. Close that one first. I'd close it but I'd rather not add error checks if people not running as admin and such." << std::endl;
	//	exit(-1);
	//}

	//Locate config files and such, older code but does the job
	newSetup.initialize();

	//Take care of Settings.xml, parse attributes to edit & associated values
	settingMgmt = SettingsMgr(newSetup.getApplicationDirectoryPath());
	settingMgmt.setVKFilePath(newSetup.getKeyValueNamesFilePath());

	//Add the reserved AA keybind that does loadweapons newweap on weapon attribute change
	if (!newSetup.addNewKeyBind(settingMgmt.getKeybindPrefix()))
	{
		cerr << "Could not add the required loadweapons keybind, let AA loader redownload config files, make sure app is in that dir, try again." << endl;
		exit(-1);
	}

	//try to read and parse weapon config file
	ReadWeaponConfigResults rdWpnConfigRes = myWeap.readWeaponConfigFile(newSetup.getWeaponFilePath());

	//check results
	if (rdWpnConfigRes == ReadWeaponConfigResults::Success)
	{
		std::cout << "Succesfully parsed weapons from weapon config file." << std::endl;
	}
	else if (rdWpnConfigRes == ReadWeaponConfigResults::AttributeNotUniqueToFiremode)
	{
		//They specified an attribute to edit in Settings.xml which appears more than once in a firemode, Settings.xml needs to be reset.

		std::cout << "Resetting Settings.xml, if you are going to edit it again please make sure not to insert an attribute that appears more than once in a firemode." << std::endl;
		if (!newSetup.resetSettingsXmlFile())
		{
			std::cout << "We were unable to reset your Settings.xml file to default. Make sure you are running as admin, not running in a weird or protected path, and you have permission to edit files in the directory. If you have Settings.xml open in an editor, close it and try again. Copy over default Settings.xml from original archive just to be safe." << std::endl;
			return -1;
		}
		else
		{
			std::cout << "Exiting...";
			return -1;
		}
	}
	else if (rdWpnConfigRes == ReadWeaponConfigResults::ConfigFilePathEmpty)
	{
		//They probably will have seen feedback already
		std::cout << "Config file path was empty." << std::endl;
		if (!newSetup.resetSettingsXmlFile())
		{
			std::cout << "We were unable to reset your Settings.xml file to default. Make sure you are running as admin, not running in a weird or protected path, and you have permission to edit files in the directory. If you have Settings.xml open in an editor, close it and try again. Copy over default Settings.xml from original archive just to be safe." << std::endl;
			return -1;
		}
	}
	else if (rdWpnConfigRes == ReadWeaponConfigResults::FailedToReadConfigFile)
	{
		std::cout << "Encountered an error when attempting to read Settings.xml, run as admin, make sure account has permission to edit stuff in directory, etc etc." << std::endl;
		return -1;
	}
	else if (rdWpnConfigRes == ReadWeaponConfigResults::ZeroWeaponsParsed)
	{
		std::cout << "No weapons were succesfully parsed from the target weapon config file. Redownload weapon config from AA loader and try again." << std::endl;
		return -1;
	}
	else
	{
		std::cout << "Something went terribly wrong, please remove any magnets from the side of your computer case and try again." << std::endl;
		return -1;
	}

	//store a list of keys to watch for presses
	int countOfAttributesToEdit = settingMgmt.getAttributeObjectCount();

	if (countOfAttributesToEdit <= 0)
	{
		std::cout << "You have not defined any attributes to edit. Resetting the Settings.xml file..." << std::endl;
		if (!newSetup.resetSettingsXmlFile())
		{
			std::cout << "We were unable to reset your Settings.xml file to default. Make sure you are running as admin, not running in a weird or protected path, and you have permission to edit files in the directory. If you have Settings.xml open in an editor, close it and try again. Copy over default Settings.xml from original archive just to be safe." << std::endl;
			return -1;
		}
		std::cout << "Settings file reset, feel free to start program again." << std::endl;
		return -1;
	}

	//Store a mapping of key presses to shared_ptr of attribute to edit
	for (int i = 0; i < countOfAttributesToEdit; i++)
	{
		keysToAttributeMap.insert(std::make_pair(settingMgmt.getAttributeObjectByIndex(i)->AttributeRaiseKey(), settingMgmt.getAttributeObjectByIndex(i)));
		keysToAttributeMap.insert(std::make_pair(settingMgmt.getAttributeObjectByIndex(i)->AttributeLowerKey(), settingMgmt.getAttributeObjectByIndex(i)));
	}

	//set next and prev keys
	nextWeaponKey = settingMgmt.getNextWeaponKey();
	prevWeaponKey = settingMgmt.getPrevWeaponKey();
	AAKeyCode = settingMgmt.getAAKeybindKey();
	
	//If newweaps already exists, perhaps the user would like a backup of it made
	if (newSetup.backupExistingNewWeaps())
	{
		std::cout << "Backed up previous newweaps in case you had any changes in it you wanted to save elsewhere (back up newweaps if you find a setting you like! running this program overwrites newweaps!)." << std::endl;
	}


	//I was going to do something else fairly important here but I forget what...
	//Oh yeh, write out newweaps once just to make sure we can write to it before we bother setting kbd hook
	if (!newSetup.writeOutWeaponConfigFile(myWeap))
	{
		std::cout << "Failed writing to newweaps during startup test, please make sure run as admin, newweaps isnt locked for writing, folder isnt weird, etc etc" << std::endl;
		return -1;
	}

    HANDLE hThread;
    DWORD dwThread;


	std::cout << "AA In-Game Instant Weapon Editor - By ElGatoSupreme" << endl;

	hThread = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)setKeyBoardHook, (LPVOID)argv[0], NULL, &dwThread);


	if (hThread)
	{
		WaitForSingleObject(hThread, INFINITE);
	}
	else
		return -1;

	/*
	ConfigFileManager newSetup;

	myWeapons = WeaponCFG(newSetup.getWeaponFilePath());

	myWeapons.setMinMaxVals

	mySettings = newSetup.settingMgmt;

	dropUpKey = mySettings->getIntSettingValue(DropPlusKey);
	dropDownKey = mySettings->getIntSettingValue(DropMinusKey);
	speedUpKey = mySettings->getIntSettingValue(SpeedPlusKey);
	speedDownKey = mySettings->getIntSettingValue(SpeedMinusKey);
	pitchUpKey = mySettings->getIntSettingValue(PitchPlusKey);
	pitchDownKey = mySettings->getIntSettingValue(PitchMinusKey);
	yawUpKey = mySettings->getIntSettingValue(YawPlusKey);
	yawDownKey = mySettings->getIntSettingValue(YawMinusKey);
	nextWeapKey = mySettings->getIntSettingValue(NextWeaponKey);
	prevWeapKey = mySettings->getIntSettingValue(PrevWeaponKey);
	keybindKey = mySettings->getIntSettingValue(ReservedAAKeybindKey);

	dropdelta = mySettings->getFloatSettingValue(DropDelta);
	speeddelta = mySettings->getFloatSettingValue(SpeedDelta);
	pitchdelta = mySettings->getFloatSettingValue(PitchDelta);
	yawdelta = mySettings->getFloatSettingValue(YawDelta);

	AAKeyCode = mySettings->getIntSettingValue(ReservedAAKeybindKey);

	cursor = myWeapons->head;

	std::cout << "AA In-Game Instant Weapon Editor - By ElGatoSupreme" << endl;

    hThread = CreateThread(NULL,NULL,(LPTHREAD_START_ROUTINE)   setKeyBoardHook, (LPVOID) argv[0], NULL, &dwThread);


	if (hThread) 
		{
			WaitForSingleObject(hThread,INFINITE);
		}
	else 
		return -1;*/

}