cmake_minimum_required(VERSION 3.16)
project(Cartographer C CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Force modern C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 23)

# -----------------------------------------------------------------
# 1. CONFIGURE BUILD FLAGS FOR SDL AND ITS EXTENSIONS
# -----------------------------------------------------------------
# Optimize build times by disabling tests, examples, and installs
set(SDL_TESTS OFF CACHE BOOL "Disable SDL test suite" FORCE)
set(SDL_INSTALL OFF CACHE BOOL "Disable installation targets" FORCE)

# SDL3_image: Enable formats you want, disable what you don't need
set(SDL_IMAGE_SAMPLES OFF CACHE BOOL "" FORCE)
set(SDL_IMAGE_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_IMAGE_INSTALL OFF CACHE BOOL "" FORCE)
set(SDL_IMAGE_BACKEND_STB ON CACHE BOOL "Use built-in STB for PNG/JPEG" FORCE)

# Choose dynamic (.dll/.so) or static (.lib/.a) linking
# Note: SDL3's extensions naturally default to matching the base SDL settings
set(BUILD_SHARED_LIBS ON CACHE BOOL "Build everything as shared libraries" FORCE)

# -----------------------------------------------------------------
# 2. FETCH AND LOAD THE EXTERNAL SOURCE CODE
# -----------------------------------------------------------------
include(FetchContent)

# --- Fetch Core SDL3 ---
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        main # Or pinpoint a specific release tag, e.g., release-3.2.0
)

# --- Fetch SDL3_image ---
FetchContent_Declare(
    SDL3_image
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_image.git
    GIT_TAG        main
)

# Compile everything and make targets available
# Order matters: Satellite libraries rely on core SDL3 targets being ready first
FetchContent_MakeAvailable(SDL3 SDL3_image)

# -----------------------------------------------------------------
# 3. DEFINE AND LINK YOUR EXECUTABLE AND LIBRARIES
# -----------------------------------------------------------------

add_library(cartographer_headers INTERFACE)

target_include_directories(cartographer_headers INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(Cartographer::headers ALIAS cartographer_headers)

add_executable(cartographer 
    src/main.c
)

add_executable(Cartographer::cli ALIAS cartographer)

target_compile_options(cartographer PRIVATE -g)

target_include_directories(cartographer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Link your game against SDL3 and its extension modules. 
# This automatically handles includes (-I) and binaries (-l).
target_link_libraries(cartographer PRIVATE 
    SDL3::SDL3
    SDL3_image::SDL3_image
)

# -----------------------------------------------------------------
# 4. AUTOMATIC DLL / SO COPYING (Crucial for Windows)
# -----------------------------------------------------------------
# If you build with BUILD_SHARED_LIBS=ON, this step automatically copies 
# the generated compiled .dll files to the folder where your game .exe lives.
if(WIN32)
    add_custom_command(TARGET cartographer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:SDL3::SDL3>
        $<TARGET_FILE:SDL3_image::SDL3_image>
        $<TARGET_FILE_DIR:cartographer>
    )
endif()
