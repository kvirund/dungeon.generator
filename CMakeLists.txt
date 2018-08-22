cmake_minimum_required(VERSION 3.0)

project(dungeon.generator)

# TODO: Change hardcoded values to be provided from environment variables.
set(TARGET run)
set(SDL2_PREFIX "D:\\environment\\misc\\SDL2-2.0.8")
set(SDL2_TTF_PREFIX "D:\\environment\\misc\\SDL2_ttf-2.0.14")
set(SYSTEM_LIBRARIES)

# SDL2 starts ...
find_path(SDL2_INCLUDE_DIRS SDL.h "${SDL2_PREFIX}/include")
find_library(SDL2_LIB SDL2 PATHS "${SDL2_PREFIX}/lib/x64")
find_library(SDL2_LIB_MAIN SDL2main PATHS "${SDL2_PREFIX}/lib/x64")

if (NOT SDL2_LIB OR NOT SDL2_LIB_MAIN)
	message(FATAL_ERROR "Couldn't find SDL2 libraries.")
endif ()

set(SDL2_LIBRARIES ${SDL2_LIB} ${SDL2_LIB_MAIN})
message(STATUS "SDL2 includes: ${SDL2_INCLUDE_DIRS}")
message(STATUS "SDL2 libraries: ${SDL2_LIBRARIES}")
# ... SDL2 ends

#SDL2_ttf starts ...
find_path(SDL2_TTF_INCLUDE_DIRS SDL_ttf.h "${SDL2_TTF_PREFIX}/include")
find_library(SDL2_TTF_LIB SDL2_ttf PATHS "${SDL2_TTF_PREFIX}/lib/x64")

if (NOT SDL2_TTF_LIB)
	message(FATAL_ERROR "Couldn't find SDL2_ttf libraries.")
endif ()

set(SDL2_TTF_LIBRARIES ${SDL2_TTF_LIB} ${SDL2_TTF_LIB_MAIN})
message(STATUS "SDL2_ttf includes: ${SDL2_TTF_INCLUDE_DIRS}")
message(STATUS "SDL2_ttf libraries: ${SDL2_TTF_LIBRARIES}")
# ... SDL2_ttf ends

set(SOURCE_FILES main.cpp level.cpp level.hpp fractal.cpp fractal.hpp time_utils.hpp time_utils.cpp)
list(SORT SOURCE_FILES)

include_directories(${SDL2_INCLUDE_DIRS} ${SDL2_TTF_INCLUDE_DIRS})
add_executable(${TARGET} ${SOURCE_FILES})
target_link_libraries(${TARGET} ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES} ${SYSTEM_LIBRARIES})