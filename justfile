preset := env("PRESET", "clang_release_default")
iros_preset := if preset =~ 'iros' { preset } else { "gcc_iros_x86_64_release_default" }
test := "^test_di$"
default_iros_test := "^test_iris$"

alias c := configure
alias b := build
alias t := test
alias tonly := test_only
alias cb := configure_build
alias bt := build_test
alias cbt := configure_build_test
alias btonly := build_test_only
alias bf := build_file
alias bonly := build_target
alias r := run
alias br := build_run

# Default command: configure and build
default:
    just configure_build

# Configure
configure *args="":
    cmake --preset {{ preset }} {{ args }}

# Build
build *args="":
    cmake --build --preset {{ preset }} {{ args }}

# Run tests
test *args="":
    ctest --preset {{ preset }} {{ args }}

# Run a specific test (regex matching)
test_only name=test:
    ctest --preset {{ preset }} -R {{ name }}

# Configure and build
configure_build:
    just configure
    just build

# Build and test
build_test:
    just build
    just test

# Configure and build and test
configure_build_test:
    just config
    just bt

# Build and run a specific test (regex matching)
build_test_only name=test:
    just build
    just test_only {{ name }}

# Compile a specific file (regex matching)
build_file name:
    #!/usr/bin/env bash
    set -euxo pipefail

    targets=$(cmake --build --preset {{ preset }}_non_unity -- -t targets all \
        | cut -d ' ' -f 1 \
        | tr -d '[:]' \
        | grep -E "{{ name }}" \
    )
    cmake --build --preset {{ preset }}_non_unity -t ${targets}

# Build a specific target (regex matching)
build_target name:
    #!/usr/bin/env bash
    set -euxo pipefail

    targets=$(cmake --build --preset {{ preset }} -- -t targets all \
        | cut -d ' ' -f 1 \
        | tr -d '[:]' \
        | grep -E "{{ name }}" \
        | grep -vF '.cxx.o' \
        | grep -vF 'cmake_object_order' \
        | grep -vF 'CMakeFiles' \
        | grep -vF 'CMakeLists.txt' \
        | grep -vF '/install' \
        | grep -vF 'verify_interface_header_sets' \
        | grep -vF '/edit_cache' \
        | grep -vF '/rebuild_cache' \
        | grep -vF '/list_install_components' \
        | grep -vE '/all$' \
        | grep -vE '/test$' \
        | grep -E '/' \
    )
    cmake --build --preset {{ preset }} -t $targets

# Run a specific program (regex matching)
run name *args:
    #!/usr/bin/env bash
    set -euxo pipefail

    targets=$(cmake --build --preset {{ preset }} -- -t targets all \
        | cut -d ' ' -f 1 \
        | tr -d '[:]' \
        | grep -E "{{ name }}" \
        | grep -vF '.cxx.o' \
        | grep -vF 'cmake_object_order' \
        | grep -vF 'CMakeFiles' \
        | grep -vF 'CMakeLists.txt' \
        | grep -vF '/install' \
        | grep -vF 'verify_interface_header_sets' \
        | grep -vF '/edit_cache' \
        | grep -vF '/rebuild_cache' \
        | grep -vF '/list_install_components' \
        | grep -vE '/all$' \
        | grep -vE '/test$' \
        | grep -E '/' \
    )
    build_directory=$( \
        jq -rc '.configurePresets.[] | select(.name == "{{ preset }}") | .binaryDir' < CMakePresets.json | \
        sed s/\${sourceDir}/./g \
    )
    for target in $targets; do
        $build_directory/$target {{ args }}
    done

# Build and run a specific program (regex matching)
build_run name *args:
    just build_target {{ name }}
    just run {{ name }} {{ args }}

alias ic := iros_configure
alias ibimg := iros_build_image
alias ir := iros_run
alias ib := iros_build
alias ibr := iros_build_run
alias it := iros_test
alias itonly := iros_test_only
alias ibt := iros_build_test
alias ibtonly := iros_build_test_only

# Configure the build system for Iros
iros_configure:
    cmake --preset {{ iros_preset }}

# Build Iros disk image
iros_build_image:
    cmake --build --preset {{ iros_preset }} -t image

# Run Iros
iros_run:
    cmake --build --preset {{ iros_preset }} -t run

# Full build Iros and produce disk image
iros_build:
    cmake --build --preset {{ iros_preset }}

# Full build Iros and run
iros_build_run:
    cmake --build --preset {{ iros_preset }} -t ibr

# Run tests on Iros
iros_test:
    ctest --preset {{ iros_preset }}

# Run a specific test on Iros (regex matching)
iros_test_only name=default_iros_test:
    ctest --preset {{ iros_preset }} -R {{ name }}

# Full build Iros and run tests
iros_build_test:
    just ib
    just it

# Full build Iros and run a specific test (regex matching)
iros_build_test_only name=default_iros_test:
    just ib
    just itonly {{ name }}

# Build Iros cross compiler
build_toolchain:
    ./meta/toolchain/build.sh

# Build Iros toolchain docker image
build_docker:
    docker build -t ghcr.io/coletrammer/iros_toolchain:iris . -f meta/docker/Dockerfile

# Auto-format source code
format:
    nix fmt

# Validate format and lint code
check:
    nix flake check

# Build docs
build_docs:
    cmake --build --preset {{ preset }} --target docs

# Open docs
open_docs:
    cmake --build --preset {{ preset }} --target open_docs

# Build and open docs
docs:
    just build_docs
    just open_docs
