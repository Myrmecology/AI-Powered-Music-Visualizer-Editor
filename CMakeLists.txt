cmake_minimum_required(VERSION 3.16)

# Set Qt path with the correct MinGW version
set(CMAKE_PREFIX_PATH "C:/Qt/6.9.0/mingw_64/lib/cmake" CACHE PATH "Path to Qt installation")

# Specify the exact compiler paths to use the correct MinGW version
set(CMAKE_C_COMPILER "C:/Qt/Tools/mingw1310_64/bin/gcc.exe")
set(CMAKE_CXX_COMPILER "C:/Qt/Tools/mingw1310_64/bin/g++.exe")

project(MusicVisualizer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6 - Added Multimedia for audio playback
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets Multimedia)

# Find OpenGL
find_package(OpenGL REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/src/cpp)

# Add source files
set(SOURCES
    src/cpp/main.cpp
)

# Add executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link libraries - Added Qt6::Multimedia
target_link_libraries(${PROJECT_NAME}
    PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
    Qt6::Multimedia
    OpenGL::GL
)

# Copy Python scripts to build directory
file(COPY src/python DESTINATION ${CMAKE_BINARY_DIR}/src)