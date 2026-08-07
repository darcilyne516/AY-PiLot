cmake_minimum_required(VERSION 3.22.1)
project("ay_pilot_native")

add_library(ay_pilot_native SHARED
    ay_pilot_jni.cpp
    ../../../../shared-engine/src/ay_pilot_engine.cpp
)

target_include_directories(ay_pilot_native PRIVATE
    ../../../../shared-engine/include
)

find_library(log-lib log)

target_link_libraries(ay_pilot_native
    ${log-lib}
)
