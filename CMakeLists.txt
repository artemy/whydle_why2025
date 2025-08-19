cmake_minimum_required(VERSION 4.0)

set(GAME_TITLE whydle2025)

project(${GAME_TITLE} C)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Option to enable RISC-V embedded build
option(ENABLE_RISCV_BUILD "Enable RISC-V embedded build configuration" OFF)

# Check if we're doing a RISC-V build
if(ENABLE_RISCV_BUILD)
    add_definitions(-DRISCV_BUILD)

    message(STATUS "Configuring for RISC-V embedded target")

    set(CMAKE_SYSROOT "${CMAKE_CURRENT_SOURCE_DIR}/badge_sdk_dist")

    # Compiler flags for RISC-V
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -fPIC -fdata-sections -ffunction-sections -flto=auto")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-builtin -fno-builtin-function -fno-jump-tables -fno-tree-switch-conversion")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstrict-volatile-bitfields -fvisibility=hidden -g3")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mabi=ilp32f -march=rv32imafc_zicsr_zifencei")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -nostartfiles -nostdlib")

    # Include directories
    include_directories("${CMAKE_SYSROOT}/include")

    set(SDL3_STATIC_LIB "${CMAKE_SYSROOT}/lib/libsdl3.a")

    add_executable(${GAME_TITLE} ./src/main.c)

    # Set executable properties for RISC-V
    set_target_properties(${GAME_TITLE} PROPERTIES
            LINK_FLAGS "-shared -Wl,--strip-debug -Wl,--gc-sections -Wl,--exclude-libs,libsdl3.a -e main"
            SUFFIX ".elf"
    )

    # Link against the static SDL3 library
    if(SDL3_STATIC_LIB)
        message(STATUS "Found SDL3 static library: ${SDL3_STATIC_LIB}")
        target_link_libraries(${GAME_TITLE} ${SDL3_STATIC_LIB})
    else()
        message(FATAL_ERROR "SDL3 static library not found")
    endif()
else()
    message(STATUS "Configuring for standard desktop build")

    set(RESOURCES_DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/res)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/res/wordle-answers-alphabetical.txt DESTINATION ${RESOURCES_DESTINATION})
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/res/letters.bmp DESTINATION ${RESOURCES_DESTINATION})

    add_executable(${GAME_TITLE} ./src/main.c)

    target_link_libraries(${GAME_TITLE} PUBLIC ${CMAKE_DL_LIBS} m pthread)
    find_package(SDL3 REQUIRED)
    target_link_libraries(${GAME_TITLE} PUBLIC ${SDL3_LIBRARIES})
endif()

target_compile_options(${GAME_TITLE} PRIVATE -Wall -Wextra -Wpedantic -Werror)
