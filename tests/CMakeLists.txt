include(FetchContent)

# doctest is header-only: download just doctest.h for the pinned release and
# verify its hash, instead of cloning the whole repository (whose examples
# exceed MAX_PATH when the build directory is deep).
FetchContent_Declare(doctest
    URL https://raw.githubusercontent.com/doctest/doctest/v2.4.11/doctest/doctest.h
    URL_HASH SHA256=44faa038e9c3f9728efbda143748d01124ea0a27f4bf78f35a15d8fab2e039fb
    DOWNLOAD_NO_EXTRACT TRUE
    SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/_deps/doctest/doctest)
FetchContent_MakeAvailable(doctest)

add_library(inpututil_doctest INTERFACE)
target_include_directories(inpututil_doctest SYSTEM INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/_deps/doctest)

add_executable(inpututil_tests
    main.cpp
    test_status.cpp
    test_backend.cpp
    test_session.cpp
    test_key.cpp
    test_key_combo.cpp
    test_keyboard.cpp
    test_text.cpp
    test_coords.cpp
    test_motion.cpp
    test_mouse.cpp
    test_mouse_actions.cpp
    test_sequence.cpp
    test_abort.cpp
    test_emergency.cpp
    test_windows_names.cpp
)
target_include_directories(inpututil_tests PRIVATE ${PROJECT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(inpututil_tests PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN UNICODE _UNICODE)
target_link_libraries(inpututil_tests PRIVATE inpututil::inpututil inpututil_doctest inpututil_warnings)

add_test(NAME inpututil_tests COMMAND inpututil_tests)

# Emergency release in a real dying process (no real input: the helper's
# backend prints the events). Each case must print the release before dying.
add_executable(inpututil_emergency_helper emergency_helper.cpp)
target_include_directories(inpututil_emergency_helper PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(inpututil_emergency_helper PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN UNICODE _UNICODE)
target_link_libraries(inpututil_emergency_helper PRIVATE inpututil::inpututil inpututil_warnings)

foreach(death crash terminate abort exit)
    add_test(NAME emergency_${death} COMMAND inpututil_emergency_helper ${death})
    set_tests_properties(emergency_${death} PROPERTIES
        PASS_REGULAR_EXPRESSION "RELEASED vk=0x41"
        TIMEOUT 30)
endforeach()
