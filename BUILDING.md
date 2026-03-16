# Building BS2CE

This document describes how to compile BS2CE from source on Windows and Linux.

---

## Requirements

| Tool | Version |
|------|---------|
| CMake | 3.16 or newer |
| Clang / Clang++ | Any recent version |
| Ninja | Any recent version |
| SDL | 1.2 |
| SDL_net | 1.2 |
| SDL_ttf | 1.2 |
| zlib | Any recent version |
| libpng | Any recent version |

---

## Windows

Windows builds use **MSYS2** with the **CLANG64** environment. Do **not** use the MINGW64, UCRT64, or MSYS shells — only CLANG64 is supported.

### 1. Install MSYS2

Download and install MSYS2 from https://www.msys2.org. The default install path is `C:\msys64`.

### 2. Open the CLANG64 Shell

Launch **MSYS2 CLANG64** from the Start menu (not the generic MSYS2 terminal).

### 3. Install Dependencies

Run the following in the CLANG64 shell:

```bash
pacman -Syu
pacman -S git mingw-w64-clang-x86_64-{clang,cmake,ninja,SDL,SDL_net,SDL_ttf,zlib,libpng}
```

### 4. Clone the Repository

```bash
git clone https://github.com/yourname/BS2CE.git
cd BS2CE
```

### 5. Configure and Build

```bash
cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### 6. Run

```bash
./build/bin/BS2Plus.exe
```

The build system automatically copies all required DLLs and asset folders (`img/`, `bs2/`, `misc/`) into `build/bin/` alongside the executable.

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

### 2. Clone the Repository

```bash
git clone https://github.com/yourname/BS2CE.git
cd BS2CE
```

### 3. Configure and Build

```bash
cmake -B build -S . -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++
cmake --build build
```

### 4. Run

```bash
./build/bin/BS2Plus
```

---

## Build Types

Pass `-DCMAKE_BUILD_TYPE=<type>` during configuration:

| Type | Description |
|------|-------------|
| `Release` | Fully optimized, no debug info — recommended for distribution |
| `Debug` | No optimization, full debug symbols |
| `RelWithDebInfo` | Optimized with debug symbols |

---

## Project Structure

```
BS2CE/
├── CMakeLists.txt
├── BUILDING.md
├── include/          # Header files
├── src/              # Source files
└── Resources/
    ├── img/          # Images (copied to build/bin/img/)
    ├── bs2/          # Game data (copied to build/bin/)
    └── misc/         # Misc assets (copied to build/bin/)
```

---

## Troubleshooting

**`SDL not found` on Windows**
Make sure you are running CMake from the **CLANG64** shell, not a regular Command Prompt or PowerShell.

**Missing DLL errors on Windows**
All required DLLs are copied automatically from `C:\msys64\clang64\bin` after build. If a DLL is still missing, check it exists in that folder or reinstall the relevant pacman package.

**Implicit template / missing header errors on Linux**
Clang on Linux requires all standard headers to be explicitly included. Ensure files that use `std::vector`, `std::array`, `std::string`, etc. have the corresponding `#include` directive.

**`ldd` — inspect runtime dependencies (Linux / MSYS2)**
```bash
ldd build/bin/BS2Plus        # Linux
ldd build/bin/BS2Plus.exe    # MSYS2 CLANG64 shell
```
