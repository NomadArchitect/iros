# Build Instructions

## Using provided dev container

The easiest way to build and run Iros is to use the provided dev container. The CI pipeline for the repository builds
and tests the toolchain image, which ensures everything will work inside the dev container.

To use the dev container, use Visual Studio Code and install the remote contains extension. VS Code will then prompt you
to open the repository inside the dev container. This will also automatically install relevant VS Code extensions, which
enables building and running tests through the VS Code CMake extension.

## Build the toolchain locally

In order to use something other than VS Code, it is probably easiest to just build the toolchain locally. Once finished,
the project can be built like a normal CMake project, and intellisense can be provided using `compile_commands.json` and
any c++ language server. Currently, your best bet is to use `clangd`, as the project uses bleeding-edge c++ features
which only `clang` and `gcc` support. The Microsoft C++ engine is not recommended, as it cannot compile the project's
source code.

In addition, the project requires at least GCC 14.1.0 or clang 18 and CMake 3.25.2 to compile. The CMake version can
probably be relaxed, but GCC 13 will fail to compile the system. Likewise, clang-17 will not compile the system.
Currently, the clang support is experimental, since there is not a custom toolchain target for llvm. Testing the kernel
additionally requires various system commands, including `parted`, `mkfs.fat`, and `qemu`. A full list of packages
needed under Ubuntu can be found in the
[dockerfile](https://github.com/ColeTrammer/iros/tree/iris/meta/docker/Dockerfile). This dockerfile also provides steps
which install the latest version of CMake and clang on Ubuntu. For other Linux distros, you will have to look up the
corresponding package names for your distro. At the very least, this list of dependencies is actively tested in CI, and
so will always be up to date.

To build the project documentation, you must have the latest version of Doxygen installed, and Graphviz is needed to
generate the various diagrams. If Doxygen is not installed, the `docs` target will not exist in the generated build
system.

### Using the Nix shell

If you prefer, you can use the provided nix shell which gives you access to all required and optional dependencies.
This requires installing [Nix](https://nixos.org/download/). To access the shell simply run:

```sh
# NOTE: you will have to enable the flake and nix-command experimental features.
nix develop
```

Additionally, the development environment can be loaded automatically when inside the project directory using
[direnv](https://direnv.net/). After install, simply run:

```sh
direnv allow .
```

### Justfile

To simplify running build commands, the project ships with a `justfile`. To use it, either use the nix shell
or install [just](https://github.com/casey/just) manually.

The main advantage of the just file is that it has a default CMake presets which can also be overridden by the
`PRESET` environment variable. This makes building and testing the project significantly easier. The following
build steps showcase how to perform common actions using the `justfile`, although the raw commands are also shown.

### VS Code

The project includes a `tests.json` and `launch.json` which should enable building the project through the
VS Code GUI and well as debugging it. This requires the `cmake-tools` extension and actually uses it
to determine the current `CMake` preset. This is entirely optional, as you can simply use the `justfile`
instead to run project commands.

### Build Steps

Build the toolchain.

```sh
./meta/toolchain/build.sh

# Or, using the justfile
just build_toolchain
```

Add `cross/bin` to your path. This is only needed when configuring the build for the first time.

```sh
# This is done automatically by direnv if you choose to use it.
export PATH="$(realpath .)/cross/bin:$PATH"
```

At this point, the entire system should be buildable with CMake.

### Build Commands

Note that these commands apply using a dev container or locally, once things are setup. To use clang instead, use the
`clang_iros_x86_64_release_default` preset instead.

#### Configure

```sh
cmake --preset gcc_iros_x86_64_release_default

# Or, using the justfile
just ic
```

#### Build

```sh
cmake --build --preset gcc_iros_x86_64_release_default

# Or, using the justfile
just ib
```

#### Run Tests

```sh
ctest --preset gcc_iros_x86_64_release_default

# Or, using the justfile
just it
```

#### Run the kernel directly

```sh
cmake --build --preset gcc_iros_x86_64_release_default --target ibr

# Or, using the justfile
just run
```

#### Build Documentation

This outputs the viewable documentation to `build/x86_64/gcc/release/default/html`. This can be viewed using the VS Code
Live Preview or by pointing a web browser at this directory.

```sh
cmake --build --preset gcc_iros_x86_64_release_default --target docs

# Or, using the justfile
# This also opens the documentation for you using `xdg-open`
just docs
```

### Linux Presets

Additionally, presets are defined for compiling the userland libraries and their unit tests on a Linux system. A list of
presets should be displayed by the IDE, or can discovered in the `CMakePresets.json` file. These can be useful for
debugging purposes, especially because we can use sanitizers directly. For instance, the `gcc_debug_ubasan` preset
compiles the userspace code with both `ubsan` and `asan` enabled. This configuration is actively tested in CI.

These can be built and used without any special setup, using normal CMake commands, although recent versions of GCC or
clang and CMake are needed.

## Build Directories

The CMake presets assign separate build directories for each target. Each is located in a subdirectory of `build`. The
build directories are named after the target and the preset used to build it. For instance, the
`gcc_iros_x86_64_release_default` preset corresponds to a build directory of `build/x86_64/gcc/release/default`. The
host tools required to build the kernel are located in `build/host/gcc/release/tools`. All x86_64 builds share the same
install directory, which is used as the system root for the disk image. This is located in `build/x86_64/sysroot`.

## Running the Kernel

The kernel can be run using QEMU. Currently, the kernel does not support graphical output, and so it must be run in a
terminal. The kernel uses the serial console to do input and output. Since there is also need for the QEMU debug
console, this mode uses QEMU's chardev multiplexer. This means that exiting the kernel must be done using the special
QEMU command: `Ctrl-D X`. This will exit the QEMU debug console, and then the kernel will exit. To get to the QEMU debug
console, use `Ctrl-D C`. Then the kernel can be inspected using normal QEMU commands.

### Debugging the Kernel

The kernel can be debugged using GDB or LLDB. This requires passing `IROS_DEBUG=1` to the run script, which is done
automatically when using the VSCode launch configurations. These start QEMU in debug mode and then attach to the QEMU
socket. This requires using the `debug_iros_x86_64` preset, or else symbols and breakpoint information will not be
available. Additionally, debugging is mostly useful for boot issues, as there is no way to debug userspace programs and
once the kernel scheduler is running, the debugger interferes with the timer interrupts.
