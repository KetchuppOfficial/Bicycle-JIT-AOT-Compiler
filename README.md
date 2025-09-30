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

### 2) Build the project

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Simple IR test

Run executable:

```bash
build/fibonacci-test
```

You will see the textual representation of the IR of a function computing n'th fibonacci number:

```text
i64 fibonacci(i64):
%bb0: preds:
    %0x43b510 = arg 0
    %0x43b550 = constant i64 2
    %0x43b590 = icmp ult 0x43b510, 0x43b550
    br %0x43b590, label %bb3, label %bb1
%bb1: preds: %bb0
    %0x43b690 = constant i64 0
    %0x43b6d0 = constant i64 1
    br %bb2
%bb2: preds: %bb1, %bb2
    %0x43b790 = phi [%0x43b930, %bb2], [%0x43b550, %bb1]
    %0x43b800 = phi [%0x43b8e0, %bb2], [%0x43b6d0, %bb1]
    %0x43b870 = phi [%0x43b800, %bb2], [%0x43b690, %bb1]
    %0x43b8e0 = add %0x43b870 %0x43b800
    %0x43b930 = add %0x43b790 %0x43b6d0
    %0x43b980 = icmp ule 0x43b790, 0x43b510
    br %0x43b980, label %bb2, label %bb3
%bb3: preds: %bb0, %bb2
    %0x43bc90 = phi [%0x43b800, %bb2], [%0x43b510, %bb0]
    ret %0x43bc90
```
