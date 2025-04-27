# ESP32 Audio Player

This Python script listens to the Serial port for commands from an ESP32 and plays MP3 files when it receives them.

## Requirements

1. Python 3.6 or higher
2. Required Python packages:
   - pyserial
   - pygame

## Installation

1. Install the required packages:

```bash
pip install pyserial pygame
```

2. Make sure your ESP32 is connected to your computer via USB.

## Usage

1. Open a terminal or command prompt.

2. Run the script:

```bash
python audio_player.py
```

3. The script will connect to the ESP32's Serial port (default: COM5) and listen for commands.

4. When the ESP32 sends a command like `PLAY_FILE:C:\path\to\your\file.mp3`, the script will play the specified MP3 file.

5. Press Ctrl+C to exit the script.

## Configuration

You can modify the following settings in the script:

- `port`: The Serial port where your ESP32 is connected (default: 'COM5')
- `baud_rate`: The baud rate for Serial communication (default: 115200)

## Troubleshooting

- **Port not found**: Make sure your ESP32 is connected and the correct port is specified in the script.
- **File not found**: Ensure the file paths in your ESP32 code match the actual locations of your MP3 files.
- **Audio not playing**: Check that your system's audio is working and that the MP3 files are valid.

## How It Works

1. The script connects to the ESP32's Serial port.
2. It listens for commands that start with "PLAY_FILE:".
3. When a command is received, it extracts the file path and plays the MP3 file.
4. If a new command is received while audio is playing, it stops the current audio and plays the new file. 