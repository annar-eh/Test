// time tracker.cpp : main project file.

#include "stdafx.h"

using namespace System;

unsigned int const STRLEN = 50;
unsigned int totalTimeSpent = 0;		//the amount of time spent in selected programs the past 7 days
unsigned int timer = 0;

bool bPause = false;

time_t firstTime;		//set this to the current time whenever a program is detected to have started
POINT lastMousePos;

struct programInfo
{
	TCHAR executableName[STRLEN];
	TCHAR name[STRLEN];
	unsigned int seconds;
};

struct dateInfo
{
	unsigned int day;
	unsigned int month;
	unsigned int year;
	unsigned int time;		//amount of time spent that day on selected programs
};

typedef std::vector<programInfo> programlist;

dateInfo dates[7];
programlist programs;

programInfo *currentProgram;

bool processRunning(const TCHAR* name, HANDLE &snapshot);
bool init();			//if initialization failed for whatever reason it returns false and the application terminates
int isAnyProgramRunning();
int cmpDates(struct tm &timeinfo, dateInfo &dateinfo);
DWORD WINAPI threadFunction(LPVOID lpParam);
DWORD WINAPI inputFunction(LPVOID lpParam);
bool writeToFile();
void addProgram();
void lookUp();
void eraseProgram();
void editProgram();
void lookUpDates();
void pause();
void printMenu();
bool verifyAlphaNumerical(std::string &str);
bool checkMousePosition(POINT &pos);

int main(array<System::String ^> ^args)
{
	if(!init())
	{
		//could not read file
		return 0;
	}
	firstTime = 0;
	CreateThread(NULL, 0, &inputFunction, NULL, 0, NULL);
	currentProgram = NULL;

	while(true)
	{
		//wait about 10 seconds so we dont have to tax the CPU as much
		//it is not important to have this running at maximum speed anyway
		Sleep(10000);
		CreateThread(NULL, 0, &threadFunction, NULL, 0, NULL);
	}
	return 0;
}

DWORD WINAPI inputFunction(LPVOID lpParam)
{
	std::string iStr;
	do
	{
		int i = 0;
		printMenu();
	
		std::getline(std::cin, iStr);
		if(verifyAlphaNumerical(iStr))
		{
			i = std::stoi(iStr);
		}

		switch(i)
		{
		case 1:
			addProgram();
			break;

		case 2:
			lookUp();
			break;

		case 3:
			lookUpDates();
			break;

		case 4:
			pause();
			break;

		case 5:
			editProgram();
			break;

		case 6:
			eraseProgram();
			break;

		case 7:
			if(writeToFile())
			{
				exit(0);
			}
			else break;

		default:
			break;

		}
	}
	while(true);
}

bool init()
{
	std::wifstream filert("data.dat");

	if(!filert.is_open())
	{
		std::cout << "could not open file, terminating application" << std::endl;
		std::cin.get();
		return false;
	}

	wchar_t date[STRLEN] = L"";
	wchar_t name[STRLEN] = L"";
	wchar_t mins[STRLEN] = L"";
	int i = 0;

	while(filert.good())
	{
		filert.getline(date, STRLEN);
		//check to see if we are done reading dates
		if(_tcscmp(date, L"ED") == 0)
		{
			break;
		}

		dates[i].day = _ttoi(date);
		filert.getline(date, STRLEN);
		dates[i].month = _ttoi(date);
		filert.getline(date, STRLEN);
		dates[i].year = _ttoi(date);
		filert.getline(date, STRLEN);
		dates[i].time = _ttoi(date);

		++i;
	}
	
	time_t t = time(0);
	struct tm timeinfo;
	localtime_s(&timeinfo, &t);
	if(cmpDates(timeinfo, dates[6]) != 0)
	{
		for(int x = 1; x < 7; ++x)
		{
			dates[x - 1] = dates[x];
		}

		dates[6].day = timeinfo.tm_mday;
		dates[6].month = timeinfo.tm_mon + 1;
		dates[6].year = timeinfo.tm_year + 1900;
		dates[6].time = 0;
	}

	//read in how many elements of programInfo there is stored
	filert.getline(name, STRLEN);
	int size = 0;
	if(_tcscmp(name, L"") != 0)
	{
		size = _ttoi(name);
	}

	
	//read in all the programs and their respective time in minutes
	programInfo newProgram;
	i = 0;
	while(filert.good() && i < size)
	{
		filert.getline(name, STRLEN);
		filert.getline(mins, STRLEN);
		filert.getline(date, STRLEN);
		if(_tcscmp(name, L"") != 0)
		{
			newProgram.seconds = _ttoi(mins);
			_tcscpy(newProgram.executableName, name);
			_tcscpy(newProgram.name, date);
			programs.push_back(newProgram);
		}
		i++;
	}
	//read the last potential line in the file
	//that holds the amount of minutes spent on programs today
	filert.getline(name, STRLEN);
	if(_tcscmp(name, L"") != 0)
	{
		totalTimeSpent = _ttoi(name);
	}
	filert.close();
	return true;
}

