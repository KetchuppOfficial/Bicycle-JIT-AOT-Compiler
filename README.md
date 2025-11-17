# BJAC -- Bicycle JIT and AOT Compiler

## How to clone the repository

```bash
git clone https://github.com/KetchuppOfficial/Bicycle-JIT-AOT-Compiler.git
```

## How to build

### 0) Requirements

Installed nix package manager (can be install with apt on debian-like distributives).

### 1) Create development environment

```bash
nix develop
```

You may need to pass additional options to nix in case you do not have corresponding features set
in nix.conf.

```bash
nix develop --extra-experimental-features nix-command --extra-experimental-features flakes
```

### 2) Configure build system

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
```

List of cmake options defined by the project:

| Option             | Values     | Explanation                     | Default |
|--------------------|------------|---------------------------------|---------|
| `BJAC_BUILD_TESTS` | `ON`/`OFF` | Set to `ON` to build unit tests |  `OFF`  |

### 3) Build the project

```bash
cmake --build build
```

## Run unit tests

```bash
ctest --test-dir build
```

## Simple IR test

Run executable (`BJAC_BUILD_TESTS` has to be `ON`):

```bash
build/test/IR/fibonacci_test
```

You will see the textual representation of the IR of a function computing n'th fibonacci number:

```text
i64 fibonacci("i64")
%bb0:
    %0.0 = i64 arg [0]
    %0.1 = i64 constant 2
    %0.2 = icmp ult i64 %0.0, %0.1
    br i1 %0.2, label %bb3, label %bb1
%bb1: ; preds: %bb0
    %1.0 = i64 constant 0
    %1.1 = i64 constant 1
    br label %bb2
%bb2: ; preds: %bb1, %bb2
    %2.0 = phi i64 [%0.1, %bb1], [%2.4, %bb2]
    %2.1 = phi i64 [%1.1, %bb1], [%2.3, %bb2]
    %2.2 = phi i64 [%1.0, %bb1], [%2.1, %bb2]
    %2.3 = i64 add %2.2, %2.1
    %2.4 = i64 add %2.0, %1.1
    %2.5 = icmp ule i64 %2.0, %0.0
    br i1 %2.5, label %bb2, label %bb3
%bb3: ; preds: %bb0, %bb2
    %3.0 = phi i64 [%0.0, %bb0], [%2.1, %bb2]
    ret i64 %3.0
```
