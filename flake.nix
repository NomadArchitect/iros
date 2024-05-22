{
  description = "An Operating System focused on asynchronicity, minimalism, and performance.";

  inputs = {
    flake-parts.url = "github:hercules-ci/flake-parts";
    flake-root.url = "github:srid/flake-root";
    treefmt-nix.url = "github:numtide/treefmt-nix";
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
  };

  outputs = inputs @ {flake-parts, ...}:
    flake-parts.lib.mkFlake {inherit inputs;} {
      imports = [
        inputs.treefmt-nix.flakeModule
        inputs.flake-root.flakeModule
      ];
      systems = ["x86_64-linux"];
      perSystem = {
        config,
        pkgs,
        system,
        ...
      }: {
        treefmt = {
          inherit (config.flake-root) projectRootFile;

          programs = {
            alejandra.enable = true;
            clang-format = {
              enable = true;
              package = pkgs.clang-tools_18;
            };
            prettier.enable = true;
            shfmt = {
              enable = true;
              indent_size = 4;
            };
            just.enable = true;
          };

          settings.formatter.cmake-format = {
            command = "${pkgs.cmake-format}/bin/cmake-format";
            options = ["-i"];
            includes = ["CMakeLists.txt" "CMakeToolchain.*.txt" "*.cmake"];
          };
        };

        devShells.default = let
          gccVersion = "14";
          llvmVersion = "18";
        in
          pkgs.mkShell.override {stdenv = pkgs."gcc${gccVersion}Stdenv";} {
            packages =
              [
                config.treefmt.build.wrapper
                pkgs.cmake-format
              ]
              ++ builtins.attrValues config.treefmt.build.programs
              ++ [
                pkgs.nil
                pkgs."clang-tools_${llvmVersion}"
                pkgs.neocmakelsp
                pkgs.marksman
                pkgs.markdownlint-cli
                pkgs.dockerfile-language-server-nodejs
                pkgs.yaml-language-server
                pkgs.hadolint
                pkgs.jq
                pkgs.just
                (pkgs.writeShellScriptBin
                  "vscode-json-language-server"
                  ''${pkgs.nodePackages_latest.vscode-json-languageserver}/bin/vscode-json-languageserver "$@"'')
              ]
              ++ [
                pkgs."clang_${llvmVersion}"
                pkgs.cmake
                pkgs.ninja
                pkgs.bison
                pkgs.flex
                pkgs.doxygen
                pkgs.graphviz
                pkgs.mpfr
                pkgs.gmp
                pkgs.libmpc
                pkgs.autoconf269
                pkgs.automake115x
                pkgs.qemu
                pkgs.gdb
                pkgs.fzf
                pkgs."lldb_${llvmVersion}"
                pkgs.ccache
                pkgs.parted
                pkgs.gcovr
                pkgs.pipewire
                pkgs.wayland-scanner
                pkgs.wayland
                pkgs.valgrind
              ];

            hardeningDisable = ["format"];
          };
      };
    };
}
