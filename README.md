# BJAC -- Bicycle JIT and AOT Compiler

## How to clone the repository

```bash
git clone https://github.com/KetchuppOfficial/Bicycle-JIT-AOT-Compiler.git
```

## How to build

### 0) Requirements

Installed nix package manager.

### 1) Create development environment

```bash
nix develop
```

You may need to pass additional options to nix in case you do not have corresponding features set
in nix.conf.

```bash
nix develop --extra-experimental-features nix-command --extra-experimental-features flakes
```

### 2) Build the project

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```
