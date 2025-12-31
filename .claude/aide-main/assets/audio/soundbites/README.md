# Soundbites Directory

This directory contains original audio soundbites from movies, TV shows, games, and memes for Dee Dee's voice responses.

## File Naming Convention

Audio files should be named according to the phrase they contain. The system will automatically match phrases to audio files.

Example:
- `terminator_ill_be_back.mp3` - For "I'll be back!"
- `starwars_may_the_force.mp3` - For "May the Force be with you!"
- `mario_its_a_me.mp3` - For "It's a me, Mario!"

## Supported Formats

- MP3 (recommended)
- WAV
- OGG
- M4A

## Adding New Soundbites

1. Place your audio file in this directory
2. Update the `audioFileMap` in `/src/renderer/deedee-tts.js` to map the phrase to your filename
3. The system will automatically use the audio file when that phrase is spoken

## How It Works

When Dee Dee needs to speak a phrase:
1. The system checks if an audio file exists for that phrase
2. If found, it plays the original audio soundbite
3. If not found, it falls back to Text-to-Speech (TTS)

## Legal Note

Make sure you have the rights to use any audio clips you add here. Consider:
- Using royalty-free sound effect libraries
- Creating your own recordings/impressions
- Obtaining proper licenses for copyrighted material
