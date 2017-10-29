#pragma once

#include "WeaponEntry.h"
#include "SettingsMgr.h"
#include <string>

//default constructor
WeaponConfigEntry::WeaponConfigEntry()
	:
	isValid(false), weaponName("")
{

}

WeaponConfigEntry::WeaponConfigEntry(const string& nWeaponName)
	:
	isValid(false), weaponName(nWeaponName)
{

}

FiremodeChangeResult WeaponConfigEntry::setFiremodeAttributeValue(int firemodeIndex, const std::shared_ptr<AttributeBase>& refToAttributeProps, uint32_t keyCode)
{
	if (firemodeIndex >= firemodeContainer.size())
		throw IndexOutOfRangeException("Index out of range");
	else if (refToAttributeProps->AttributeLowerKey() != keyCode && refToAttributeProps->AttributeRaiseKey() != keyCode)
		throw InvalidOperationException("Key didn't match values in Settings.xml"); //eh what to do in this case... there is only one place that will be calling this and params will match, so edge case in case another person branches or something

	//Get attribute type
	SETTING_VALUE_TYPE valType = refToAttributeProps->AttributeType();

	bool raiseValue = refToAttributeProps->AttributeRaiseKey() == keyCode;

	if (valType == SETTING_VALUE_TYPE::FloatValue)
	{
		 const std::shared_ptr<AttributeFloat>& fPtr = std::dynamic_pointer_cast<AttributeFloat, AttributeBase>(refToAttributeProps);
		 if (fPtr == nullptr)
			 throw InvalidOperationException("Attribute class type does not match class field attribute type");	//Again... this is in case anyone else modifies this
		
		 //See if we need to change it
		 float oldCurVal;
		 try
		 {
			 oldCurVal = firemodeContainer[firemodeIndex].getFiremodeAttributeValue<float>(fPtr->AttributeName());
		 }
		 catch (WeaponAttributeNotFoundException& ex)
		 {
			 if (refToAttributeProps->AddAttributeIfMissing())
			 {
				 //Add to firemode with min value
				 if (!firemodeContainer[firemodeIndex].addAttributeToFiremode(refToAttributeProps->AttributeName(), fPtr->getAttributeMin()))
					 throw InvalidOperationException("Tried to add attribute we thought was missing to firemode, but it already exists.");	//Validation purposes
				 oldCurVal = firemodeContainer[firemodeIndex].getFiremodeAttributeValue<float>(fPtr->AttributeName());
			 }
			 else
			 {
				 printf("%s is not present for weapon %s and AddToweaponIfMissing is false.\n", refToAttributeProps->AttributeName().c_str(), getWeaponName().c_str());
				 return FiremodeChangeResult::AttributeDoesntExistOnWeaponAndNotForcedToAdd;
			 }
		 }

		 //Need to change this, if value is above/below there is a bug

		 ////Check if already maxed or min'ed
		 //if (raiseValue && oldCurVal + SettingsMgr::floatEpsilon >= fPtr->getAttributeMax())
			// return FiremodeChangeResult::NoChangeInAttribute;
		 //else if (!raiseValue && oldCurVal - SettingsMgr::floatEpsilon <= fPtr->getAttributeMin())
			// return FiremodeChangeResult::NoChangeInAttribute;

		 float newCurVal = 0;

		 if (oldCurVal - SettingsMgr::floatEpsilon > fPtr->getAttributeMax())
		 {
			 //It is above the maximum. This is probably the first time we are editing this attribute on this firemode.
			 oldCurVal = fPtr->getAttributeMax();
		 }
		 else if (oldCurVal + SettingsMgr::floatEpsilon < fPtr->getAttributeMin())
		 {
			 //It is below the min. This is probably the first time we are editing this attribute on this firemode.
			 oldCurVal = fPtr->getAttributeMin();
		 }
		 else
		 {
			 //See if the change amount is zero
			 if (fPtr->getAttributeDelta() <= SettingsMgr::floatEpsilon)
				 return FiremodeChangeResult::NoChangeInAttribute;
			 
			 //change value
			 newCurVal = raiseValue ? oldCurVal + fPtr->getAttributeDelta() : oldCurVal - fPtr->getAttributeDelta();
		 }

		 //If we went over max or min, set to max/min
		 if (raiseValue && newCurVal - SettingsMgr::floatEpsilon > fPtr->getAttributeMax())
			 newCurVal = fPtr->getAttributeMax();
		 else if (!raiseValue && newCurVal + SettingsMgr::floatEpsilon < fPtr->getAttributeMin())
			 newCurVal = fPtr->getAttributeMin();

		 //set attribute value
		 if (!firemodeContainer[firemodeIndex].setFiremodeAttributeValue<float>(fPtr->AttributeName(), newCurVal))
		 {
			 throw InvalidOperationException(std::string("Error setting target attribute name " + fPtr->AttributeName()));	//Again, if anyone branches and makes a change that breaks something, let them know.
		 }
		 else
		 {
			 //TODO: add enum for return result, ie value changed, value the same
			 return FiremodeChangeResult::AttributeChanged;
		 }
	
	}
	else if (valType == SETTING_VALUE_TYPE::IntValue)
	{
		const std::shared_ptr<AttributeInt>& fPtr = std::dynamic_pointer_cast<AttributeInt, AttributeBase>(refToAttributeProps);
		if (fPtr == nullptr)
			throw InvalidOperationException("Attribute class type does not match class field attribute type");	//Again... this is in case anyone else modifies this


		//See if we need to change it
		int oldCurVal;

		try
		{
			oldCurVal = firemodeContainer[firemodeIndex].getFiremodeAttributeValue<int>(fPtr->AttributeName());
		}
		catch (WeaponAttributeNotFoundException& ex)
		{
			if (refToAttributeProps->AddAttributeIfMissing())
			{
				//Add to firemode with min value
				if (!firemodeContainer[firemodeIndex].addAttributeToFiremode(refToAttributeProps->AttributeName(), fPtr->getAttributeMin()))
					throw InvalidOperationException("Tried to add attribute we thought was missing to firemode, but it already exists.");	//Validation purposes
				oldCurVal = firemodeContainer[firemodeIndex].getFiremodeAttributeValue<int>(fPtr->AttributeName());
			}
			else
			{
				printf("%s is not present for weapon %s and AddToweaponIfMissing is false.\n", refToAttributeProps->AttributeName().c_str(), getWeaponName().c_str());
				return FiremodeChangeResult::AttributeDoesntExistOnWeaponAndNotForcedToAdd;
			}
		}

		////Check if already maxed or min'ed
		//if (raiseValue && oldCurVal >= fPtr->getAttributeMax())
		//	return FiremodeChangeResult::NoChangeInAttribute;
		//else if (!raiseValue && oldCurVal <= fPtr->getAttributeMin())
		//	return FiremodeChangeResult::NoChangeInAttribute;

		int newCurVal = 0;

		if (oldCurVal > fPtr->getAttributeMax())
		{
			//It is above the maximum. This is probably the first time we are editing this attribute on this firemode.
			oldCurVal = fPtr->getAttributeMax();
		}
		else if (oldCurVal < fPtr->getAttributeMin())
		{
			//It is below the min. This is probably the first time we are editing this attribute on this firemode.
			oldCurVal = fPtr->getAttributeMin();
		}
		else
		{
			//See if the change amount is zero
			if (fPtr->getAttributeDelta() == 0)
				return FiremodeChangeResult::NoChangeInAttribute;

			//change value
			newCurVal = raiseValue ? oldCurVal + fPtr->getAttributeDelta() : oldCurVal - fPtr->getAttributeDelta();
		}

		//If we went over max or min, set to max/min
		if (raiseValue && newCurVal > fPtr->getAttributeMax())
			newCurVal = fPtr->getAttributeMax();
		else if (!raiseValue && newCurVal < fPtr->getAttributeMin())
			newCurVal = fPtr->getAttributeMin();

		//set attribute value
		if (!firemodeContainer[firemodeIndex].setFiremodeAttributeValue<int>(fPtr->AttributeName(), newCurVal))
		{
			throw InvalidOperationException(std::string("Error setting target attribute name " + fPtr->AttributeName()));	//Again, if anyone branches and makes a change that breaks something, let them know.
		}
		else
		{
			//TODO: add enum for return result, ie value changed, value the same
			return FiremodeChangeResult::AttributeChanged;
		}

	}
	else
	{
		throw InvalidOperationException("Attribute type specified in class field doesn't match float or int.");	//Again... this is in case anyone else modifies this
	}

}


