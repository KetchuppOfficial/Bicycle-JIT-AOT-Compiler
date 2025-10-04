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
i64 fibonacci("i64")
%bb0:
    %0x439510 = arg i64 [0]
    %0x439550 = constant i64 2
    %0x4395a0 = icmp ult i64 %0x439510, %0x439550
    br i1 %0x4395a0, label %bb3, label %bb1
%bb1: preds: %bb0
    %0x4396b0 = constant i64 0
    %0x439700 = constant i64 1
    br label %bb2
%bb2: preds: %bb1, %bb2
    %0x4397e0 = phi i64 [%0x439550, %bb1], [%0x439980, %bb2]
    %0x439850 = phi i64 [%0x439700, %bb1], [%0x439930, %bb2]
    %0x4398c0 = phi i64 [%0x4396b0, %bb1], [%0x439850, %bb2]
    %0x439930 = i64 add %0x4398c0, %0x439850
    %0x439980 = i64 add %0x4397e0, %0x439700
    %0x4399d0 = icmp ule i64 %0x4397e0, %0x439510
    br i1 %0x4399d0, label %bb2, label %bb3
%bb3: preds: %bb0, %bb2
    %0x439c60 = phi i64 [%0x439510, %bb0], [%0x439850, %bb2]
    ret i64 %0x439c60
```
