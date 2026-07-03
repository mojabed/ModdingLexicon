cmake_minimum_required(VERSION 3.16)

project(ModdingLexicon VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Quick QuickControls2 Network Concurrent WebEngineQuick)

qt_standard_project_setup(REQUIRES 6.8)

add_subdirectory(external/spdlog)

set(PROJECT_SOURCES
    src/pathing.cpp
    src/lexicon.cpp
    src/descriptionparser.cpp
    src/descriptionfetcher.cpp
    src/parser.cpp
    src/HttpClient.cpp
    src/AddonModel.cpp
    src/AddonFilterModel.cpp
    src/CategoryModel.cpp
)

set(PROJECT_HEADERS
    include/pathing.h
    include/ModType.h
    include/lexicon.h
    include/descriptionparser.h
    include/descriptionfetcher.h
    include/parser.h
    include/HttpClient.h
    include/CategoryModel.h
    include/AddonModel.h
    include/AddonFilterModel.h
)

set(app_icon_resource_windows "${CMAKE_CURRENT_SOURCE_DIR}/resources/lexicon.rc")

qt_add_executable(ModdingLexicon
    main.cpp
    ${PROJECT_SOURCES}
    ${PROJECT_HEADERS}
    resources/Fonts.qrc
    ${app_icon_resource_windows}
)

qt_add_qml_module(ModdingLexicon
    URI ModdingLexicon
    VERSION 1.0
    QML_FILES
        Main.qml
        MainTabBar.qml
        ListItemDelegate.qml
        MyAddonsView.qml
        BrowseAddonsView.qml
        SettingsView.qml
        AddonDetailWindow.qml
    SOURCES
        include/lexicon.h
        include/descriptionfetcher.h
        include/AddonModel.h
        include/AddonFilterModel.h
        include/CategoryModel.h
        src/lexicon.cpp
        src/AddonModel.cpp
        src/AddonFilterModel.cpp
        src/CategoryModel.cpp
    DEPENDENCIES QtQuick QtWebEngineQuick
)

target_include_directories(ModdingLexicon
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(ModdingLexicon PRIVATE /EHsc)

target_compile_definitions(ModdingLexicon PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:_SILENCE_CLANG_COROUTINE_MESSAGE>
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(ModdingLexicon PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.ModdingLexicon
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(ModdingLexicon
    PRIVATE
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Network
    Qt6::Concurrent
    Qt6::WebEngineQuick
    spdlog::spdlog
)

if(MSVC)
    message(STATUS "MSVC version (internal): ${MSVC_VERSION}")
    message(STATUS "MSVC toolset version:  ${MSVC_TOOLSET_VERSION}")
    message(STATUS "Full Compiler Version: ${CMAKE_CXX_COMPILER_VERSION}")
endif()

# Auto deploy Qt DLLs
if(WIN32)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt
        HINTS "${QT6_INSTALL_PREFIX}/bin"
    )
    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET ModdingLexicon POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}"
                --qmldir "${CMAKE_CURRENT_SOURCE_DIR}"
                "$<TARGET_FILE:ModdingLexicon>"
            COMMENT "Deploying Qt runtime dependencies..."
        )
    endif()
endif()

include(GNUInstallDirs)
install(TARGETS ModdingLexicon
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
