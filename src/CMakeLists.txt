# The directory layout enforces the raylib boundary: *.c files directly in
# src/ are simulation code, compiled into the raylib-free openarpg_core
# library that the headless test runner links; *.c files in src/platform/
# may use raylib and only link into the game executable. Adding a file to
# either side needs no CMake changes.
file(GLOB CORE_SOURCE_FILES CONFIGURE_DEPENDS *.c)
file(GLOB CORE_HEADER_FILES CONFIGURE_DEPENDS *.h)

add_library(openarpg_core STATIC ${CORE_SOURCE_FILES} ${CORE_HEADER_FILES})
target_include_directories(openarpg_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(openarpg_core PRIVATE openarpg_build_flags)
# libm for sqrtf etc.; MSVC has it built into the CRT.
target_link_libraries(openarpg_core PUBLIC $<$<NOT:$<C_COMPILER_ID:MSVC>>:m>)
set_target_properties(openarpg_core PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON)

file(GLOB PLATFORM_SOURCE_FILES CONFIGURE_DEPENDS platform/*.c)
file(GLOB PLATFORM_HEADER_FILES CONFIGURE_DEPENDS platform/*.h)

target_sources(${PROJECT_NAME} PRIVATE ${PLATFORM_SOURCE_FILES} ${PLATFORM_HEADER_FILES})
target_link_libraries(${PROJECT_NAME} openarpg_core)
