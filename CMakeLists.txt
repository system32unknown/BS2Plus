cmake_minimum_required(VERSION 3.16)
project(BS2Plus VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ─────────────────────────────────────────────
#  MSYS2 CLANG64 Toolchain Guard
# ─────────────────────────────────────────────
if(WIN32)
    # Detect MSYS2 CLANG64 environment
    if(NOT DEFINED ENV{MSYSTEM} OR NOT "$ENV{MSYSTEM}" STREQUAL "CLANG64")
        message(WARNING
            "This CMakeLists is designed for MSYS2 CLANG64.\n"
            "Please run from the CLANG64 shell and configure with:\n"
            "  cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Release"
        )
    endif()

    # MSYS2 CLANG64 root prefix
    set(MSYS2_PREFIX "C:/msys64/clang64" CACHE PATH "MSYS2 CLANG64 prefix")

    # Force Clang toolchain from MSYS2
    if(NOT CMAKE_C_COMPILER)
        set(CMAKE_C_COMPILER   "${MSYS2_PREFIX}/bin/clang.exe")
    endif()
    if(NOT CMAKE_CXX_COMPILER)
        set(CMAKE_CXX_COMPILER "${MSYS2_PREFIX}/bin/clang++.exe")
    endif()
endif()

# ─────────────────────────────────────────────
#  Output Directories
# ─────────────────────────────────────────────
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ─────────────────────────────────────────────
#  MSYS2 CLANG64 Package Search Paths
#  All libs installed via: pacman -S mingw-w64-clang-x86_64-<pkg>
# ─────────────────────────────────────────────
if(WIN32 AND DEFINED MSYS2_PREFIX)
    list(PREPEND CMAKE_PREFIX_PATH "${MSYS2_PREFIX}")
    list(PREPEND CMAKE_LIBRARY_PATH "${MSYS2_PREFIX}/lib")
    list(PREPEND CMAKE_INCLUDE_PATH "${MSYS2_PREFIX}/include")
endif()

# ─────────────────────────────────────────────
#  Find SDL 1.2
# ─────────────────────────────────────────────
find_path(SDL_INCLUDE_DIR SDL/SDL.h
    HINTS "${MSYS2_PREFIX}/include" /usr/include /usr/local/include
)
find_library(SDL_LIBRARY NAMES SDL SDL
    HINTS "${MSYS2_PREFIX}/lib" /usr/lib /usr/local/lib
)
find_library(SDLMAIN_LIBRARY NAMES SDLmain
    HINTS "${MSYS2_PREFIX}/lib" /usr/lib /usr/local/lib
)

if(NOT SDL_LIBRARY OR NOT SDL_INCLUDE_DIR)
    message(FATAL_ERROR
        "SDL 1.2 not found.\n"
        "Run: pacman -S mingw-w64-clang-x86_64-SDL"
    )
endif()
message(STATUS "Found SDL 1.2: ${SDL_LIBRARY}")

# ─────────────────────────────────────────────
#  Find SDL_net 1.2
# ─────────────────────────────────────────────
find_path(SDL_NET_INCLUDE_DIR SDL/SDL_net.h
    HINTS "${MSYS2_PREFIX}/include" /usr/include /usr/local/include
)
find_library(SDL_NET_LIBRARY NAMES SDL_net SDL_net-1.2
    HINTS "${MSYS2_PREFIX}/lib" /usr/lib /usr/local/lib
)

if(NOT SDL_NET_LIBRARY OR NOT SDL_NET_INCLUDE_DIR)
    message(FATAL_ERROR
        "SDL_net 1.2 not found.\n"
        "Run: pacman -S mingw-w64-clang-x86_64-SDL_net"
    )
endif()
message(STATUS "Found SDL_net 1.2: ${SDL_NET_LIBRARY}")

# ─────────────────────────────────────────────
#  Find SDL_ttf 1.2
# ─────────────────────────────────────────────
find_path(SDL_TTF_INCLUDE_DIR SDL/SDL_ttf.h
    HINTS "${MSYS2_PREFIX}/include" /usr/include /usr/local/include
)
find_library(SDL_TTF_LIBRARY NAMES SDL_ttf SDL_ttf-1.2
    HINTS "${MSYS2_PREFIX}/lib" /usr/lib /usr/local/lib
)

if(NOT SDL_TTF_LIBRARY OR NOT SDL_TTF_INCLUDE_DIR)
    message(FATAL_ERROR
        "SDL_ttf 1.2 not found.\n"
        "Run: pacman -S mingw-w64-clang-x86_64-SDL_ttf"
    )
endif()
message(STATUS "Found SDL_ttf 1.2: ${SDL_TTF_LIBRARY}")

# ─────────────────────────────────────────────
#  Find ZLIB
# ─────────────────────────────────────────────
find_package(ZLIB REQUIRED)

# ─────────────────────────────────────────────
#  Find libpng
# ─────────────────────────────────────────────
find_package(PNG REQUIRED)

# ─────────────────────────────────────────────
#  Source Files
# ─────────────────────────────────────────────
file(GLOB_RECURSE SOURCES
    "src/*.cpp"
    "src/*.cxx"
    "src/*.cc"
    "src/*.c"
)

# Fallback: create a dummy main if no sources exist yet
if(NOT SOURCES)
    message(WARNING "No source files found in src/. Add .cpp files to src/")
    set(SOURCES "src/main.cpp")
endif()

# ─────────────────────────────────────────────
#  Executable
# ─────────────────────────────────────────────
add_executable(${PROJECT_NAME} ${SOURCES})

# ─────────────────────────────────────────────
#  Include Directories
# ─────────────────────────────────────────────
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${SDL_INCLUDE_DIR}
    ${SDL_NET_INCLUDE_DIR}
    ${SDL_TTF_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIRS}
    ${PNG_INCLUDE_DIRS}
)

