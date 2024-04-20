file(GLOB sources CONFIGURE_DEPENDS *.cpp)
add_executable(test_diusgfx ${sources})

find_package(Dius REQUIRED)

target_link_libraries(test_diusgfx PRIVATE Diusgfx::diusgfx Dius::dius_test_main)

add_test(NAME test_diusgfx COMMAND test_diusgfx)
