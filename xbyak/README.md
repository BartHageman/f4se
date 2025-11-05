# Xbyak (pronounced kai-byak)

Vendored dependency of most xSE projects. See [this repository](https://github.com/herumi/xbyak) for further information.

A JIT assembler for x86/x64 architectures supporting advanced instruction sets up to AVX10.2.

Xbyak is a C++ header-only library that enables dynamic assembly of x86/x64 instructions using mnemonics.

Features:

  - Header-only library
  - Intel/MASM-like syntax
  - Full support for AVX-512, APX, and AVX10.2

Note: Use and_(), or_(), ... instead of and(), or(). If you want to use them, then specify -fno-operator-names option to gcc/clang.
