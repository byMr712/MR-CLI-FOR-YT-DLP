cmake_minimum_required(VERSION 3.16)
project(MR-CLI-FOR-YT-DLP LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(
        /utf-8
        /EHsc
        /W3
        /permissive-
        /guard:cf
    )
    add_link_options(
        /guard:cf
        /DYNAMICBASE
        /NXCOMPAT
        /RELEASE
    )
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        add_link_options(/CETCOMPAT)
    endif()
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(CORE_SOURCES
    src/core/Clipboard.cpp
    src/core/Console.cpp
    src/core/FileSystem.cpp
    src/core/HttpClient.cpp
    src/core/Keyboard.cpp
    src/core/Process.cpp
    src/core/ZipExtractor.cpp
)

set(SERVICES_SOURCES
    src/services/ArchiveService.cpp
    src/services/ConfigService.cpp
    src/services/CookieService.cpp
    src/services/DependencyManager.cpp
    src/services/FormatBuilder.cpp
    src/services/LocalizationService.cpp
    src/services/Logger.cpp
    src/services/OutputParser.cpp
    src/services/PathProvider.cpp
    src/services/YtDlpService.cpp
)

set(UI_SOURCES
    src/ui/Dialogs.cpp
    src/ui/InputReader.cpp
    src/ui/Menu.cpp
    src/ui/ProgressRenderer.cpp
    src/ui/screens/CookieEditorScreen.cpp
    src/ui/screens/DownloadScreen.cpp
    src/ui/screens/MainMenuScreen.cpp
    src/ui/screens/SettingsScreen.cpp
)

set(APP_SOURCES
    src/app/Application.cpp
    src/main.cpp
)

add_executable(mr-cli-yt-dlp
    ${CORE_SOURCES}
    ${SERVICES_SOURCES}
    ${UI_SOURCES}
    ${APP_SOURCES}
    mr-cli-yt-dlp.rc
)

target_link_libraries(mr-cli-yt-dlp PRIVATE
    wininet
    shell32
    ole32
    crypt32
    user32
    comdlg32
    advapi32
)

# Test executables
enable_testing()

add_executable(TestOutputParser
    tests/TestOutputParser.cpp
    src/services/OutputParser.cpp
)
target_link_libraries(TestOutputParser PRIVATE)
add_test(NAME OutputParserTest COMMAND TestOutputParser)

add_executable(TestFormatBuilder
    tests/TestFormatBuilder.cpp
    src/services/FormatBuilder.cpp
)
target_link_libraries(TestFormatBuilder PRIVATE)
add_test(NAME FormatBuilderTest COMMAND TestFormatBuilder)

add_executable(TestConfig
    tests/TestConfig.cpp
    src/services/ConfigService.cpp
    src/core/FileSystem.cpp
)
target_link_libraries(TestConfig PRIVATE shell32 ole32)
add_test(NAME ConfigTest COMMAND TestConfig)

add_executable(TestLocalization
    tests/TestLocalization.cpp
    src/services/LocalizationService.cpp
)
target_link_libraries(TestLocalization PRIVATE)
add_test(NAME LocalizationTest COMMAND TestLocalization)
