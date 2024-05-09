preset := env("PRESET", "clang_release_default")
iros_preset := if preset =~ 'iros' { preset } else { "gcc_iros_x86_64_release_default" }
test := "^test_di$"
default_iros_test := "^test_iris$"

# Default command: configure and build
default:
    just cb

# Configure
config *args="":
    cmake --preset {{ preset }} {{ args }}

# Build
build *args="":
    cmake --build --preset {{ preset }} {{ args }}

# Run tests
test *args="":
    ctest --preset {{ preset }} {{ args }}

# Run a specific test (regex matching)
testonly name=test:
    ctest --preset {{ preset }} -R {{ name }}

# Configure and build
cb:
    just config
    just build

# Build and test
bt:
    just build
    just test

# Configure and build and test
cbt:
    just config
    just bt

# Build and run a specific test (regex matching)
btonly name=test:
    just build
    just testonly {{ name }}

# Configure the build system for Iros
ic:
    cmake --preset {{ iros_preset }}

# Build Iros disk image
image:
    cmake --build --preset {{ iros_preset }} -t image

# Run Iros
run:
    cmake --build --preset {{ iros_preset }} -t run

# Full build Iros and produce disk image
ib:
    cmake --build --preset {{ iros_preset }} -t ib

# Full build Iros and run
ibr:
    cmake --build --preset {{ iros_preset }} -t ibr

# Run tests on Iros
it:
    ctest --preset {{ iros_preset }}

# Run a specific test on Iros (regex matching)
itonly name=default_iros_test:
    ctest --preset {{ iros_preset }} -R {{ name }}

# Full build Iros and run tests
ibt:
    just ib
    just it

# Full build Iros and run a specific test (regex matching)
ibtonly name=default_iros_test:
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
