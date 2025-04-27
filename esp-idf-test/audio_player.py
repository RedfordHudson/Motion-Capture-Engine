import serial
import time
import os
import pygame
import threading
import re

# Initialize pygame mixer
pygame.mixer.init()

# Global variables
current_audio = None
audio_thread = None
is_playing = False

def play_audio(file_path):
    global current_audio, is_playing
    
    # Check if file exists
    if not os.path.exists(file_path):
        print(f"Error: File not found: {file_path}")
        return
    
    try:
        # Load and play the audio file
        pygame.mixer.music.load(file_path)
        pygame.mixer.music.play()
        is_playing = True
        
        # Wait for the audio to finish playing
        while pygame.mixer.music.get_busy():
            time.sleep(0.1)
        
        is_playing = False
        print(f"Finished playing: {file_path}")
    except Exception as e:
        print(f"Error playing audio: {e}")
        is_playing = False

def audio_player_thread(file_path):
    global audio_thread
    play_audio(file_path)
    audio_thread = None

def process_command(command):
    global audio_thread, is_playing
    
    # Check if it's a play file command
    if command.startswith("PLAY_FILE:"):
        file_path = command[10:].strip()
        print(f"Received command to play: {file_path}")
        
        # If audio is already playing, stop it
        if is_playing:
            pygame.mixer.music.stop()
            is_playing = False
            if audio_thread:
                audio_thread.join(timeout=1.0)
        
        # Start a new thread to play the audio
        audio_thread = threading.Thread(target=audio_player_thread, args=(file_path,))
        audio_thread.daemon = True
        audio_thread.start()

def main():
    # Serial port configuration
    port = 'COM5'  # Change this to match your ESP32's port
    baud_rate = 115200
    
    print(f"Connecting to {port} at {baud_rate} baud...")
    
    try:
        # Open serial port
        ser = serial.Serial(port, baud_rate, timeout=1)
        print("Connected to ESP32")
        print("Listening for audio commands...")
        print("Press Ctrl+C to exit")
        
        # Buffer for incomplete commands
        buffer = ""
        
        while True:
            # Read data from serial port
            if ser.in_waiting:
                data = ser.read(ser.in_waiting).decode('utf-8')
                buffer += data
                
                # Process complete commands
                while '\n' in buffer:
                    line, buffer = buffer.split('\n', 1)
                    line = line.strip()
                    if line:
                        process_command(line)
            
            # Small delay to prevent CPU hogging
            time.sleep(0.01)
    
    except serial.SerialException as e:
        print(f"Error opening serial port: {e}")
    except KeyboardInterrupt:
        print("\nExiting...")
    finally:
        # Clean up
        if 'ser' in locals() and ser.is_open:
            ser.close()
        pygame.mixer.quit()

if __name__ == "__main__":
    main() 