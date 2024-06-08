import numpy as np
import wave

# Parameters for the WAV file
sample_rate = 44100  # Sample rate in Hz
duration = 2 * 60 + 30  # Duration in seconds (2 minutes and 30 seconds)
frequency = 440.0  # Frequency of the sine wave in Hz (A4 note)
max_amplitude = 255  # Maximum amplitude for 8-bit audio

# Generate the audio data
num_samples = sample_rate * duration
t = np.linspace(0, duration, num_samples, False)  # Time array
waveform = max_amplitude * (0.5 * np.sin(2 * np.pi * frequency * t) + 0.5)  # Generate sine wave

# Ensure the values are in the 8-bit range (0-255)
waveform = np.clip(waveform, 0, 255).astype(np.uint8)

# Save the waveform as a WAV file
with wave.open('music.wav', 'w') as wav_file:
    wav_file.setnchannels(1)  # Mono
    wav_file.setsampwidth(1)  # 8-bit
    wav_file.setframerate(sample_rate)  # 44.1kHz
    wav_file.writeframes(waveform.tobytes())

print("WAV file 'music.wav' generated successfully.")
