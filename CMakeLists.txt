cmake_minimum_required(VERSION 3.15)
project(TrueSymph CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Statisk runtime: inga extra DLL-filer behövs på användarens dator
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")

# Spärr: MSVC 14.50 (Visual Studio 2026) och nyare kan inte bygga för Windows 7.
if(MSVC AND MSVC_VERSION GREATER_EQUAL 1950)
    message(FATAL_ERROR "Denna kompilator stöder inte Windows 7. Använd Visual Studio 2022 (toolset v143).")
endif()

if(MSVC)
    add_compile_options(/utf-8)   # källfilerna är UTF-8 (svenska kommentarer)
endif()

if(WIN32)
    add_compile_definitions(_WIN32_WINNT=0x0601 WINVER=0x0601 NOMINMAX _CRT_SECURE_NO_WARNINGS)
    set(TRUESYMPH_EDITOR_SOURCES TrueSymphEditor.cpp Manual.h)
    set(TRUESYMPH_EDITOR_LIBS gdiplus user32 gdi32 comdlg32 shell32)
endif()

# --- Pluginet (bara på Windows) -------------------------------------------
if(WIN32)
    add_library(TrueSymph MODULE TrueSymph.cpp TrueSymphCore.h TrueSymphDSP.h Presets.h vst2.h ${TRUESYMPH_EDITOR_SOURCES})
    set_target_properties(TrueSymph PROPERTIES PREFIX "" SUFFIX ".dll")
    target_link_libraries(TrueSymph PRIVATE ${TRUESYMPH_EDITOR_LIBS})
endif()

# --- Automatiska tester ----------------------------------------------------
enable_testing()

add_executable(TrueSymphTests TrueSymphTests.cpp)
add_test(NAME dsp COMMAND TrueSymphTests)

add_executable(HostTests HostTests.cpp TrueSymph.cpp ${TRUESYMPH_EDITOR_SOURCES})
target_link_libraries(HostTests PRIVATE ${TRUESYMPH_EDITOR_LIBS})
add_test(NAME host COMMAND HostTests)

# Bilder av gränssnittet (Windows): ritas med samma kod som pluginet
if(WIN32)
    add_executable(RenderGui RenderGui.cpp ${TRUESYMPH_EDITOR_SOURCES})
    target_link_libraries(RenderGui PRIVATE ${TRUESYMPH_EDITOR_LIBS})
endif()
