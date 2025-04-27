#ifndef AUDIO_PLAYER_H
#define AUDIO_PLAYER_H

#include <Arduino.h>

// Audio player class for Serial output
class AudioPlayer {
public:
  AudioPlayer();
  void setup();
  void playBothActive();
  void playBothInactive();
  bool isPlaying();

private:
  void sendAudioCommand(const char* command);
  bool playing;
  unsigned long lastPlayTime;
};

#endif 