#pragma once

#include <string>
#include <iosfwd>
#include <fstream>
#include <memory>
#include <mutex>

using namespace std;

class LogHelperNotOpenException : public exception
{
public:
	LogHelperNotOpenException(const std::string& msg) : exception(msg.c_str()) {}
};

class LogHelper
{

public:

	//Returns single instance of LogHelper
	static LogHelper& getInstance()
	{
		std::lock_guard<std::mutex> lock(instanceGuard);

		static LogHelper instance;

		return instance;
	}

	//Enable or disable logging
	void setLoggingState(bool nState)
	{
		std::lock_guard<std::mutex> lock(instanceGuard);

		//They can set to true or false if the backing file is open or logging is enabled, as the params will have been verified
		//and we know we can open the backing file.
		if (isLoggingEnabled || backingFile.is_open())
		{
			isLoggingEnabled = nState;
		}
		else if (nState)
		{
			//try to open file if they are setting true
			if (logFileName.empty())
				isLoggingEnabled = false;
			else
			{
				backingFile.open(logFileName, ofstream::out | ofstream::trunc);
				if (!backingFile.is_open())
				{
					isLoggingEnabled = false;
				}
				else
				{
					isLoggingEnabled = true;
				}
			}
		}
		else
		{
			//The backing file is closed and they are setting false, we can allow that
			isLoggingEnabled = nState;
		}
	}

	bool getLoggingState() const
	{
		std::lock_guard<std::mutex> lock(instanceGuard);
		return isLoggingEnabled;
	}

	std::string getLoggingPath() const
	{
		std::lock_guard<std::mutex> lock(instanceGuard);
		return logFileName;
	}

	//set logging path and internally open backing file, true on success, false if fail
	bool setLoggingPathAndOpen(const std::string& nPath)
	{
		std::lock_guard<std::mutex> lock(instanceGuard);
		if (backingFile.is_open())
			backingFile.close();
		backingFile.open(nPath, std::ofstream::out | std::ofstream::trunc | std::ios::binary);
		if (!backingFile.is_open())
		{
			isLoggingEnabled = false;
			return false;
		}
		else
		{
			isLoggingEnabled = true;
			return true;
		}
	}

	std::ostream& operator<<(const std::string& newLogLine)
	{
		std::lock_guard<std::mutex> lock(instanceGuard);

		if (isLoggingEnabled && backingFile.is_open())
		{
			backingFile << newLogLine;
			return backingFile;
		}
		else
		{
			throw LogHelperNotOpenException("Log helper is not open but << operator was called.");
		}
	}


	LogHelper(const LogHelper&) = delete;
	void operator=(const LogHelper&) = delete;

	~LogHelper();

private:
	LogHelper() : isLoggingEnabled(false), logFileName("") {}

	bool isLoggingEnabled;
	std::string logFileName;
	std::ofstream backingFile;

	//Make sure we don't have a race for calls to getInstance from multiple threads
	static std::mutex instanceGuard;
};