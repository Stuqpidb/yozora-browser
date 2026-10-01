# Yozora unit tests.
#
# Only logic that is worth protecting is tested: URL/search classification and
# the tab arithmetic. Rendering a page is verified by running the browser, not
# by a test.

set(YOZORA_TESTS
    tst_urlutils
    tst_tabwidget
    tst_errorpage
    tst_glass
    tst_library
    tst_nightsky
)

# Qt's shared libraries live next to the Qt installation, not next to the test
# binary. Without this the test process would pop a modal "Qt6Core.dll not
# found" dialog and hang instead of failing.
get_target_property(_qt_core_location Qt6::Core IMPORTED_LOCATION)
get_filename_component(_qt_bin_dir "${_qt_core_location}" DIRECTORY)

foreach(test IN LISTS YOZORA_TESTS)
    qt_add_executable(${test} ${test}.cpp)
    target_link_libraries(${test} PRIVATE yozora_core Qt6::Test)
    target_include_directories(${test} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/../src"
        "${CMAKE_BINARY_DIR}/generated"
    )
    if(MSVC)
        target_compile_options(${test} PRIVATE /utf-8)
    endif()

    # Tests must be console applications: with the GUI subsystem QTest output
    # goes to a console that CI never sees.
    set_target_properties(${test} PROPERTIES
        WIN32_EXECUTABLE OFF
        MACOSX_BUNDLE OFF
    )

    add_test(NAME ${test} COMMAND ${test})

    # Widget tests need a platform plugin; offscreen keeps CI headless.
    set_tests_properties(${test} PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        TIMEOUT 60
    )
    if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.22)
        set_tests_properties(${test} PROPERTIES
            ENVIRONMENT_MODIFICATION
                "PATH=path_list_append:${_qt_bin_dir}"
        )
    endif()
endforeach()