void pause()
{
	bPause = !bPause;
	time_t t = time(0);
	struct tm timeinfo;
	localtime_s(&timeinfo, &t);
	std::cout << std::endl;
	GetCursorPos(&lastMousePos);
	if(bPause)
	{	
		timer = 0;
		std::cout << "Took a break at: " << timeinfo.tm_hour << ":" << (timeinfo.tm_min < 10 ? "0" : "" ) << timeinfo.tm_min << std::endl << std::endl;
		if(currentProgram != NULL)
		{
			int difference = t - firstTime;
			int i = isAnyProgramRunning();
			programs[i].seconds += difference;
			dates[6].time += difference;
		}
	}
	else
	{
		std::cout << "Came back at: " << timeinfo.tm_hour << ":" << (timeinfo.tm_min < 10 ? "0" : "" ) << timeinfo.tm_min << std::endl << std::endl;	
		if(currentProgram != NULL)
		{
			firstTime = time(0);
		}
	}
}

bool writeToFile()
{
	std::wofstream filert("data.dat");
	//have we managed to properly open the file for reading?
	if(!filert.is_open())
	{
		std::cout << "Could not read to file, do you want to continue the program and try to fix the error?" << std::endl;
		int input = 0;

		do
		{
			std::cout << "Input(1. Y, 2. N): ";
			std::cin >> input;
			//ignore the '\n' character 
			std::cin.ignore(1);
		}
		while(input == 1 || input == 2);
		if(input == 1)
		{
			return false;
		}
		else return true;
	}
	//file is now open and good to go
	//lets read in the dates as defined in file_format.txt file
	for(int x = 0; x < 7; ++x)
	{
		filert << dates[x].day;
		filert << '\n';
		filert << dates[x].month;
		filert << '\n';
		filert << dates[x].year;
		filert << '\n';
		filert << dates[x].time;
		filert << '\n';
	}
	filert << L"ED";
	filert << '\n';
	programlist::iterator itr;
	filert << programs.size();
	filert << '\n';

	for(itr = programs.begin(); itr != programs.end(); ++itr)
	{
		if(filert.good())
		{
			filert << itr->executableName;
			filert << '\n';
			filert << itr->seconds;
			filert << '\n';
			filert << itr->name;
			filert << '\n';
		}
		else break;
	}
	totalTimeSpent = 0;
	for(int x = 0; x < 7; ++x)
	{
		totalTimeSpent += dates[x].time;
	}

	filert << totalTimeSpent;
	filert.close();
	return true;
}

void editProgram()
{
	wchar_t input[STRLEN];
	programlist::iterator itr;
	bool  valid = false;
	do
	{
		std::cout << "Write the name of the program(exit to return back to the main menu): ";
		std::wcin.getline(input, STRLEN);
		if(_tcscmp(input, L"exit") == 0) return;
		for(itr = programs.begin(); itr != programs.end(); ++itr)
		{
			if(_tcscmp(input, itr->executableName) == 0)
			{
				valid = true;
				std::wcout << "Current descriptive name: " << itr->name << std::endl;
				std::cout << "Edit the description of this program to what you want it to be: " << std::endl;
				std::wcin.getline(input, STRLEN);
				_tcscpy(itr->name, input);
				break;
			}
		}
	}
	while(valid);
}

void lookUpDates()
{
	std::cout << "Date format: YYYY-DD-MM" << std::endl;
	for(int x = 0; x < 7; ++x)
	{
		std::cout << dates[x].year << "-" << (dates[x].day < 10 ? "0" : "") 
			<< dates[x].day << "-" << (dates[x].month < 10 ? "0" : "")
			<< dates[x].month << " Minutes: " << dates[x].time / 60 << std::endl;
	}
	std::cout << std::endl;
}

