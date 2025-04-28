@echo off

REM Activate virtual environment
if exist venv (
    call venv\Scripts\activate
) else (
    echo Creating virtual environment...
    python -m venv venv
    call venv\Scripts\activate
    pip install librosa numpy soundfile scikit-learn matplotlib pybind11 pyzmq torch
)

REM Check if build directory exists
if not exist build (
    echo Creating build directory...
    mkdir build
    cd build
    cmake -G "Visual Studio 17 2022" -A x64 ..
    cmake --build . --config Release
    cd ..
)

REM Launch the application
cd build
Release\MusicVisualizer.exe