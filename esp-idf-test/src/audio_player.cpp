#include "audio_player.h"

// Define the file paths for the MP3 files
const char* BOTH_ACTIVE_FILE = "C:\\Users\\divya\\Downloads\\ttsMP3.com_VoiceText_2025-4-26_19-12-24.mp3";
const char* BOTH_INACTIVE_FILE = "C:\\Users\\divya\\Downloads\\ttsMP3.com_VoiceText_2025-4-26_19-12-11.mp3";

// Constructor
AudioPlayer::AudioPlayer() {
  playing = false;
}

// Setup function
void AudioPlayer::setup() {
  Serial.println("Audio Player initialized");
  Serial.println("To play audio, connect to the ESP32's Serial port and run a Serial monitor");
  Serial.println("Available audio files:");
  Serial.print("  Both sensors active: ");
  Serial.println(BOTH_ACTIVE_FILE);
  Serial.print("  Both sensors inactive: ");
  Serial.println(BOTH_INACTIVE_FILE);
}

// Helper function to send audio commands over Serial
void AudioPlayer::sendAudioCommand(const char* command) {
  Serial.print("Playing audio: ");
  Serial.println(command);
}

// Play audio when both sensors are active (1,1)
void AudioPlayer::playBothActive() {
  Serial.print("PLAY_FILE:");
  Serial.println(BOTH_ACTIVE_FILE);
  playing = true;
  lastPlayTime = millis();
}

// Play audio when both sensors are inactive (0,0)
void AudioPlayer::playBothInactive() {
  Serial.print("PLAY_FILE:");
  Serial.println(BOTH_INACTIVE_FILE);
  playing = true;
  lastPlayTime = millis();
}

// Check if audio is currently playing
bool AudioPlayer::isPlaying() {
  if (playing) {
    if (millis() - lastPlayTime > 3000) { // Simulate 3 seconds of playback
      playing = false;
    }
  }
  return playing;
} 