void eraseProgram()
{
	wchar_t input[STRLEN];
	programlist::iterator itr;
	int iInput = 1;
	do
	{
		std::cout << "Write the name of the program: ";
		std::wcin.getline(input, STRLEN);
		for(itr = programs.begin(); itr != programs.end(); ++itr)
		{
			if(_tcscmp(itr->executableName, input) == 0)
			{
				std::wcout << L"You wrote: " <<  input << std::endl << L"Is this okay? (1. Y, 2. N, 3. Go Back)" << std::endl;
				std::cin >> iInput;

				//ignore the '\n' character
				std::wcin.ignore(1);
				if(iInput == 2) break;
				else if(iInput == 3) return;

				programs.erase(itr);
				return;
			}
		}
	} 
	while(iInput != 1);
}

void printMenu()
{
	std::cout << "What do you want to do?" << std::endl;
	std::cout << "1. Add a new program to be looked at" << std::endl;
	std::cout << "2. Look up stats" << std::endl;
	std::cout << "3. Look at dates" << std::endl;
	std::cout << "4. Take a break/I'm back!" << std::endl;
	std::cout << "5. Edit program info" << std::endl;
	std::cout << "6. Erase a program" << std::endl;
	std::cout << "7. Exit" << std::endl;
}

int cmpDates(struct tm &timeinfo, dateInfo &date)
{
	if(timeinfo.tm_year + 1900 < date.year)
	{
		return -1; //the first date is less then the second date
	}
	else if(timeinfo.tm_year + 1900 > date.year)
	{
		return 1; //the first date is greater then the second date
	}
	else //years are equivalent
	{	//add one so that it represent months correspondingly to dateInfo
		if(timeinfo.tm_mon + 1 < date.month)
		{
			return -1; 
		}
		else if(timeinfo.tm_mon + 1 > date.month)
		{
			return 1;
		}
		else
		{
			if(timeinfo.tm_mday < date.day)
			{
				return -1;
			}
			else if(timeinfo.tm_mday > date.day)
			{
				return 1;
			}
			else return 0;	//the dates are equivalent
		}
	}
}

int isAnyProgramRunning()	//with few instances of programInfo this loop is not a problem
{							//but if the list grows up to much more it will become slow and bog the program down
							//might want to look into a solution to that at some point
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	int i = 0;
	programlist::iterator itr;

	for(itr = programs.begin(); itr != programs.end(); ++itr)
	{
		if(processRunning(itr->executableName, snapshot))
		{
			return i;
		}
		++i;
	}
	return -1;
}

bool processRunning(const TCHAR* name, HANDLE &snapShot)
{
	if(snapShot == INVALID_HANDLE_VALUE)
		return false;

	PROCESSENTRY32 procEntry;
	procEntry.dwSize = sizeof(PROCESSENTRY32);
	

	if(!Process32First(snapShot, &procEntry))
		return false;

	do
	{
		if(_tcscmp( procEntry.szExeFile, name) == 0)
			return true;
	}
	while(Process32Next(snapShot, &procEntry));

	return false;
}

bool verifyAlphaNumerical(std::string &str)
{
	int i = 0;
	try
	{
		i = std::stoi(str);
	}
	catch (const std::out_of_range &oor)
	{
		std::cout << "You can't type in a number that big!" << std::endl;
		return false;
	}
	catch (const std::invalid_argument &ia)
	{
		std::cout << "Only digits please" << std::endl;
		return false;
	}
	catch (...)
	{
		std::cout << "Unknown input error" << std::endl;
		return false;
	}
	return true;
}

