cmake_minimum_required(VERSION 3.16)

project(ModdingLexicon VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick)

qt_standard_project_setup(REQUIRES 6.8)

add_subdirectory(external/spdlog)

# Add your source files
set(PROJECT_SOURCES
    src/pathing.cpp
)

# Add your headers
set(PROJECT_HEADERS
    include/pathing.h
    include/ModType.h
)

qt_add_executable(appModdingLexicon
    main.cpp
)

qt_add_qml_module(appModdingLexicon
    URI ModdingLexicon
    VERSION 1.0
    QML_FILES
        Main.qml
    DEPENDENCIES QtQuick
)

# Add include directory
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
    spdlog::spdlog
)

include(GNUInstallDirs)
install(TARGETS appModdingLexicon
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)