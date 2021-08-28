set(UNIT_TEST tester)
add_executable(${UNIT_TEST} main.c)
target_link_libraries(${UNIT_TEST} unity)

add_test(NAME ${UNIT_TEST} COMMAND ${UNIT_TEST})
add_custom_command(
    TARGET ${UNIT_TEST}
    COMMENT "Run tests"
    POST_BUILD
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMAND ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION> -R "^${UNIT_TEST}" --output-on-failures
)
