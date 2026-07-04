# Simulation code lives in the openarpg_core static library so the headless
# test runner can link it without raylib; openarpg.c is the raylib
# platform/render layer on top and stays out of the library.
file(GLOB_RECURSE CORE_SOURCE_FILES CONFIGURE_DEPENDS *.c)
file(GLOB_RECURSE CORE_HEADER_FILES CONFIGURE_DEPENDS *.h)
list(REMOVE_ITEM CORE_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/openarpg.c)

add_library(openarpg_core STATIC ${CORE_SOURCE_FILES} ${CORE_HEADER_FILES})
target_include_directories(openarpg_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(openarpg_core PRIVATE openarpg_build_flags)
# libm for sqrtf etc.; MSVC has it built into the CRT.
target_link_libraries(openarpg_core PUBLIC $<$<NOT:$<C_COMPILER_ID:MSVC>>:m>)
set_target_properties(openarpg_core PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON)

target_sources(${PROJECT_NAME} PRIVATE openarpg.c)
target_link_libraries(${PROJECT_NAME} openarpg_core)
