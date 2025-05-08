#!/usr/bin/env python3
"""
Main entry point for standalone Python testing of the AI Music Visualizer components.
"""

import sys
import argparse
from src.python.audio_analyzer import AudioAnalyzer
from src.python.mood_classifier import MoodClassifier
from src.python.analysis_server import AnalysisServer


def test_audio_analysis(file_path):
    """Test audio analysis on a single file."""
    analyzer = AudioAnalyzer()

    print(f"Analyzing {file_path}...")
    result = analyzer.analyze_audio_file(file_path)

    if "error" in result:
        print(f"Error: {result['error']}")
        return

    print(f"Duration: {result['duration']:.2f} seconds")
    print(f"Sample rate: {result['sample_rate']} Hz")
    print(f"Tempo: {result['beats']['tempo']:.1f} BPM")
    print(f"Beat count: {result['beats']['beat_count']}")

    # Create visualization
    analyzer.plot_analysis(result, "analysis_visualization.png")
    print("Analysis visualization saved to analysis_visualization.png")


def test_mood_classification(file_path):
    """Test mood classification on a single file."""
    classifier = MoodClassifier()

    print(f"Classifying mood for {file_path}...")
    try:
        audio_data, sr = classifier.analyzer.load_audio(file_path)
        result = classifier.predict_mood(audio_data, sr)

        print(f"Predicted mood: {result['predicted_mood']}")
        print(f"Confidence: {result['confidence']:.2f}")
        print("\nProbabilities:")
        for mood, prob in result["probabilities"].items():
            print(f"  {mood}: {prob:.2f}")
    except Exception as e:
        print(f"Error: {e}")


def start_server(port=5555):
    """Start the analysis server."""
    server = AnalysisServer(port)

    try:
        print(f"Starting analysis server on port {port}...")
        server.start()
    except KeyboardInterrupt:
        print("\nShutting down server...")
        server.stop()


def main():
    parser = argparse.ArgumentParser(
        description="AI Music Visualizer Python Components"
    )
    subparsers = parser.add_subparsers(dest="command", help="Command to run")

    # Analyze command
    analyze_parser = subparsers.add_parser("analyze", help="Analyze an audio file")
    analyze_parser.add_argument("file", help="Audio file to analyze")

    # Classify command
    classify_parser = subparsers.add_parser(
        "classify", help="Classify mood for an audio file"
    )
    classify_parser.add_argument("file", help="Audio file to classify")

    # Server command
    server_parser = subparsers.add_parser("server", help="Start the analysis server")
    server_parser.add_argument(
        "--port", type=int, default=5555, help="Port to run server on"
    )

    args = parser.parse_args()

    if args.command == "analyze":
        test_audio_analysis(args.file)
    elif args.command == "classify":
        test_mood_classification(args.file)
    elif args.command == "server":
        start_server(args.port)
    else:
        parser.print_help()


if __name__ == "__main__":
    main()
