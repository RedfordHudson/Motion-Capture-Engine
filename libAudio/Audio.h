#ifndef AUDIO_H
#define AUDIO_H

#include <string>

namespace Audio {
    /**
     * Plays an audio file and waits for user input to exit
     * @param fileName Path to the audio file to play
     */
    void playSound(const std::string& fileName);

    /**
     * Opens an audio file and creates an alias for it
     * @param fileName Path to the audio file
     * @param alias Alias to use for the file
     * @return True if successful, false otherwise
     */
    bool openAudioFile(const std::string& fileName, const std::string& alias);

    /**
     * Starts playback of an audio file
     * @param alias Alias of the audio file
     * @return True if successful, false otherwise
     */
    bool startPlayback(const std::string& alias);

    /**
     * Gets the length of the audio file in milliseconds
     * @param alias Alias of the audio file
     * @return Length in milliseconds
     */
    int getAudioLength(const std::string& alias);

    /**
     * Closes an audio file
     * @param alias Alias of the audio file
     */
    void closeAudioFile(const std::string& alias);
}

#endif // AUDIO_H 