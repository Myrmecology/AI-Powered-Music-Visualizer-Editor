cmake_minimum_required(VERSION 3.16)
project(MusicVisualizer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets OpenGL Quick)

# Find OpenGL
find_package(OpenGL REQUIRED)

# Find ZeroMQ
find_package(cppzmq REQUIRED)

# Find RtAudio
find_package(PkgConfig REQUIRED)
pkg_check_modules(RTAUDIO REQUIRED rtaudio)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/src/cpp)
include_directories(${cppzmq_INCLUDE_DIR})
include_directories(${ZeroMQ_INCLUDE_DIR})
include_directories(${RTAUDIO_INCLUDE_DIRS})

# Add nlohmann/json (assuming it's installed via package manager)
include_directories(${CMAKE_SOURCE_DIR}/external/json/include)

# Add source files
set(SOURCES
    src/cpp/main.cpp
    src/cpp/analyzer_client.h
)

# Add executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME}
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::Quick
    OpenGL::GL
    cppzmq
    ${RTAUDIO_LIBRARIES}
)

# Copy Python scripts to build directory
file(COPY src/python DESTINATION ${CMAKE_BINARY_DIR}/src)

# Install targets
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
install(DIRECTORY src/python DESTINATION bin/src)