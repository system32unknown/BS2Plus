cmake_minimum_required(VERSION 3.16)
project(MySDLProject VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ─────────────────────────────────────────
#  Output directories
# ─────────────────────────────────────────
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ─────────────────────────────────────────
#  Find Packages
# ─────────────────────────────────────────
find_package(SDL REQUIRED)
find_package(SDL_net REQUIRED)
find_package(SDL_ttf REQUIRED)

# ─────────────────────────────────────────
#  Source Files
# ─────────────────────────────────────────
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h")

# ─────────────────────────────────────────
#  Executable
# ─────────────────────────────────────────
add_executable(${PROJECT_NAME} ${SOURCES})

# ─────────────────────────────────────────
#  Include Directories
# ─────────────────────────────────────────
target_include_directories(${PROJECT_NAME} PRIVATE
    ${SDL_INCLUDE_DIRS}
    ${SDL_NET_INCLUDE_DIRS}
    ${SDL_TTF_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/include
)

# ─────────────────────────────────────────
#  Link Libraries
# ─────────────────────────────────────────
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${SDL_LIBRARIES}
    ${SDL_NET_LIBRARIES}
    ${SDL_TTF_LIBRARIES}
)

# ─────────────────────────────────────────
#  Windows: Copy DLLs to output folder
# ─────────────────────────────────────────
if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL::SDL>
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
endif()

# ─────────────────────────────────────────
#  Install Rules (optional)
# ─────────────────────────────────────────
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
```

---

## Project Structure
```
MySDLProject/
├── CMakeLists.txt
├── include/
│   └── (your headers)
└── src/
    └── main.cpp