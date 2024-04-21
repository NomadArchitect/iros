cmake_minimum_required(VERSION 3.25.2)

project(audioTest CXX)

find_package(Dius REQUIRED)

file(GLOB headers CONFIGURE_DEPENDS "*.h")
add_library(audio_test_headers INTERFACE)
add_library(PackageManager::audio_test_headers ALIAS audio_test_headers)

target_sources(audio_test_headers INTERFACE FILE_SET headers TYPE HEADERS FILES ${headers})
target_link_libraries(audio_test_headers INTERFACE Dius::dius Di::di)

file(GLOB sources CONFIGURE_DEPENDS "*.cpp")

find_path(PIPEWIRE_INCLUDE_DIRS pipewire-0.3/pipewire/pipewire.h)
find_path(SPA_INCLUDE_DIRS spa-0.2/spa/param/props.h)
find_library(PIPEWIRE_LIBRARIES NAMES pipewire-0.3)

add_executable(audio_test ${sources})
target_link_libraries(audio_test PUBLIC Dius::dius Di::di)

if(NOT PIPEWIRE_INCLUDE_DIRS
   OR NOT SPA_INCLUDE_DIRS
   OR NOT PIPEWIRE_LIBRARIES
)
    message(FATAL_ERROR "Pipewire not found")
endif()

target_include_directories(audio_test SYSTEM PUBLIC ${PIPEWIRE_INCLUDE_DIRS}/pipewire-0.3 ${SPA_INCLUDE_DIRS}/spa-0.2)
target_link_libraries(audio_test PUBLIC ${PIPEWIRE_LIBRARIES})

install(TARGETS audio_test RUNTIME DESTINATION bin)
