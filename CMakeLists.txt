cmake_minimum_required(VERSION 3.16)

# Add vcpkg toolchain file (add this before project declaration)
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake" 
        CACHE STRING "Vcpkg toolchain file")
endif()

# Set Qt path with the correct MinGW version
set(CMAKE_PREFIX_PATH "C:/Qt/6.9.0/mingw_64/lib/cmake" CACHE PATH "Path to Qt installation")

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
find_package(cppzmq CONFIG REQUIRED)

# Find nlohmann_json
find_package(nlohmann_json CONFIG REQUIRED)

# Find RtAudio
find_package(RtAudio CONFIG REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/src/cpp)

# Add source files
set(SOURCES
    src/cpp/main.cpp
    src/cpp/analyzer_client.h
)

# Add executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME}
    PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::Quick
    OpenGL::GL
    cppzmq
    nlohmann_json::nlohmann_json
    RtAudio::rtaudio
)

# Copy Python scripts to build directory
file(COPY src/python DESTINATION ${CMAKE_BINARY_DIR}/src)

# Install targets
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
install(DIRECTORY src/python DESTINATION bin/src)