ostream& operator<<(ostream& os, const WeaponConfigEntry& wpnOut)
{

	string outputStr = "";

	if (!wpnOut.getIsValid())
		return os;

	string weaponNameLine = "<Weapon " + wpnOut.getWeaponName() + ">";

	os << weaponNameLine << std::endl;

	for (int i = 0; i < wpnOut.getFiremodeCount(); i++)
	{
		os << wpnOut.getFiremodeByIndex(i);
	}

	os << "</Weapon>" << std::endl;

	return os;
}

ostream& operator<<(ostream& os, const Firemode& fmOut)
{

	string outputStr = "";

	//output all the strings we arent interested in editing, then output our editing string, then end firemode
	for (int i = 0; i < fmOut.firemodeLeftOverStrings.size() - 1; i++)
	{
		os << fmOut.firemodeLeftOverStrings[i] << std::endl;
	}

	string attributeLine = "";



	for (auto fAtrIt = fmOut.firemodeFloatSettings.begin(); fAtrIt != fmOut.firemodeFloatSettings.end(); fAtrIt++)
	{
		attributeLine = "";

		char breakToInsert = fmOut.useTabsForBreaks ? '\t' : ' ';

		//insert our tabs (or spaces)
		for (int i = 0; i < fmOut.breaksToInsertBeforeAttribute; i++)
			os << breakToInsert;

		//get attribute name and value, set it to use fixed floating point notation so we dont end up with 23e+009
		os << '<' << fAtrIt->first << " " << std::fixed << fAtrIt->second << '>' << "</" << fAtrIt->first << '>' << std::endl;
		
	}


	for (auto iAtrIt = fmOut.firemodeIntSettings.begin(); iAtrIt != fmOut.firemodeIntSettings.end(); iAtrIt++)
	{
		attributeLine = "";

		char breakToInsert = fmOut.useTabsForBreaks ? '\t' : ' ';

		//insert our tabs
		for (int i = 0; i < fmOut.breaksToInsertBeforeAttribute; i++)
			os << '\t';

		//get attribute name
		os << '<' << iAtrIt->first << " " << iAtrIt->second << '>' << "</" << iAtrIt->first << '>' << std::endl;

	}

	//output last non attribute string which is closing firemode tab
	os << fmOut.firemodeLeftOverStrings[fmOut.firemodeLeftOverStrings.size() - 1] << std::endl;

	return os;
}



Firemode::Firemode()
	:
	breaksToInsertBeforeAttribute(0), useTabsForBreaks(true)
{
//TODO
}