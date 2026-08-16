# Audio subsystem
set(AUDIO_SOURCES
    Audio/AudioSystem.cpp
)

add_library(ChiaEngineAudio STATIC ${AUDIO_SOURCES})
target_include_directories(ChiaEngineAudio PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Audio
    ${CMAKE_SOURCE_DIR}/src/3rdparty
)

# Link miniaudio (we are using the single header, so no extra lib needed)
# On Linux, we might need to link with dl or pthread for miniaudio backends
if(UNIX AND NOT APPLE)
    target_link_libraries(ChiaEngineAudio PRIVATE dl pthread m)
endif()

# Make sure the audio module is included in the main ChiaEngineLib
# This will be done in the parent CMakeLists.txt