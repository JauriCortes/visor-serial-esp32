cmake_minimum_required(VERSION 3.21)
project(esp32visor VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 SerialPort)
qt_standard_project_setup(REQUIRES 6.5)

# The QML files live in qml/, so ask Qt for the per-subdirectory qmldir.
if(QT_KNOWN_POLICY_QTP0004)
    qt_policy(SET QTP0004 NEW)
endif()

qt_add_executable(esp32visor src/main.cpp)

# Must be declared before the module so the generated qmldir marks it singleton.
set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(esp32visor
    URI Esp32Visor
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/Theme.qml
        qml/ConnectionBar.qml
        qml/StatusBar.qml
        qml/ConsoleView.qml
        qml/GenerativeView.qml
    SOURCES
        src/LogLine.h
        src/SerialLink.h
        src/SerialLink.cpp
        src/LineAssembler.h
        src/LineAssembler.cpp
        src/LogModel.h
        src/LogModel.cpp
)

target_include_directories(esp32visor PRIVATE src)
target_link_libraries(esp32visor PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::SerialPort
)

# Installs to the prefix given on the command line, e.g.
#   cmake --install <build> --prefix ~/.local
# On Windows the app ships as a folder that gets unzipped and double clicked,
# so the .exe belongs at the root, next to the DLLs windeployqt puts there.
if(WIN32)
    set(CMAKE_INSTALL_BINDIR ".")
else()
    set(CMAKE_INSTALL_BINDIR "bin")
endif()

install(TARGETS esp32visor RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# The launcher entry and its icon only mean something on a desktop Linux.
if(UNIX AND NOT APPLE)
    install(FILES packaging/esp32visor.desktop DESTINATION share/applications)
    install(FILES packaging/esp32visor.svg
            DESTINATION share/icons/hicolor/scalable/apps)
endif()

# The Qt libraries are not installed here: the release workflow runs
# windeployqt (Windows) and linuxdeploy (Linux) over this tree.  Qt's own
# deployment script splits the .exe from its DLLs across bin/ and the prefix
# root, which does not run, so it is not used.

# Proves the criterion of RNF-08: the pipeline classes build and run with no
# window and no serial port in sight.  ctest --test-dir <build>
option(BUILD_TESTING "Build the LineAssembler checks" ON)
if(BUILD_TESTING)
    enable_testing()
    qt_add_executable(test_lineassembler
        tests/test_lineassembler.cpp
        src/LineAssembler.cpp
        src/LineAssembler.h
    )
    target_include_directories(test_lineassembler PRIVATE src)
    target_link_libraries(test_lineassembler PRIVATE Qt6::Core)
    add_test(NAME lineassembler COMMAND test_lineassembler)
    set_tests_properties(lineassembler PROPERTIES
        ENVIRONMENT "QT_LOGGING_RULES=*.info=true")
endif()
