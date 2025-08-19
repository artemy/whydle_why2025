cmake_minimum_required(VERSION 4.0)

set(GAME_TITLE wordle_clone)

project(${GAME_TITLE} C)

#debug/errorlog on/off
add_definitions(-DHFE_DEBUG)

#output dir
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_executable(${GAME_TITLE} ./src/main.c)
else ()
    add_executable(${GAME_TITLE} ./src/main.c)
endif ()

#copy assets
set(RESOURCES_DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/res)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/res/wordle-answers-alphabetical.txt DESTINATION ${RESOURCES_DESTINATION})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/res/letters.bmp DESTINATION ${RESOURCES_DESTINATION})

# Make compiler scream out every possible warning
target_compile_options(${GAME_TITLE} PRIVATE -Wconversion -Wall -Wextra -Wpedantic -Werror)

#includes
include_directories(${GAME_TITLE} PUBLIC ${CMAKE_SOURCE_DIR}/include)

#libs
#sdl
target_link_libraries(${GAME_TITLE} PUBLIC ${CMAKE_DL_LIBS} m pthread)
find_package(SDL3 REQUIRED)
target_link_libraries(${GAME_TITLE} PUBLIC ${SDL3_LIBRARIES})