void addProgram()
{
	TCHAR input[STRLEN] = L"";
	TCHAR input2[STRLEN] = L"";
	std::string iStr;
	int iInput;

	do
	{
		bool validInput = false;
		std::cout << "Enter the name of the .exe file for the program(case sensitive and need .exe)" << std::endl;
		std::wcin.getline(input, STRLEN);
		std::cout << "Enter your own identifier for the program: " << std::endl;
		std::wcin.getline(input2, STRLEN);
		do
		{
			std::wcout << L"You wrote: " <<  input << " and " << input2 << std::endl << L"Is this okay? (1. Y, 2. N, 3. Go Back)" << std::endl;
			std::getline(std::cin, iStr);

			if(verifyAlphaNumerical(iStr))
			{
				validInput = true;
				iInput = std::stoi(iStr);
			}
		}
		while(!validInput);
		if(iInput == 3) return;
	} 
	while(iInput != 1);

	programInfo newProgram;
	_tcscpy(newProgram.executableName, input);
	_tcscpy(newProgram.name, input2);
	newProgram.seconds = 0;
	programs.push_back(newProgram);
}

void lookUp()
{
	std::cout << std::endl;
	programlist::iterator itr;
	int i = 0;
	for(itr = programs.begin(); itr != programs.end(); ++itr)
	{
		if(std::wcslen(itr->name) > 25)
		{
			for(int x = 0; x < 22; ++x)
			{
				std::wcout << itr->name[x];
			}
			std::wcout << L"...";
		}
		else std::wcout << std::setw(25) << itr->name;		
	
		std::wcout << L" -- ";
		if(std::wcslen(itr->executableName) > 25)
		{
			for(int x = 0; x < 22; ++x)
			{
				std::wcout << itr->executableName[x];
			}
			std::wcout <<  L"...";
		}
		else std::wcout << std::setw(25) << itr->executableName;

		std::wcout << std::setw(5) << itr->seconds / 3600 << L" hours and " << 
			std::setw(2) << (itr->seconds / 60) % 60 << L" minutes" << std::endl;
		i++;
		if(i % 5 == 0)
		{
			std::cout << "Press any key to continue the look up." << std::endl;
			std::cin.get();
		}
	}
	totalTimeSpent = 0;
	for(int x = 0; x < 7; ++x)
	{
		totalTimeSpent += dates[x].time;
	}
	std::cout << std::endl << "Total time spent last 7 days: " << totalTimeSpent / 3600 << " hours and " << (totalTimeSpent / 60) % 60 << " minutes." << std::endl;
	std::cout << std::endl;
}

DWORD WINAPI threadFunction(LPVOID lpParam)
{
	
	POINT tempPos;
	GetCursorPos(&tempPos);
	if(!bPause) 
	{
		timer += 10;
	}
	else if(checkMousePosition(tempPos))		//check to see if the cursor position has changed since the user took a pause
	{
		lastMousePos = tempPos;
		pause();
		printMenu();
	}

	if(bPause) return 0;
	int i = isAnyProgramRunning();
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if(i != -1 && firstTime == 0 && currentProgram == NULL)
	{
		currentProgram = &(programs[i]);
		firstTime = time(0);
	}
	else if(i == -1)
	{
		if(currentProgram != NULL)
		{
			time_t currentTime = time(0);
			time_t difference = currentTime - firstTime;
			currentProgram->seconds += difference;
			dates[6].time += difference;
			firstTime = 0;
			currentProgram = NULL;
			if(!writeToFile())
			{
				std::cout << "AN ERROR OCCURED WHILE TRYING TO WRITE TO FILE" << std::endl;
			}
		}
	}
	//within the 10 seconds time frame another program may have started
	else if(i != -1 && !processRunning(currentProgram->executableName, snapshot))
	{
		currentProgram = &(programs[i]);
		firstTime = time(0);
	}

	if(currentProgram != NULL)
	{			
		struct tm timeinfo;
		localtime_s(&timeinfo, &firstTime);
		if(cmpDates(timeinfo, dates[6]) != 0)	//if a program is running but the current date has changed (program was being used past midnight)
		{
			for(int x = 1; x < 7; ++x)
			{
				dates[x - 1] = dates[x];
			}
			dates[6].year = timeinfo.tm_year + 1900;
			dates[6].month = timeinfo.tm_mon + 1;
			dates[6].day = timeinfo.tm_mday;
			dates[6].time = 0;
		}
	}
	//every 30 minutes
	if(timer % 1800 == 0 && timer != 0)
	{
		PlaySoundW(TEXT("alarm.wav"), NULL, SND_FILENAME | SND_ASYNC);
	}
	
	return 0;
}

bool checkMousePosition(POINT &pos)
{
	if(pos.x == lastMousePos.x && pos.y == lastMousePos.y)
	{
		return false;
	}
	return true;
}