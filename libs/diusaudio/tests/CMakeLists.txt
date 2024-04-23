file(GLOB sources CONFIGURE_DEPENDS *.cpp)
add_executable(test_diusaudio ${sources})

find_package(Dius REQUIRED)

target_link_libraries(test_diusaudio PRIVATE Diusaudio::diusaudio Dius::dius_test_main)

add_test(NAME test_diusaudio COMMAND test_diusaudio)
