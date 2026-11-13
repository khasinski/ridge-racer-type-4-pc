cmake_minimum_required(VERSION 3.21..3.31)
if(CMAKE_HOST_APPLE AND NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET
   AND (NOT DEFINED CMAKE_SYSTEM_NAME OR CMAKE_SYSTEM_NAME STREQUAL "Darwin"))
    set(CMAKE_OSX_DEPLOYMENT_TARGET "15.0" CACHE STRING "Minimum supported macOS version")
endif()
project(ridge-racer-type-4-pc VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(BUILD_TESTING "Build native game tests" OFF)
option(RR4_BUILD_GAME "Compile the game modules (src/main, src/overlays) into rr4_game" ON)
option(RR4_MACOS_BUNDLE "Package the macOS application bundle (Ridge Racer Type 4.app)" OFF)
option(RR4_EXPORT_SYMBOLS "Export the executable's globals for RR4_WATCH / RR4_POKE on Linux and Windows" OFF)
include(CTest)

set(PSYZ_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/psyz)
add_subdirectory(${PSYZ_DIR}/psyz ${CMAKE_CURRENT_BINARY_DIR}/psyz)
target_compile_definitions(psyz PRIVATE __psyz)

# Shared workspaces and vehicle/track records are accessed through multiple
# typed layout views and rely on two's-complement wrapping arithmetic.
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options("$<$<COMPILE_LANGUAGE:C>:-Werror=pointer-to-int-cast>"
        "$<$<COMPILE_LANGUAGE:C>:-Werror=int-to-pointer-cast>"
        "$<$<COMPILE_LANGUAGE:C>:-Werror=int-conversion>"
        "$<$<COMPILE_LANGUAGE:C>:-Werror=implicit-function-declaration>")
    add_compile_options("$<$<COMPILE_LANGUAGE:C>:-fno-strict-aliasing>"
        "$<$<COMPILE_LANGUAGE:C>:-fwrapv>"
        "$<$<COMPILE_LANGUAGE:C>:-ffunction-sections>"
        "$<$<COMPILE_LANGUAGE:C>:-fdata-sections>")
    if(NOT WIN32)
        add_compile_options("$<$<COMPILE_LANGUAGE:C>:-fno-omit-frame-pointer>")
    endif()
elseif(MSVC)
    add_compile_options("$<$<COMPILE_LANGUAGE:C>:/Gy>" "$<$<COMPILE_LANGUAGE:C>:/Gw>")
endif()

if(APPLE)
    add_link_options(-Wl,-dead_strip)
elseif(NOT MSVC)
    add_link_options(-Wl,--gc-sections)
else()
    add_link_options(/OPT:REF)
endif()

# Game modules: the resident program (src/main, including the game-coupled
# SDK helpers in src/main/psyq that PsyZ lacks), the statically linked
# overlays and the native host services they use.
if(RR4_BUILD_GAME)
    file(GLOB_RECURSE RR4_GAME_SOURCES CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/src/main/*.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/overlays/*.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/native/*.c)
    add_library(rr4_game STATIC ${RR4_GAME_SOURCES})
    target_include_directories(rr4_game
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/include
        SYSTEM PRIVATE
            ${PSYZ_DIR}/psyz/include
    )
    target_compile_definitions(rr4_game PUBLIC __psyz)
    target_link_libraries(rr4_game PUBLIC psyz)
endif()

# Host services of the desktop build: preferences and the Escape settings
# menu (model, CPU raster, SDL_GPU overlay glue), shared with the tests.
add_library(rr4_host STATIC
    src/port/preferences.c
    src/port/settings_model.c
    src/port/settings_raster.c
    src/port/settings_overlay.c
)
target_include_directories(rr4_host
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/port
    SYSTEM PRIVATE
        ${PSYZ_DIR}/psyz/include
        ${PSYZ_DIR}/external/SDL/src/render/gpu/shaders
)
target_compile_definitions(rr4_host PRIVATE __psyz)
target_link_libraries(rr4_host PUBLIC psyz)

add_executable(ridge-racer-type-4-pc
    src/port/main.c
)
target_include_directories(ridge-racer-type-4-pc
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    SYSTEM PRIVATE
        ${PSYZ_DIR}/psyz/include
)
target_compile_definitions(ridge-racer-type-4-pc PRIVATE __psyz)
target_link_libraries(ridge-racer-type-4-pc PRIVATE rr4_host psyz)
if(RR4_BUILD_GAME)
    # The game: src/port/main.c mounts the disc and enters MainLoop.
    target_link_libraries(ridge-racer-type-4-pc PRIVATE rr4_game)
endif()
if(APPLE AND RR4_MACOS_BUNDLE)
    # "Ridge Racer Type 4.app": icon, documentation and the license texts of
    # everything linked into the executable go to Contents/Resources.
    set_target_properties(ridge-racer-type-4-pc PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/packaging/macos/Info.plist"
        OUTPUT_NAME "Ridge Racer Type 4")
    target_sources(ridge-racer-type-4-pc PRIVATE packaging/macos/RidgeRacerType4.icns)
    set_source_files_properties(packaging/macos/RidgeRacerType4.icns PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources)
    foreach(doc README.md CHANGELOG.md LICENSE.md THIRD_PARTY_NOTICES.md)
        if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${doc})
            target_sources(ridge-racer-type-4-pc PRIVATE ${doc})
            set_source_files_properties(${doc} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
        endif()
    endforeach()
    file(STRINGS packaging/licenses.txt RR4_LICENSE_FILES REGEX "^[^#]")
    foreach(notice ${RR4_LICENSE_FILES})
        get_filename_component(notice_dir ${notice} DIRECTORY)
        target_sources(ridge-racer-type-4-pc PRIVATE ${notice})
        set_source_files_properties(${notice} PROPERTIES
            MACOSX_PACKAGE_LOCATION "Resources/licenses/${notice_dir}")
    endforeach()
    include(FetchContent)
    FetchContent_GetProperties(psyz_webp)
    if(psyz_webp_SOURCE_DIR)
        foreach(notice COPYING PATENTS)
            target_sources(ridge-racer-type-4-pc PRIVATE ${psyz_webp_SOURCE_DIR}/${notice})
            set_source_files_properties(${psyz_webp_SOURCE_DIR}/${notice} PROPERTIES
                MACOSX_PACKAGE_LOCATION Resources/licenses/libwebp)
        endforeach()
    endif()
endif()
if(RR4_EXPORT_SYMBOLS)
    if(MINGW)
        target_link_options(ridge-racer-type-4-pc PRIVATE -Wl,--export-all-symbols)
    elseif(NOT APPLE AND NOT MSVC)
        set_target_properties(ridge-racer-type-4-pc PROPERTIES ENABLE_EXPORTS ON)
    endif()
endif()
if(MINGW)
    # Self-contained executables: the GCC runtime and winpthreads are linked
    # statically, leaving only Windows system DLLs (and the UCRT).
    set(RR4_MINGW_STATIC -static -static-libgcc)
    target_link_options(ridge-racer-type-4-pc PRIVATE ${RR4_MINGW_STATIC})
endif()
if(WIN32)
    # CommandLineToArgvW: the UTF-16 command line (src/port/main.c).
    target_link_libraries(ridge-racer-type-4-pc PRIVATE shell32)
    # Executable icon and version information.
    enable_language(RC)
    set(RR4_ICON_ICO ${CMAKE_CURRENT_SOURCE_DIR}/packaging/windows/ridge-racer-type-4.ico)
    configure_file(packaging/windows/ridge-racer-type-4.rc.in
        ${CMAKE_CURRENT_BINARY_DIR}/ridge-racer-type-4.rc @ONLY)
    target_sources(ridge-racer-type-4-pc PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/ridge-racer-type-4.rc)
endif()
psyz_title(ridge-racer-type-4-pc "Ridge Racer Type 4")

if(BUILD_TESTING)
    # Settings menu and preferences, exercised headlessly.
    add_executable(rr4_settings_test tests/port/settings_test.c)
    target_include_directories(rr4_settings_test SYSTEM PRIVATE ${PSYZ_DIR}/psyz/include)
    target_compile_definitions(rr4_settings_test PRIVATE __psyz)
    target_link_libraries(rr4_settings_test PRIVATE rr4_host psyz)
    if(MINGW)
        target_link_options(rr4_settings_test PRIVATE ${RR4_MINGW_STATIC})
    endif()
    add_test(NAME settings COMMAND rr4_settings_test ${CMAKE_CURRENT_BINARY_DIR}/settings-test)
    set_tests_properties(settings PROPERTIES ENVIRONMENT "PSYZ_HEADLESS=1;SDL_AUDIODRIVER=dummy")
    # Memory card saves through the game's file services, preferences and a
    # synthetic disc image under a non-ASCII (UTF-8) directory.
    add_executable(rr4_host_files_test tests/port/host_files_test.c)
    target_include_directories(rr4_host_files_test SYSTEM PRIVATE ${PSYZ_DIR}/psyz/include)
    target_compile_definitions(rr4_host_files_test PRIVATE __psyz)
    target_link_libraries(rr4_host_files_test PRIVATE rr4_host rr4_game psyz)
    if(MINGW)
        target_link_options(rr4_host_files_test PRIVATE ${RR4_MINGW_STATIC})
    endif()
    add_test(NAME host-files COMMAND rr4_host_files_test ${CMAKE_CURRENT_BINARY_DIR}/host-files-test)
    set_tests_properties(host-files PROPERTIES ENVIRONMENT "PSYZ_HEADLESS=1;SDL_AUDIODRIVER=dummy")
    # A CUE path outside the Windows ANSI code page reaches PsyZ intact
    # (UTF-16 command line): the error names it in UTF-8.
    add_test(NAME startup-utf8-disc-path
        COMMAND ridge-racer-type-4-pc "${CMAKE_CURRENT_BINARY_DIR}/Zoë 日本/missing.cue")
    set_tests_properties(startup-utf8-disc-path PROPERTIES
        ENVIRONMENT "PSYZ_HEADLESS=1;SDL_AUDIODRIVER=dummy;RR4_DISC="
        PASS_REGULAR_EXPRESSION "cannot open disc image .*Zoë 日本/missing\\.cue")
    # Startup without a disc image: a headless run prints its usage.
    add_test(NAME startup-usage COMMAND ridge-racer-type-4-pc)
    set_tests_properties(startup-usage PROPERTIES
        ENVIRONMENT "PSYZ_HEADLESS=1;SDL_AUDIODRIVER=dummy;RR4_DISC="
        PASS_REGULAR_EXPRESSION "usage: .*\\.cue")
endif()
