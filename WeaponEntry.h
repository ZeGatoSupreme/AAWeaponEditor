#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <map>
#include <memory>
#include "AttributeBase.h"

using namespace std;

enum FiremodeChangeResult : int32_t
{
	AttributeDoesntExistOnWeaponAndNotForcedToAdd = -1,
	NoChangeInAttribute = 0,
	AttributeChanged = 1
};


class Firemode
{

public:

	Firemode();

	//Generally will be float values
	//but make tempalted in case need to add other types later
	template <typename T>
	T getFiremodeAttributeValue(const string& attributeName) const;

	template <typename T>
	bool setFiremodeAttributeValue(const string& attributeName, const T& inValue);

	void addNonAttributeLine(const std::string& line) { firemodeLeftOverStrings.push_back(line); }

	bool doesAttributeExist(const std::string& attrName) const { return (firemodeFloatSettings.find(attrName) != firemodeFloatSettings.end() || firemodeIntSettings.find(attrName) != firemodeIntSettings.end()); }

	friend ostream& operator<<(ostream& os, const Firemode& wpnOut);

protected:


private:

	//most values will be floating point
	std::map<string, float> firemodeFloatSettings;

	//just in case
	std::map<string, int> firemodeIntSettings;

	//And finally we will have some strings left over that we are not interested in, ie CanHitVehicle, store those so we can re-create the firemode when outputting
	std::vector<string> firemodeLeftOverStrings;

	//since we generate attribute tags that we edit, isntead of copying them from source, we need to know how many tabs to insert before we output them to ostream
	int tabsToInsertBeforeAttribute;

	//Called if user tries to change an attribute that was not present in original weapon config file, but they specified force add in settings.xml
	template <typename T>
	bool addAttributeToFiremode(const std::string& attrName, T value);

	//Ofc a weapon config entry can access the private data of its firemodes
	friend class WeaponConfigEntry;

	//Ofc the weapon config generator can access private data of firemode
	friend class WeaponConfig;

};


class WeaponConfigEntry
{

public:

	WeaponConfigEntry();

	WeaponConfigEntry(const string& nWeaponName);

	void AddFiremode(const Firemode& nFireMode) { firemodeContainer.push_back(nFireMode); if (!isValid) isValid = true; }

	bool getIsValid() const { return isValid; }

	string getWeaponName() const { return weaponName; }

	const Firemode& getFiremodeByIndex(int fIndex) const { return firemodeContainer[fIndex]; }

	Firemode& getFiremodeByIndex(int fIndex) { return firemodeContainer[fIndex]; }

	//Returns abs value of weapon attribute change, 0 if no change (i.e. weapon is at max val anyways)
	FiremodeChangeResult setFiremodeAttributeValue(int firemodeIndex, const std::shared_ptr<AttributeBase>& refToAttributeProps, uint32_t keyCode);

	int getFiremodeCount() const { return firemodeContainer.size(); }

	friend ostream& operator<<(ostream& os, const WeaponConfigEntry& wpnOut);


private:

	//is the weapon entry valid, as far as we can tell
	bool isValid;

	//For now two is the max, could ahve more in future possibly so use dynamic container
	std::vector<Firemode> firemodeContainer;

	//name of weapon
	string weaponName;
};




//meh dont need to implement anything, juust want the type
class WeaponAttributeNotFoundException : public exception
{
public:
	WeaponAttributeNotFoundException() : exception() {};
	WeaponAttributeNotFoundException(const string& excMsg) : exception(excMsg.c_str()) {};

};

//throws a WeaponAttributeNotFoundException if target attribute doesnt exist
template <>
inline float Firemode::getFiremodeAttributeValue<float>(const string& attributeName) const
{
	auto fmMapIterator = firemodeFloatSettings.begin();

	if (( fmMapIterator = firemodeFloatSettings.find(attributeName)) == firemodeFloatSettings.end())
		throw WeaponAttributeNotFoundException("Attribute " + attributeName + " was not found in firemode map.");  //value doesnt exist, we are returning a float so we dont have a way to communicate this to caller other than throwing an exception
	else
		return fmMapIterator->second;
}


template<>
inline bool Firemode::addAttributeToFiremode<int>(const std::string & attrName, int value)
{
	if (firemodeIntSettings.find(attrName) != firemodeIntSettings.end())
	{
		return false;	//attribute exists, have caller throw exception as we are adding this bewcause we think it doesnt exist
	}

	firemodeIntSettings.insert(std::make_pair(attrName, value));
	return true;
}

template<>
inline bool Firemode::addAttributeToFiremode<float>(const std::string & attrName, float value)
{
	if (firemodeFloatSettings.find(attrName) != firemodeFloatSettings.end())
	{
		return false;	//attribute exists, have caller throw exception as we are adding this bewcause we think it doesnt exist
	}

	firemodeFloatSettings.insert(std::make_pair(attrName, value));
	return true;
}


//throws a WeaponAttributeNotFoundException if target attribute doesnt exist
template <>
inline int Firemode::getFiremodeAttributeValue<int>(const string& attributeName) const
{
	auto fmMapIterator = firemodeIntSettings.begin();

	if ((fmMapIterator = firemodeIntSettings.find(attributeName)) == firemodeIntSettings.end())
		throw WeaponAttributeNotFoundException("Attribute " + attributeName + " was not found in firemode map.");  //value doesnt exist, we are returning an int so we dont have a way to communicate this to caller other than throwing an exception
	else
		return fmMapIterator->second;
}


//returns true if the value was found and set, false if it was not found at all
template<>
inline bool Firemode::setFiremodeAttributeValue<float>(const string& attributeName, const float& inValue)
{
	auto fmMapIterator = firemodeFloatSettings.begin();

	if ((fmMapIterator = firemodeFloatSettings.find(attributeName)) == firemodeFloatSettings.end())
		return false;
	else
	{
		//set the value and return
		fmMapIterator->second = inValue;
		return true;
	}
}

//returns true if the value was found and set, false if it was not found at all
template<>
inline bool Firemode::setFiremodeAttributeValue<int>(const string& attributeName, const int& inValue)
{
	auto fmMapIterator = firemodeIntSettings.begin();

	if ((fmMapIterator = firemodeIntSettings.find(attributeName)) == firemodeIntSettings.end())
		return false;
	else
	{
		//set the value and return
		fmMapIterator->second = inValue;
		return true;
	}
}