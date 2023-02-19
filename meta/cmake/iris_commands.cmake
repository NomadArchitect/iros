add_custom_target(
    generate-initrd
    COMMAND mkdir -p "${CMAKE_CURRENT_BINARY_DIR}/initrd"
    COMMAND cd "${CMAKE_CURRENT_BINARY_DIR}/initrd"
    COMMAND cp "${CMAKE_CURRENT_BINARY_DIR}/iris/test_userspace" .
    COMMAND rm -f "${CMAKE_CURRENT_BINARY_DIR}/initrd/initrd.bin"
    COMMAND initrd
    DEPENDS initrd
)

add_custom_target(
    image
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND sudo IROS_ROOT="${CMAKE_CURRENT_SOURCE_DIR}"
            IROS_BUILD_DIR="${CMAKE_CURRENT_BINARY_DIR}"
            IROS_LIMINE_DIR="${LIMINE_DIR}"
            "${CMAKE_CURRENT_SOURCE_DIR}/meta/make-iris-limine-image.sh"
    BYPRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/iris/iris.img"
    DEPENDS iris
    USES_TERMINAL
)

add_custom_target(
    run
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMAND IRIS_ARCH=${CMAKE_HOST_SYSTEM_PROCESSOR}
            IRIS_IMAGE=${CMAKE_CURRENT_BINARY_DIR}/iris/iris.img
            "${CMAKE_CURRENT_SOURCE_DIR}/meta/run-iris.sh"
    USES_TERMINAL
)

add_custom_target(
    ibr
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMAND IRIS_ARCH=${CMAKE_HOST_SYSTEM_PROCESSOR}
            IRIS_IMAGE=${CMAKE_CURRENT_BINARY_DIR}/iris/iris.img
            "${CMAKE_CURRENT_SOURCE_DIR}/meta/run-iris.sh"
    DEPENDS image generate-initrd
    USES_TERMINAL
)