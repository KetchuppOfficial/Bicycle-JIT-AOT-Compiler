{
  description = "Tasks on course in JIT and AOT compilers in virtual machines";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
  };

  outputs =
    {
      nixpkgs,
      ...
    }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs {
        inherit system;
      };
      native_build_inputs = [
        pkgs.cmake
        pkgs.ninja
        pkgs.nixfmt
      ];
    in
    {
      devShells."${system}" = {
        default = (pkgs.mkShell.override { stdenv = pkgs.gcc15Stdenv; }) {
          buildInputs = [
            pkgs.gtest
          ];
          nativeBuildInputs = native_build_inputs;
        };
        llvm =
          let
            llvm_packages = pkgs.llvmPackages_22;
            llvm_stdenv = llvm_packages.stdenv;
          in
          (pkgs.mkShell.override { stdenv = llvm_stdenv; }) {
            buildInputs = [
              (pkgs.gtest.override { stdenv = llvm_stdenv; })
            ];
            nativeBuildInputs = native_build_inputs ++ [
              llvm_packages.clang-tools
            ];
            shellHook =
              let
                vscodeDir = ".vscode";
                vscodeConfig = {
                  "clangd.path" = "${pkgs.clang-tools}/bin/clangd";
                };
              in
              ''
                mkdir -p ${vscodeDir}
                jq --indent 4 -n '${builtins.toJSON vscodeConfig}' > ${vscodeDir}/settings.json
              '';
          };
      };
    };
}
