#pragma once

#include <windows.h>
#include <iostream>
#include <string>

// for json parsing
#include <unordered_map>
#include <sstream>

HANDLE openSerialPort(const std::string& portName);
bool configurePort(HANDLE hSerial);
bool configureTimeouts(HANDLE hSerial);
std::unordered_map<std::string, int> parseJsonToDict(const std::string& completedMsg);
void stringifyMap(std::unordered_map<std::string, int>& result);
std::unordered_map<std::string, int> readAndProcess(HANDLE hSerial);

namespace Serial {
    HANDLE getSerialHandle(const std::string& portName);
    int doStuff();
}
