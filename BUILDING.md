# Building BS2CE

This document describes how to compile BS2CE from source on Windows and Linux.

## Windows

Windows builds use **MSYS2** with the **CLANG64** environment, only CLANG64 is supported.

### 1. Installing MSYS2

Download the latest version of [MSYS2](https://www.msys2.org) and install.
After installation, a terminal opens and follow the command to update the environment:
```bash
pacman -Syu
```

When you the terminal asks you to shut down the MSYS2 after finishing the update, Proceed.
Then start the "MSYS2 CLANG64" environment from the start menu.

```bash
pacman -S git mingw-w64-clang-x86_64-{clang,cmake,ninja,SDL,SDL_net,SDL_ttf,zlib,libpng}
```

### 2. Building

Navigate somewhere you want to clone source code to and clone the source repo using Git.
Before using Git, Download the latest version of [Git](https://git-scm.com/install/) and install.
```bash
git clone https://github.com/system32unknown/BS2CE
cd BS2CE
```

Then generate the build files using Cmake and start the compile.
```bash
cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```
When finished, there will be a "BS2CE.exe" file in the build/bin folder.

### Custom MSYS2 Path

If MSYS2 is installed somewhere other than `C:\msys64`, pass the prefix manually:

```bash
cmake -B build -S . -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DMSYS2_PREFIX=D:/msys64/clang64
```

---

## Linux

### 1. Install Dependencies

**Ubuntu / Debian:**
```bash
sudo apt-get update
sudo apt-get install -y \
  clang \
  cmake \
  ninja-build \
  libsdl1.2-dev \
  libsdl-net1.2-dev \
  libsdl-ttf2.0-dev \
  zlib1g-dev \
  libpng-dev
```

**Fedora / RHEL:**
```bash
sudo dnf install -y \
  clang \
  cmake \
  ninja-build \
  SDL-devel \
  SDL_net-devel \
  SDL_ttf-devel \
  zlib-devel \
  libpng-devel
```

**Arch Linux:**
```bash
sudo pacman -S \
  clang \
  cmake \
  ninja \
  sdl12-compat \
  sdl_net \
  sdl_ttf \
  zlib \
  libpng
```

### 2. Building

Navigate somewhere you want to clone source code to and clone the source repo using Git.
Before using Git, Download the latest version of (Git)[https://git-scm.com/install/] and install.
```bash
git clone https://github.com/system32unknown/BS2CE
cd BS2CE
```

Then generate the build files using Cmake and start the compile.
```bash
cmake -B build -S . -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++
cmake --build build
```
When finished, there will be a "BS2CE.exe" file in the build/bin folder.

---

## Build Types

Pass `-DCMAKE_BUILD_TYPE=<type>` during configuration:

| Type | Description |
|------|-------------|
| `Release` | Fully optimized, no debug info — recommended for distribution |
| `Debug` | No optimization, full debug symbols |
| `RelWithDebInfo` | Optimized with debug symbols |

---

## Troubleshooting

**`SDL not found` on Windows**
Make sure you are running CMake from the **CLANG64** shell, not a regular Command Prompt or PowerShell.

**Missing DLL errors on Windows**
All required DLLs are copied automatically from `C:\msys64\clang64\bin` after build. If a DLL is still missing, check it exists in that folder or reinstall the relevant pacman package.

**Implicit template / missing header errors on Linux**
Clang on Linux requires all standard headers to be explicitly included. Ensure files that use `std::vector`, `std::array`, `std::string`, etc. have the corresponding `#include` directive.