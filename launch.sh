#!/bin/bash

# Activate virtual environment
if [ -d "venv" ]; then
    source venv/bin/activate
else
    echo "Creating virtual environment..."
    python -m venv venv
    source venv/bin/activate
    pip install librosa numpy soundfile scikit-learn matplotlib pybind11 pyzmq torch
fi

# Check if build directory exists
if [ ! -d "build" ]; then
    echo "Creating build directory..."
    mkdir build
    cd build
    cmake ..
    make
    cd ..
fi

# Launch the application
cd build
./MusicVisualizer