# ─────────────────────────────────────────────
#  Link Libraries
# ─────────────────────────────────────────────
set(SDL_ALL_LIBS ${SDL_LIBRARY} ${SDL_NET_LIBRARY} ${SDL_TTF_LIBRARY})
if(SDLMAIN_LIBRARY)
    list(PREPEND SDL_ALL_LIBS ${SDLMAIN_LIBRARY})
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${SDL_ALL_LIBS}
    ZLIB::ZLIB
    PNG::PNG
)

# MSYS2/Windows: link extra system libs SDL 1.2 needs
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        mingw32        # Required for WinMain with SDLmain
        winmm          # Multimedia timers
        imm32          # Input method
        version        # Version info
        setupapi       # Device setup
    )
    # Use WinMain subsystem
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

# ─────────────────────────────────────────────
#  Windows: Copy DLLs from MSYS2 CLANG64
# ─────────────────────────────────────────────
if(WIN32 AND DEFINED MSYS2_PREFIX)
    set(MSYS2_BIN "${MSYS2_PREFIX}/bin")

    # All DLLs SDL 1.2 + deps typically need from MSYS2 CLANG64
    set(COPY_DLLS
        SDL.dll
        SDL_net.dll
        SDL_ttf.dll
        zlib1.dll
        libpng16-16.dll
        libfreetype-6.dll       # SDL_ttf dependency
        libbz2-1.dll            # freetype dependency
        libgraphite2.dll        # freetype/harfbuzz dependency
        libharfbuzz-0.dll       # freetype dependency
        libc++.dll              # Clang C++ runtime
        libunwind.dll           # Clang unwind
        # Brotli (freetype font compression dependency)
        libbrotlidec.dll
        libbrotlicommon.dll
        # GLib (harfbuzz dependency)
        libglib-2.0-0.dll
        libgobject-2.0-0.dll    # GLib object system
        libintl-8.dll           # GLib i18n dependency
        libiconv-2.dll          # iconv (intl dependency)
        libpcre2-8-0.dll        # GLib regex dependency
    )

    foreach(DLL_FILE ${COPY_DLLS})
        set(DLL_PATH "${MSYS2_BIN}/${DLL_FILE}")
        if(EXISTS "${DLL_PATH}")
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${DLL_PATH}"
                    "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
                COMMENT "Copying ${DLL_FILE}"
            )
        endif()
    endforeach()
endif()

# ─────────────────────────────────────────────
#  Install Rules
# ─────────────────────────────────────────────
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# ─────────────────────────────────────────────
#  Copy Assets (img and bs2 folder) to build output
# ─────────────────────────────────────────────
foreach(ASSET_DIR img bs2 misc)
    if(EXISTS "${CMAKE_SOURCE_DIR}/Resources/${ASSET_DIR}")
        if("${ASSET_DIR}" STREQUAL "img")
            # img goes into its own subfolder
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "${CMAKE_SOURCE_DIR}/Resources/${ASSET_DIR}"
                    "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${ASSET_DIR}"
                COMMENT "Copying ${ASSET_DIR}/ to build directory"
            )
        else()
            # bs2 and misc go directly into bin/
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "${CMAKE_SOURCE_DIR}/Resources/${ASSET_DIR}"
                    "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
                COMMENT "Copying ${ASSET_DIR}/ to build directory"
            )
        endif()
    else()
        message(STATUS "Asset folder Resources/${ASSET_DIR} not found, skipping.")
    endif()
endforeach()

# ─────────────────────────────────────────────
#  Summary
# ─────────────────────────────────────────────
message(STATUS "")
message(STATUS "=== BS2Plus Configuration (MSYS2 CLANG64 / SDL 1.2) ===")
message(STATUS "  Project     : ${PROJECT_NAME} v${PROJECT_VERSION}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build type  : ${CMAKE_BUILD_TYPE}")
message(STATUS "  Compiler    : ${CMAKE_CXX_COMPILER}")
message(STATUS "  SDL 1.2     : ${SDL_LIBRARY}")
message(STATUS "  SDL_net 1.2 : ${SDL_NET_LIBRARY}")
message(STATUS "  SDL_ttf 1.2 : ${SDL_TTF_LIBRARY}")
message(STATUS "  ZLIB        : ${ZLIB_LIBRARIES}")
message(STATUS "  PNG         : ${PNG_LIBRARIES}")
message(STATUS "  MSYS2 prefix: ${MSYS2_PREFIX}")
message(STATUS "=======================================================")
message(STATUS "")