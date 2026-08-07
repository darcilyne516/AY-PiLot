cmake_minimum_required(VERSION 3.10)
project(AYPilotEngine)

set(CMAKE_CXX_STANDARD 17)

include_directories(include)

add_library(ay_pilot_engine STATIC
    src/ay_pilot_engine.cpp
)

# For testing
add_executable(engine_test
    src/ay_pilot_engine.cpp
)
target_include_directories(engine_test PRIVATE include)
