import librosa
import numpy as np
import soundfile as sf
from typing import Dict, List, Tuple
import json
import matplotlib.pyplot as plt

class AudioAnalyzer:
    def __init__(self, sample_rate=44100):
        self.sample_rate = sample_rate
        
    def load_audio(self, file_path: str) -> Tuple[np.ndarray, int]:
        """Load audio file and return audio data and sample rate."""
        try:
            audio_data, sr = librosa.load(file_path, sr=self.sample_rate)
            return audio_data, sr
        except Exception as e:
            print(f"Error loading audio: {e}")
            return None, None
    
    def detect_beats(self, audio_data: np.ndarray) -> Dict:
        """Detect beats and tempo in audio data."""
        # Get tempo and beat frames
        tempo, beat_frames = librosa.beat.beat_track(y=audio_data, sr=self.sample_rate)
        
        # Convert beat frames to time
        beat_times = librosa.frames_to_time(beat_frames, sr=self.sample_rate)
        
        return {
            "tempo": float(tempo),
            "beat_times": beat_times.tolist(),
            "beat_count": len(beat_frames)
        }
    
    def extract_features(self, audio_data: np.ndarray) -> Dict:
        """Extract various audio features for visualization."""
        # Spectral centroid (brightness indicator)
        spectral_centroids = librosa.feature.spectral_centroid(y=audio_data, sr=self.sample_rate)[0]
        
        # RMS energy
        rms = librosa.feature.rms(y=audio_data)[0]
        
        # Onset detection
        onset_frames = librosa.onset.onset_detect(y=audio_data, sr=self.sample_rate)
        onset_times = librosa.frames_to_time(onset_frames, sr=self.sample_rate)
        
        # Chromagram for harmonic content
        chroma = librosa.feature.chroma_stft(y=audio_data, sr=self.sample_rate)
        
        return {
            "spectral_centroids": spectral_centroids.tolist(),
            "rms_energy": rms.tolist(),
            "onset_times": onset_times.tolist(),
            "chroma_mean": np.mean(chroma, axis=1).tolist()
        }
    
    def get_waveform_data(self, audio_data: np.ndarray, num_points: int = 1000) -> List[float]:
        """Downsample audio data for waveform visualization."""
        if len(audio_data) <= num_points:
            return audio_data.tolist()
        
        # Downsample by taking mean of chunks
        chunk_size = len(audio_data) // num_points
        downsampled = [np.mean(audio_data[i:i+chunk_size]) for i in range(0, len(audio_data), chunk_size)]
        return downsampled[:num_points]
    
    def analyze_audio_file(self, file_path: str) -> Dict:
        """Complete analysis of an audio file."""
        audio_data, sr = self.load_audio(file_path)
        
        if audio_data is None:
            return {"error": "Failed to load audio file"}
        
        beats = self.detect_beats(audio_data)
        features = self.extract_features(audio_data)
        waveform = self.get_waveform_data(audio_data)
        
        return {
            "sample_rate": sr,
            "duration": float(len(audio_data) / sr),
            "beats": beats,
            "features": features,
            "waveform": waveform
        }
    
    def plot_analysis(self, analysis_data: Dict, save_path: str = None):
        """Visualize the analysis results."""
        fig, axes = plt.subplots(3, 1, figsize=(12, 10))
        
        # Plot waveform
        axes[0].plot(analysis_data["waveform"])
        axes[0].set_title("Waveform")
        
        # Plot spectral centroids
        axes[1].plot(analysis_data["features"]["spectral_centroids"])
        axes[1].set_title("Spectral Centroids")
        
        # Plot RMS energy
        axes[2].plot(analysis_data["features"]["rms_energy"])
        axes[2].set_title("RMS Energy")
        
        plt.tight_layout()
        
        if save_path:
            plt.savefig(save_path)
        else:
            plt.show()

if __name__ == "__main__":
    # Test the analyzer
    analyzer = AudioAnalyzer()
    
    # Example usage
    test_file = "test_audio.wav"  # Replace with actual test file
    
    print("Analyzing audio file...")
    results = analyzer.analyze_audio_file(test_file)
    
    if "error" not in results:
        print(f"Tempo: {results['beats']['tempo']} BPM")
        print(f"Duration: {results['duration']} seconds")
        print(f"Beat count: {results['beats']['beat_count']}")
        
        # Save results to JSON
        with open("analysis_results.json", "w") as f:
            json.dump(results, f, indent=2)
        
        # Create visualization
        analyzer.plot_analysis(results, "analysis_plot.png")
    else:
        print(results["error"])