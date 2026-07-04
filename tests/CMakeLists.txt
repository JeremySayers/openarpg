add_executable(openarpg_tests test_main.c)
target_link_libraries(openarpg_tests openarpg_core openarpg_build_flags)
set_target_properties(openarpg_tests PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON)

add_test(NAME openarpg_core_tests COMMAND openarpg_tests)
