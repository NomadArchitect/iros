if(CMAKE_CROSSCOMPILING)
    include(ExternalProject)

    ExternalProject_Add(
        native
        SOURCE_DIR "${CMAKE_SOURCE_DIR}"
        TMP_DIR "${CMAKE_BINARY_DIR}/host-tools/tmp"
        STAMP_DIR "${CMAKE_BINARY_DIR}/host-tools/stamp"
        BINARY_DIR "${CMAKE_SOURCE_DIR}/build/host/gcc/release/tools"
        INSTALL_DIR "${CMAKE_SOURCE_DIR}/build/host/gcc/release/tools/install"
        CMAKE_ARGS "--preset=gcc_release_tools" "-DIROS_NeverLinkCompileCommands=ON"
        CMAKE_CACHE_ARGS "-DCMAKE_INSTALL_PREFIX:STRING=<INSTALL_DIR>"
        BUILD_ALWAYS YES
        STEP_TARGETS install
    )

    set(INITRD_COMMAND "${CMAKE_SOURCE_DIR}/build/host/gcc/release/tools/install/bin/initrd")
    set(INITRD_TARGET native-install)
else()
    set(INITRD_COMMAND initrd)
    set(INITRD_TARGET initrd)
endif()

add_custom_target(
    generate-initrd
    COMMAND mkdir -p "${CMAKE_BINARY_DIR}/initrd"
    COMMAND cd "${CMAKE_BINARY_DIR}/initrd"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/iris/test_userspace" .
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/iris/test_create_task" .
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/iris/test_read" .
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/userland/shell/sh" .
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/userland/tools/initrd" .
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/userland/core/ls" .
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/userland/core/cp" .
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/userland/audiotest/audio_test" .
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/iris/data.txt" .
    COMMAND mkdir -p "tmp"
    COMMAND rm -f "${CMAKE_BINARY_DIR}/initrd/initrd.bin"
    COMMAND ${INITRD_COMMAND}
    DEPENDS ${INITRD_TARGET}
            initrd
            ls
            cp
            sh
            test_userspace
            test_create_task
            test_read
            audio_test
)

add_custom_target(
    image
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMAND sudo IROS_ROOT="${CMAKE_SOURCE_DIR}" IROS_BUILD_DIR="${CMAKE_BINARY_DIR}" IROS_LIMINE_DIR="${LIMINE_DIR}"
            REMOTE_CONTAINERS="$ENV{REMOTE_CONTAINERS}" "${CMAKE_SOURCE_DIR}/meta/make-iris-limine-image.sh"
    BYPRODUCTS "${CMAKE_BINARY_DIR}/iris/iris.img"
    DEPENDS generate-initrd iris
    USES_TERMINAL
)

add_custom_target(
    run
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    COMMAND IROS_ARCH=${CMAKE_HOST_SYSTEM_PROCESSOR} IROS_IMAGE=${CMAKE_BINARY_DIR}/iris/iris.img
            "${CMAKE_SOURCE_DIR}/meta/run-iris.sh"
    USES_TERMINAL
)

add_custom_target(
    ib
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    DEPENDS generate-initrd image
)

add_custom_target(
    ibr
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    COMMAND IROS_ARCH=${CMAKE_HOST_SYSTEM_PROCESSOR} IROS_IMAGE=${CMAKE_BINARY_DIR}/iris/iris.img
            "${CMAKE_SOURCE_DIR}/meta/run-iris.sh"
    DEPENDS generate-initrd image
    USES_TERMINAL
)
