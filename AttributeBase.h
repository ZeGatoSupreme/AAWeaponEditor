#pragma once

#include <stdint.h>
#include <string>

enum SETTING_VALUE_TYPE : int32_t
{
	FloatValue,
	IntValue
};


//Hmmm lets do this slightly differently, I would like to store all SettingAttributes in one container
class AttributeBase
{
public:

	AttributeBase() : settingName(""), isValid(false), addIfMissing(false), keyCodeRaise(0), keyCodeLower(0)
	{

	}

	const std::string& AttributeName() const { return settingName; }

	const uint32_t AttributeRaiseKey() const { return keyCodeRaise; }

	const uint32_t AttributeLowerKey() const { return keyCodeLower; }

	const bool AddAttributeIfMissing() const { return addIfMissing; }

	const bool IsAttributeValid() const { return isValid; }

	const SETTING_VALUE_TYPE AttributeType() const { return settingType; }



	//Don't really have anything extra to do here
	virtual ~AttributeBase() {};

protected:

	//Anyone calling these will be a subclass
#pragma region contructor overloads
	AttributeBase(const std::string& attributeName, SETTING_VALUE_TYPE atrType) : settingName(attributeName), isValid(false),
		addIfMissing(false), keyCodeRaise(0), keyCodeLower(0), settingType(atrType)
	{

	}

	AttributeBase(const std::string& attributeName, SETTING_VALUE_TYPE atrType, bool alwaysAdd) : settingName(attributeName), isValid(false),
		addIfMissing(alwaysAdd), keyCodeRaise(0), keyCodeLower(0), settingType(atrType)
	{

	}

	AttributeBase(const std::string& attributeName, SETTING_VALUE_TYPE atrType, bool alwaysAdd, uint32_t raiseKey, uint32_t lowerKey) : settingName(attributeName), isValid(false),
		addIfMissing(alwaysAdd), keyCodeRaise(raiseKey), keyCodeLower(lowerKey), settingType(atrType)
	{

	}
#pragma endregion

	//Subclasses should have access to these
	void SetIsValid(bool valid) { isValid = valid; }
	void SetKeyRaise(uint32_t keyRaise) { keyCodeRaise = keyRaise; }
	void SetKeyLower(uint32_t keyLower) { keyCodeLower = keyLower; }
	void SetAddIfMissing(bool alwaysAdd) { addIfMissing = alwaysAdd; }

	friend class SettingAttributeFactory;

private:

	//Raise and lower virtual key codes
	uint32_t keyCodeRaise, keyCodeLower;

	//Whether to add this to the firemode or not if it is not already there.
	bool addIfMissing;

	//Is this attribute valid or not (if error in parsing or invalid values or structure, etc, set false)
	bool isValid;

	//Is this a float or an int
	SETTING_VALUE_TYPE settingType;

	//Exact name of the setting from the weapon config file as specified by the user in Settings.xml
	std::string settingName;

};

//So we can store all in one container
class AttributeFloat : public AttributeBase
{
public:

	AttributeFloat()
		: AttributeBase(), minValue(-1), maxValue(-1), deltaValue(0)
	{

	}

	AttributeFloat(const std::string& attributeName, SETTING_VALUE_TYPE atrType, bool alwaysAdd, uint32_t raiseKey, uint32_t lowerKey, float minVal, float maxVal, float deltaVal)
		: AttributeBase(attributeName, atrType, alwaysAdd, raiseKey, lowerKey), minValue(minVal), maxValue(maxVal), deltaValue(deltaVal)
	{

	}

	float getAttributeMax() const { return maxValue; }

	float getAttributeMin() const { return minValue; }

	float getAttributeDelta() const { return deltaValue; }

	//Again, nothing special to do here, we don't have any class members on the heap or anything
	virtual ~AttributeFloat() {}




private:

	float minValue, maxValue, deltaValue;
	friend class SettingAttributeFactory;
};

//So we can store all in one container
class AttributeInt : public AttributeBase
{
public:

	AttributeInt()
		: AttributeBase(), minValue(-1), maxValue(-1), deltaValue(0)
	{

	}

	AttributeInt(const std::string& attributeName, SETTING_VALUE_TYPE atrType, bool alwaysAdd, uint32_t raiseKey, uint32_t lowerKey, int minVal, int maxVal, int deltaVal)
		: AttributeBase(attributeName, atrType, alwaysAdd, raiseKey, lowerKey), minValue(minVal), maxValue(maxVal), deltaValue(deltaVal)
	{

	}

	int getAttributeMax() const { return maxValue; }

	int getAttributeMin() const { return minValue; }

	int getAttributeDelta() const { return deltaValue; }

	//Again, nothing special to do here, we don't have any class members on the heap or anything
	virtual ~AttributeInt() {}

private:

	int minValue, maxValue, deltaValue;
	friend class SettingAttributeFactory;
};