import zmq
import json
import threading
import numpy as np
from audio_analyzer import AudioAnalyzer
from mood_classifier import MoodClassifier
import time

class AnalysisServer:
    def __init__(self, port=5555):
        self.port = port
        self.context = zmq.Context()
        self.socket = self.context.socket(zmq.REP)
        self.analyzer = AudioAnalyzer()
        self.classifier = MoodClassifier()
        self.running = True
    
    def start(self):
        """Start the analysis server."""
        self.socket.bind(f"tcp://*:{self.port}")
        print(f"Analysis server started on port {self.port}")
        
        while self.running:
            try:
                # Wait for request from C++ client
                message = self.socket.recv_json()
                
                # Process the request
                response = self.process_request(message)
                
                # Send response back
                self.socket.send_json(response)
                
            except Exception as e:
                error_response = {"status": "error", "message": str(e)}
                self.socket.send_json(error_response)
    
    def process_request(self, request):
        """Process incoming request and return response."""
        command = request.get("command")
        
        if command == "analyze_file":
            file_path = request.get("file_path")
            return self.analyze_audio_file(file_path)
        
        elif command == "analyze_chunk":
            audio_data = np.array(request.get("audio_data"))
            sample_rate = request.get("sample_rate", 44100)
            return self.analyze_audio_chunk(audio_data, sample_rate)
        
        elif command == "stop":
            self.running = False
            return {"status": "stopping"}
        
        else:
            return {"status": "error", "message": f"Unknown command: {command}"}
    
    def analyze_audio_file(self, file_path):
        """Analyze an audio file and return results."""
        try:
            # Load and analyze audio
            audio_data, sr = self.analyzer.load_audio(file_path)
            
            if audio_data is None:
                return {"status": "error", "message": "Failed to load audio file"}
            
            # Get analysis results
            beats = self.analyzer.detect_beats(audio_data)
            features = self.analyzer.extract_features(audio_data)
            waveform = self.analyzer.get_waveform_data(audio_data)
            
            # Get mood prediction
            mood_result = self.classifier.predict_mood(audio_data, sr)
            
            return {
                "status": "success",
                "data": {
                    "duration": float(len(audio_data) / sr),
                    "sample_rate": sr,
                    "beats": beats,
                    "features": features,
                    "waveform": waveform,
                    "mood": mood_result
                }
            }
        
        except Exception as e:
            return {"status": "error", "message": str(e)}
    
    def analyze_audio_chunk(self, audio_data, sample_rate):
        """Analyze a chunk of audio data (for real-time processing)."""
        try:
            # Extract features for the chunk
            features = self.analyzer.extract_features(audio_data)
            
            # Get waveform data
            waveform = self.analyzer.get_waveform_data(audio_data, num_points=100)
            
            # For real-time, we might not want full mood analysis on every chunk
            # Instead, we can do it periodically or use a simpler metric
            rms = np.sqrt(np.mean(audio_data**2))
            
            return {
                "status": "success",
                "data": {
                    "waveform": waveform,
                    "rms": float(rms),
                    "spectral_centroid": float(features["spectral_centroids"][0])
                }
            }
        
        except Exception as e:
            return {"status": "error", "message": str(e)}
    
    def stop(self):
        """Stop the server."""
        self.running = False
        self.socket.close()
        self.context.term()

def main():
    """Start the analysis server."""
    server = AnalysisServer()
    
    try:
        server.start()
    except KeyboardInterrupt:
        print("\nShutting down server...")
        server.stop()

if __name__ == "__main__":
    main()