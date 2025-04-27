#include "Audio.h"
#include <iostream>
#include <Windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

namespace Audio {
    
    bool openAudioFile(const std::string& fileName, const std::string& alias) {
        // First, ensure a clean state by closing any audio file with the same alias
        std::string closeCommand = "close " + alias;
        mciSendString(closeCommand.c_str(), NULL, 0, NULL);
        
        // Then try to open the file
        std::string command = "open \"" + fileName + "\" type mpegvideo alias " + alias;
        MCIERROR error = mciSendString(command.c_str(), NULL, 0, NULL);
        
        if (error) {
            char errorText[256];
            mciGetErrorString(error, errorText, sizeof(errorText));
            std::cerr << "Failed to open audio file '" << fileName << "': " << errorText << std::endl;
            return false;
        }
        
        std::cout << "Successfully opened audio file: " << fileName << std::endl;
        return true;
    }

    bool startPlayback(const std::string& alias) {
        std::string command = "play " + alias;
        MCIERROR error = mciSendString(command.c_str(), NULL, 0, NULL);
        
        if (error) {
            char errorText[256];
            mciGetErrorString(error, errorText, sizeof(errorText));
            std::cerr << "Failed to play audio file: " << errorText << std::endl;
            return false;
        }
        return true;
    }

    int getAudioLength(const std::string& alias) {
        char lengthStr[128];
        std::string command = "status " + alias + " length";
        MCIERROR error = mciSendString(command.c_str(), lengthStr, sizeof(lengthStr), NULL);
        
        if (error) {
            char errorText[256];
            mciGetErrorString(error, errorText, sizeof(errorText));
            std::cerr << "Failed to get audio length: " << errorText << std::endl;
            return 0;
        }
        
        return atoi(lengthStr);
    }

    void closeAudioFile(const std::string& alias) {
        std::string command = "close " + alias;
        MCIERROR error = mciSendString(command.c_str(), NULL, 0, NULL);
        
        if (error) {
            char errorText[256];
            mciGetErrorString(error, errorText, sizeof(errorText));
            std::cerr << "Warning: Failed to close audio file: " << errorText << std::endl;
        }
    }

    bool playSoundSimple(const std::string& filePath, bool async) {
        std::cout << "Playing sound using Windows API: " << filePath << std::endl;
        
        // Set flags for PlaySound
        DWORD flags = SND_FILENAME;
        if (async) {
            flags |= SND_ASYNC;
        }
        
        // Use the ANSI version directly with the string's c_str()
        BOOL result = PlaySoundA(filePath.c_str(), NULL, flags);
        
        if (!result) {
            DWORD error = GetLastError();
            std::cerr << "Failed to play sound. Windows error code: " << error << std::endl;
            return false;
        }
        
        return true;
    }

    // Check if audio is still playing
    bool isPlaying(const std::string& alias) {
        char statusStr[128];
        std::string command = "status " + alias + " mode";
        MCIERROR error = mciSendString(command.c_str(), statusStr, sizeof(statusStr), NULL);
        
        if (error) {
            char errorText[256];
            mciGetErrorString(error, errorText, sizeof(errorText));
            std::cerr << "Failed to check playback status: " << errorText << std::endl;
            return false;
        }
        
        return (strcmp(statusStr, "playing") == 0);
    }

    void playSound(const std::string& fileName) {
        std::cout << "Playing " << fileName << "..." << std::endl;
        
        // Use a default alias
        std::string alias = "audiofile";
        
        // Open the audio file
        if (!openAudioFile(fileName, alias)) {
            return;
        }
        
        // Start playback
        if (!startPlayback(alias)) {
            closeAudioFile(alias);
            return;
        }
        
        // Get and display audio length
        int length = getAudioLength(alias);
        std::cout << "Audio length: " << length / 1000.0 << " seconds" << std::endl;
        
        // Wait for playback to complete
        while (isPlaying(alias)) {
            Sleep(100); // Check every 100ms
        }
        
        // Clean up
        closeAudioFile(alias);
    }
} 