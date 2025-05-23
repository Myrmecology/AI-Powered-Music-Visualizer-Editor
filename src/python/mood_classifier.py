import numpy as np
import librosa
import torch
import torch.nn as nn
from sklearn.preprocessing import StandardScaler
import joblib
from typing import Dict, List, Tuple


class MoodClassifierNN(nn.Module):
    """Simple neural network for mood classification."""

    def __init__(self, input_size=19, hidden_size=64, num_classes=5):
        super(MoodClassifierNN, self).__init__()
        self.fc1 = nn.Linear(input_size, hidden_size)
        self.relu = nn.ReLU()
        self.fc2 = nn.Linear(hidden_size, hidden_size // 2)
        self.fc3 = nn.Linear(hidden_size // 2, num_classes)
        self.softmax = nn.Softmax(dim=1)

    def forward(self, x):
        x = self.fc1(x)
        x = self.relu(x)
        x = self.fc2(x)
        x = self.relu(x)
        x = self.fc3(x)
        return self.softmax(x)


class MoodClassifier:
    def __init__(self, model_path: str = None):
        self.MOODS = ["happy", "sad", "energetic", "calm", "angry"]
        self.model = MoodClassifierNN(input_size=19)
        self.scaler = StandardScaler()

        # Fit scaler with dummy data to avoid issues when predicting
        dummy_features = np.random.randn(100, 19)  # 19 features expected
        self.scaler.fit(dummy_features)

        if model_path:
            self.load_model(model_path)

    def extract_features(self, audio_data: np.ndarray, sr: int) -> np.ndarray:
        """Extract audio features for mood classification."""
        try:
            # Ensure input is 1D
            if len(audio_data.shape) > 1:
                audio_data = audio_data.flatten()

            features = []

            # MFCCs - these return multiple coefficients
            mfccs = librosa.feature.mfcc(y=audio_data, sr=sr, n_mfcc=13)
            # Take mean across time axis to get 13 values
            for i in range(13):
                features.append(float(np.mean(mfccs[i])))

            # Spectral centroid - take mean if it returns multiple values
            spectral_centroid = librosa.feature.spectral_centroid(y=audio_data, sr=sr)
            features.append(float(np.mean(spectral_centroid)))

            # Spectral bandwidth
            spectral_bandwidth = librosa.feature.spectral_bandwidth(y=audio_data, sr=sr)
            features.append(float(np.mean(spectral_bandwidth)))

            # Spectral rolloff
            spectral_rolloff = librosa.feature.spectral_rolloff(y=audio_data, sr=sr)
            features.append(float(np.mean(spectral_rolloff)))

            # Tempo - this returns a single value
            tempo, _ = librosa.beat.beat_track(y=audio_data, sr=sr)
            features.append(float(tempo))

            # Zero crossing rate
            zcr = librosa.feature.zero_crossing_rate(y=audio_data)
            features.append(float(np.mean(zcr)))

            # RMS energy
            rms = librosa.feature.rms(y=audio_data)
            features.append(float(np.mean(rms)))

            # Convert to numpy array and ensure it's 1D
            features_array = np.array(features, dtype=np.float32)

            print(f"Extracted {len(features_array)} features")

            # Verify we have exactly 19 features
            assert (
                len(features_array) == 19
            ), f"Expected 19 features, got {len(features_array)}"

            return features_array

        except Exception as e:
            print(f"Error in feature extraction: {e}")
            # Return a default feature vector if extraction fails
            return np.zeros(19, dtype=np.float32)

    def preprocess_features(self, features: np.ndarray) -> torch.Tensor:
        """Preprocess features for the neural network."""
        # Ensure features is the right shape
        if features.ndim == 1:
            features = features.reshape(1, -1)

        # Scale features
        features_scaled = self.scaler.transform(features)

        # Convert to tensor
        return torch.FloatTensor(features_scaled)

    def predict_mood(self, audio_data: np.ndarray, sr: int) -> Dict:
        """Predict mood from audio data."""
        try:
            # Extract features
            features = self.extract_features(audio_data, sr)

            # Preprocess
            features_tensor = self.preprocess_features(features)

            # Predict
            with torch.no_grad():
                output = self.model(features_tensor)
                probabilities = output.numpy()[0]
                predicted_mood_idx = np.argmax(probabilities)

            return {
                "predicted_mood": self.MOODS[predicted_mood_idx],
                "confidence": float(probabilities[predicted_mood_idx]),
                "probabilities": {
                    mood: float(prob) for mood, prob in zip(self.MOODS, probabilities)
                },
            }
        except Exception as e:
            # Return a default prediction if something goes wrong
            print(f"Error in prediction: {e}")
            return {
                "predicted_mood": "unknown",
                "confidence": 0.0,
                "probabilities": {mood: 0.2 for mood in self.MOODS},
            }

    def train_model(self, audio_files: List[str], labels: List[str], epochs: int = 50):
        """Train the mood classifier model."""
        X = []
        y = []

        # Extract features from all audio files
        for file_path, label in zip(audio_files, labels):
            try:
                audio_data, sr = librosa.load(file_path, sr=22050)
                features = self.extract_features(audio_data, sr)
                X.append(features)
                y.append(self.MOODS.index(label))
            except Exception as e:
                print(f"Error processing {file_path}: {e}")

        X = np.array(X)
        y = np.array(y)

        # Fit scaler
        X_scaled = self.scaler.fit_transform(X)

        # Convert to tensors
        X_tensor = torch.FloatTensor(X_scaled)
        y_tensor = torch.LongTensor(y)

        # Training loop
        criterion = nn.CrossEntropyLoss()
        optimizer = torch.optim.Adam(self.model.parameters(), lr=0.001)

        for epoch in range(epochs):
            optimizer.zero_grad()
            outputs = self.model(X_tensor)
            loss = criterion(outputs, y_tensor)
            loss.backward()
            optimizer.step()

            if (epoch + 1) % 10 == 0:
                print(f"Epoch [{epoch+1}/{epochs}], Loss: {loss.item():.4f}")

    def save_model(self, path: str):
        """Save the trained model and scaler."""
        torch.save(
            {"model_state_dict": self.model.state_dict(), "scaler": self.scaler}, path
        )

    def load_model(self, path: str):
        """Load a trained model and scaler."""
        checkpoint = torch.load(path)
        self.model.load_state_dict(checkpoint["model_state_dict"])
        self.scaler = checkpoint["scaler"]
        self.model.eval()

    def get_mood_color(self, mood: str) -> Tuple[float, float, float]:
        """Return RGB color for a mood."""
        mood_colors = {
            "happy": (1.0, 0.7, 0.0),  # Warm orange
            "sad": (0.2, 0.3, 0.8),  # Cool blue
            "energetic": (1.0, 0.0, 0.3),  # Hot pink
            "calm": (0.3, 0.8, 0.5),  # Soft green
            "angry": (0.9, 0.1, 0.1),  # Bright red
        }
        return mood_colors.get(mood, (1.0, 1.0, 1.0))  # Default to white


if __name__ == "__main__":
    # Example usage
    classifier = MoodClassifier()

    # Test on a single file
    test_file = "test_audio.wav"  # Replace with actual test file

    try:
        audio_data, sr = librosa.load(test_file, sr=22050)
        result = classifier.predict_mood(audio_data, sr)

        print(f"Predicted mood: {result['predicted_mood']}")
        print(f"Confidence: {result['confidence']:.2f}")
        print("\nProbabilities:")
        for mood, prob in result["probabilities"].items():
            print(f"  {mood}: {prob:.2f}")
    except Exception as e:
        print(f"Error: {e}")
