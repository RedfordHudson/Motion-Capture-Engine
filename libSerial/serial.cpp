#include "serial.h"
/*
#include <windows.h>
#include <iostream>
#include <string>

// for json parsing
#include <unordered_map>
#include <sstream>
*/

// Opens the serial port
HANDLE openSerialPort(const std::string& portName) {
    HANDLE hSerial = CreateFileA(
        portName.c_str(),
        GENERIC_READ,
        0,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (hSerial == INVALID_HANDLE_VALUE) {
        std::cerr << "Error opening port." << std::endl;
    }

    return hSerial;
}

// Configures the port: 115200 8N1
bool configurePort(HANDLE hSerial) {
    DCB dcbSerialParams = { 0 };
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

    if (!GetCommState(hSerial, &dcbSerialParams)) return false;

    dcbSerialParams.BaudRate = CBR_115200;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity   = NOPARITY;

    return SetCommState(hSerial, &dcbSerialParams);
}

// Sets timeouts to allow non-blocking reads
bool configureTimeouts(HANDLE hSerial) {
    COMMTIMEOUTS timeouts = { 0 };
    timeouts.ReadIntervalTimeout         = 50;
    timeouts.ReadTotalTimeoutConstant    = 50;
    timeouts.ReadTotalTimeoutMultiplier  = 10;
    return SetCommTimeouts(hSerial, &timeouts);
}

std::unordered_map<std::string, int> parseJsonToDict(const std::string& completedMsg) {
    std::unordered_map<std::string, int> data;
    
    // Remove the curly braces from the string
    std::string msg = completedMsg.substr(1, completedMsg.size() - 2);
    
    // Create a stringstream to read the data
    std::stringstream ss(msg);
    std::string keyValue;
    
    // Loop through the stringstream and extract key-value pairs
    while (std::getline(ss, keyValue, ',')) {
        // Trim leading and trailing spaces (if any)
        keyValue.erase(0, keyValue.find_first_not_of(" \t"));
        keyValue.erase(keyValue.find_last_not_of(" \t") + 1);
        
        // Find the colon separating key and value
        size_t colonPos = keyValue.find(":");
        
        if (colonPos != std::string::npos) {
            // Extract key and value as strings
            std::string key = keyValue.substr(0, colonPos);
            std::string valueStr = keyValue.substr(colonPos + 1);
            
            // Remove any extra spaces and quotation marks
            key.erase(0, key.find_first_not_of("\" \t"));
            key.erase(key.find_last_not_of("\" \t") + 1);
            
            valueStr.erase(0, valueStr.find_first_not_of(" \t"));
            valueStr.erase(valueStr.find_last_not_of(" \t") + 1);
            
            // Convert the value string to an integer
            int value = std::stoi(valueStr);
            
            // Insert the key-value pair into the unordered_map
            data[key] = value;
        }
    }
    
    return data;
}

void stringifyMap(std::unordered_map<std::string, int>& result) {
    for (const auto& pair : result) {
        std::cout << pair.first << ": " << pair.second << std::endl;
    }
}

// Read data from the serial port into a string
bool readAndProcess(HANDLE hSerial) {
    const DWORD bufSize = 64;
    char buffer[bufSize];
    DWORD bytesRead;
    std::string msgBuffer;

    while (true) {
        if (ReadFile(hSerial, buffer, bufSize, &bytesRead, nullptr) && bytesRead > 0) {
            msgBuffer.append(buffer, bytesRead);

            // Look for complete JSON messages
            size_t start = msgBuffer.find('{');
            size_t end = msgBuffer.find('}', start);

            while (start != std::string::npos && end != std::string::npos) {
                std::string completeMsg = msgBuffer.substr(start, end - start + 1);
                std::unordered_map<std::string, int> result = parseJsonToDict(completeMsg);

                std::cout << "ax: " << result["ax"] << std::endl;

                // Erase processed chunk
                msgBuffer.erase(0, end + 1);
                start = msgBuffer.find('{');
                end = msgBuffer.find('}', start);
            }
        }
    }

    return true;
}

namespace Serial {
    int doStuff() {
        std::string portName = "\\\\.\\COM3"; // Adjust as needed (e.g. COM4)
        HANDLE hSerial = openSerialPort(portName);

        if (hSerial == INVALID_HANDLE_VALUE) return 1;
        if (!configurePort(hSerial)) {
            std::cerr << "Failed to configure port." << std::endl;
            return 1;
        }

        if (!configureTimeouts(hSerial)) {
            std::cerr << "Failed to set timeouts." << std::endl;
            return 1;
        }

        std::cout << "Listening on " << portName << "...\n";
        readAndProcess(hSerial);

        CloseHandle(hSerial);
        return 0;
    }
}

