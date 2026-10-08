cmake_minimum_required(VERSION 3.21)

project(inpututil
    VERSION 2.0.0
    DESCRIPTION "Mouse and keyboard input simulation for Windows (SendInput)"
    LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "inpututil only supports Windows")
endif()

option(INPUTUTIL_BUILD_TESTS "Build the inpututil unit tests" ${PROJECT_IS_TOP_LEVEL})
option(INPUTUTIL_BUILD_EXAMPLES "Build the inpututil examples" ${PROJECT_IS_TOP_LEVEL})
option(INPUTUTIL_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(INPUTUTIL_INSTALL "Generate the install target" ${PROJECT_IS_TOP_LEVEL})

include(GNUInstallDirs)

# Warning flags shared by the library, tests and examples.
add_library(inpututil_warnings INTERFACE)
if(MSVC)
    target_compile_options(inpututil_warnings INTERFACE /W4 /permissive- /utf-8
        $<$<BOOL:${INPUTUTIL_WARNINGS_AS_ERRORS}>:/WX>)
else()
    target_compile_options(inpututil_warnings INTERFACE -Wall -Wextra -Wpedantic
        $<$<BOOL:${INPUTUTIL_WARNINGS_AS_ERRORS}>:-Werror>)
endif()

add_library(inpututil
    src/InputUtilities.cpp
    src/InputUtilitiesCore.cpp
    src/Status.cpp
    src/Win32Backend.cpp
)
add_library(inpututil::inpututil ALIAS inpututil)

target_compile_features(inpututil PUBLIC cxx_std_20)
target_include_directories(inpututil
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_definitions(inpututil PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN UNICODE _UNICODE)
target_link_libraries(inpututil
    PUBLIC user32 winmm
    PRIVATE $<BUILD_INTERFACE:inpututil_warnings>)

if(INPUTUTIL_INSTALL)
    include(CMakePackageConfigHelpers)

    install(TARGETS inpututil EXPORT inpututilTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    install(EXPORT inpututilTargets
        NAMESPACE inpututil::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/inpututil)

    configure_package_config_file(cmake/inpututilConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/inpututilConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/inpututil)
    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/inpututilConfigVersion.cmake
        COMPATIBILITY SameMajorVersion)
    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/inpututilConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/inpututilConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/inpututil)
endif()

if(INPUTUTIL_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
