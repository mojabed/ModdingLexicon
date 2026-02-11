cmake_minimum_required(VERSION 3.16)

project(ModdingLexicon VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Quick QuickControls2 Network)

qt_standard_project_setup(REQUIRES 6.8)

add_subdirectory(external/spdlog)

set(PROJECT_SOURCES
    src/pathing.cpp
    src/lexicon.cpp
    src/parser.cpp
    src/HttpClient.cpp
    src/AddonModel.cpp
)

set(PROJECT_HEADERS
    include/pathing.h
    include/ModType.h
    include/lexicon.h
    include/parser.h
    include/HttpClient.h
    include/AddonModel.h
)

set(app_icon_resource_windows "${CMAKE_CURRENT_SOURCE_DIR}/resources/lexicon.rc")

qt_add_executable(appModdingLexicon
    main.cpp
    ${PROJECT_SOURCES}
    ${PROJECT_HEADERS}
    resources/Fonts.qrc
    ${app_icon_resource_windows}
)

qt_add_qml_module(appModdingLexicon
    URI ModdingLexicon
    VERSION 1.0
    QML_FILES
        Main.qml
    SOURCES
        include/lexicon.h
        include/AddonModel.h
        src/lexicon.cpp
        src/AddonModel.cpp
    DEPENDENCIES QtQuick
)

target_include_directories(appModdingLexicon
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(appModdingLexicon PRIVATE /EHsc)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appModdingLexicon PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appModdingLexicon
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appModdingLexicon
    PRIVATE
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Network
    spdlog::spdlog
)

include(GNUInstallDirs)
install(TARGETS appModdingLexicon
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)