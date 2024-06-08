import numpy as np
import wave
import pyttsx3

# Parameters for the WAV file
sample_rate = 44100  # Sample rate in Hz
duration = 20  # Duration in seconds
max_amplitude = 127  # Maximum amplitude for 8-bit audio

# Text to be spoken
text = "Baby shark."

# Initialize the text-to-speech engine
engine = pyttsx3.init()

# Adjust speaking rate
engine.setProperty('rate', 150)  # You can adjust this value to change the speaking rate

# Set voice type if available
voices = engine.getProperty('voices')
for voice in voices:
    if "english" in voice.languages and "female" in voice.gender:
        engine.setProperty('voice', voice.id)
        break

# Convert text to speech and save it as a WAV file
engine.save_to_file(text, 'spoken_text.wav')
engine.runAndWait()

# Load spoken text
with wave.open('spoken_text.wav', 'rb') as spoken_file:
    spoken_data = spoken_file.readframes(-1)
    spoken_duration = len(spoken_data) / sample_rate  # Calculate duration of spoken text

# Generate the audio data for the entire duration
num_samples = sample_rate * duration
t = np.linspace(0, duration, num_samples, False)  # Time array
if spoken_duration < duration:
    # If spoken text duration is less than total duration, generate sine wave for the remaining duration
    frequency = 440.0  # Frequency of the sine wave in Hz (A4 note)
    waveform = max_amplitude * (0.5 * np.sin(2 * np.pi * frequency * t) + 0.5)  # Generate sine wave
else:
    # If spoken text duration is greater than or equal to total duration, truncate spoken text
    spoken_data = spoken_data[:num_samples]

# Ensure the values are in the 8-bit range (-128 to 127) for signed 8-bit PCM
waveform = np.clip(waveform, -127, 127).astype(np.int8)

# Create a combined waveform
combined_waveform = spoken_data + waveform.tobytes()

# Save the combined waveform as a WAV file
with wave.open('music_with_baby_shark.wav', 'w') as wav_file:
    wav_file.setnchannels(1)  # Mono
    wav_file.setsampwidth(1)  # 8-bit
    wav_file.setframerate(sample_rate)  # 44.1kHz
    wav_file.writeframes(combined_waveform)

print("WAV file 'music_with_baby_shark.wav' generated successfully.")
