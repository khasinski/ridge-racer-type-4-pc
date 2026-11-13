# Writes a module-definition (.def) file that exports every global of a
# static library from the MSVC executable linking it, for GetProcAddress
# (RR4_WATCH / RR4_POKE, src/port/diagnostics.c):
#   cmake -DLINKER=<link.exe> -DLIBRARY=<rr4_game.lib> -DOUT=<file.def> -P msvc_exports.cmake
# CMake's WINDOWS_EXPORT_ALL_SYMBOLS cannot be used: it reads only the
# executable's own objects and skips communal symbols, which is what MSVC
# makes of C's uninitialised globals ("int x;"), most of the game's state.
# Exporting a symbol makes the linker take its object from the library.
execute_process(COMMAND "${LINKER}" /dump /symbols "${LIBRARY}"
    OUTPUT_VARIABLE dump RESULT_VARIABLE result)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "msvc_exports: ${LINKER} /dump /symbols ${LIBRARY} failed (${result})")
endif()
# "01A 00000004 UNDEF  notype       External     | FrameCounter"  (communal: a size)
# "01B 00000000 SECT5  notype ()    External     | MainLoop"      (a function)
string(REGEX MATCHALL "[0-9A-F]+ [0-9A-F]+ (SECT[0-9A-F]+|UNDEF) +notype( \\(\\))? +External +\\| [^ \r\n]+"
    lines "${dump}")
set(functions "")
set(data "")
foreach(line IN LISTS lines)
    string(REGEX MATCH "^[0-9A-F]+ ([0-9A-F]+) (SECT[0-9A-F]+|UNDEF) +notype( \\(\\))? +External +\\| ([^ \r\n]+)" _ "${line}")
    set(value "${CMAKE_MATCH_1}")
    set(section "${CMAKE_MATCH_2}")
    set(call "${CMAKE_MATCH_3}")
    set(name "${CMAKE_MATCH_4}")
    if(name MATCHES "^[?@.$]" OR name MATCHES "^__")
        continue() # C++ names, compiler and linker internals
    endif()
    if(section STREQUAL "UNDEF")
        if(value MATCHES "^0+$")
            continue() # a reference, not a definition
        endif()
        list(APPEND data "${name}")
    elseif(call)
        list(APPEND functions "${name}")
    else()
        list(APPEND data "${name}")
    endif()
endforeach()
list(REMOVE_DUPLICATES functions)
list(REMOVE_DUPLICATES data)
if(data)
    list(REMOVE_ITEM data ${functions})
endif()
set(text "EXPORTS\n")
foreach(name IN LISTS functions)
    string(APPEND text "    ${name}\n")
endforeach()
foreach(name IN LISTS data)
    string(APPEND text "    ${name} DATA\n")
endforeach()
list(LENGTH functions nf)
list(LENGTH data nd)
message(STATUS "msvc_exports: ${nf} functions, ${nd} data symbols")
file(WRITE "${OUT}" "${text}")
