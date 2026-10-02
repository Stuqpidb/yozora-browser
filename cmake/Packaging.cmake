# ---------------------------------------------------------------------------
# Yozora packaging
#
# `cmake --install` produces a self-contained folder (Qt DLLs included) and
# `cpack` wraps it into YozoraSetup-<version>.exe. Keeping this in its own file
# means the build logic never has to think about installers.
# ---------------------------------------------------------------------------

# Resolve the Qt6 bin directory (windeployqt lives next to the Qt libraries).
get_target_property(_qt_core_location Qt6::Core IMPORTED_LOCATION)
get_filename_component(_qt_bin_dir "${_qt_core_location}" DIRECTORY)

# Layout of the staged folder, relative to the install prefix.
if(WIN32)
    set(YOZORA_STAGE_SUBDIR ".")      # Yozora.exe next to the Qt DLLs
elseif(APPLE)
    set(YOZORA_STAGE_SUBDIR "Yozora.app/Contents/MacOS")
else()
    set(YOZORA_STAGE_SUBDIR "bin")
endif()

install(TARGETS yozora RUNTIME DESTINATION "${YOZORA_STAGE_SUBDIR}")

# Deploy the Qt runtime into the staged folder. On Windows the user must not
# have to install Qt, a compiler, the Visual C++ redistributable or a graphics
# redistributable by hand, so windeployqt runs with its defaults on purpose:
# only translations are dropped, because Yozora ships no translated strings yet.
if(WIN32)
    install(CODE "
        set(_stage \"\${CMAKE_INSTALL_PREFIX}\")
        if(NOT EXISTS \"\${_stage}/Qt6Core.dll\")
            execute_process(
                COMMAND \"${_qt_bin_dir}/windeployqt.exe\"
                        --release --no-translations
                        \"\${_stage}/Yozora.exe\"
                RESULT_VARIABLE _deploy_result
                OUTPUT_QUIET
                ERROR_QUIET)
            if(NOT _deploy_result EQUAL 0)
                message(FATAL_ERROR
                    \"windeployqt failed (exit \${_deploy_result}). The staged build would not run on a machine without Qt installed.\")
            endif()
        endif()
    ")
endif()

# --- Licence texts ----------------------------------------------------------
# The staged folder ships Qt's shared libraries, and those are licensed under
# terms of their own. The texts therefore have to travel with the binaries: a
# distribution that conveys no licence conveys no permission either, and the
# person who just installed this has no way to find out what they may do with
# it. LICENSES/THIRD-PARTY-NOTICES.txt explains which option of Qt's licence
# choice was taken and how to exercise the LGPLv3 relinking right.
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
        DESTINATION "licenses"
        RENAME "LICENSE-Yozora-MIT.txt")
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/LICENSES/"
        DESTINATION "licenses"
        FILES_MATCHING
        PATTERN "*.txt")

# --- Windows installer ------------------------------------------------------
if(WIN32)
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_PACKAGE_NAME "Yozora Browser")
    set(CPACK_PACKAGE_VENDOR "The Yozora Browser Authors")
    set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
    set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
    set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
    set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "Yozora Browser")
    set(CPACK_PACKAGE_EXECUTABLES "Yozora" "Yozora Browser")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
    set(CPACK_NSIS_PACKAGE_NAME "Yozora Browser")
    set(CPACK_NSIS_DISPLAY_NAME "Yozora Browser")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_NSIS_MODIFY_PATH OFF)
    set(CPACK_NSIS_EXECUTABLES_DIRECTORY ".")
    # Start menu entry. It takes a bare target name from
    # CPACK_PACKAGE_EXECUTABLES; CPACK_NSIS_MENU_LINKS (which would create an
    # extra sub-folder) is deliberately left unset so the shortcut sits
    # directly in the Programs folder, the way Firefox and Chrome do it.
    #
    # The desktop shortcut is offered as a checkbox by the installer's own
    # "Install Options" page. CPack reads that checkbox into $INSTALL_DESKTOP
    # but never acts on it, so the two hooks below are what make it real: one
    # creates the link when the box was ticked, the other removes it on
    # uninstall. (CPACK_CREATE_DESKTOP_LINKS would force the link on everyone
    # and ignore the checkbox, which is not what we want.)
    # The values are inserted verbatim into CPackConfig.cmake, so '$' is escaped
    # (otherwise CMake would expand $INSTALL_DESKTOP when CPack includes that
    # file) and NSIS single quotes are used so no double quotes appear at all.
    set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS
        "  StrCmp \\\$INSTALL_DESKTOP '1' 0 +2\n    CreateShortCut '\\\$DESKTOP\\\\Yozora Browser.lnk' '\\\$INSTDIR\\\\Yozora.exe'\n")
    set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS
        "  StrCmp \\\$INSTALL_DESKTOP '1' 0 +2\n    Delete '\\\$DESKTOP\\\\Yozora Browser.lnk'\n")

    # Branding: the NSIS wizard's default bitmaps and titles are replaced with
    # the app's own artwork (see installer/make_installer_art.py), the finish
    # page offers to launch the browser, and the installer is DPI-aware.
    set(CPACK_NSIS_WELCOME_TITLE "Welcome to Yozora Browser")
    set(CPACK_NSIS_FINISH_TITLE "Yozora Browser is installed")
    set(CPACK_NSIS_MUI_WELCOMEFINISHPAGE_BITMAP
        "${PROJECT_SOURCE_DIR}/resources/installer/welcome.bmp")
    set(CPACK_NSIS_MUI_UNWELCOMEFINISHPAGE_BITMAP
        "${PROJECT_SOURCE_DIR}/resources/installer/welcome-uninstall.bmp")
    set(CPACK_NSIS_MUI_HEADERIMAGE
        "${PROJECT_SOURCE_DIR}/resources/installer/header.bmp")
    set(CPACK_NSIS_MUI_FINISHPAGE_RUN "Yozora.exe")
    set(CPACK_NSIS_MUI_FINISHPAGE_RUN_TEXT "Launch Yozora Browser")
    set(CPACK_NSIS_MANIFEST_DPI_AWARE ON)
    set(CPACK_NSIS_BRANDING_TEXT "Yozora Browser ${PROJECT_VERSION}")

    set(CPACK_NSIS_URL_INFO_ABOUT "${PROJECT_HOMEPAGE_URL}")
    # The licence page shows the project's own MIT licence instead of the
    # placeholder text CPack ships.
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
    set(CPACK_NSIS_INSTALLED_ICON_NAME "Yozora.exe")
    set(CPACK_NSIS_MUI_ICON "${PROJECT_SOURCE_DIR}/installer/yozora.ico")
    set(CPACK_NSIS_MUI_UNIICON "${PROJECT_SOURCE_DIR}/installer/yozora.ico")
    set(CPACK_NSIS_MUI_UNICON "${PROJECT_SOURCE_DIR}/installer/yozora.ico")
    set(CPACK_PACKAGE_EXECUTABLES_SFX "")
    # The result is C:\...\YozoraSetup-0.1.0.exe
    set(CPACK_PACKAGE_FILE_NAME "YozoraSetup-${PROJECT_VERSION}")
endif()

include(CPack)
