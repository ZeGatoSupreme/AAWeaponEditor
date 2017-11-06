#include "LoggerMgr.h"
#include <string>
#include <iostream>

using namespace std;

//define static private mutex guarding getInstance
std::mutex LogHelper::instanceGuard;

LogHelper::~LogHelper()
{
	if (backingFile.is_open())
		backingFile.close();
}