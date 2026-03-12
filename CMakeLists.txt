cmake_minimum_required(VERSION 3.22)
# ---------------------------------------------------------------------------
# Custom configs for Visual Studio (must be BEFORE project())
# NOTE: Don't use if(MSVC) here, MSVC is not known yet.
# ---------------------------------------------------------------------------
set(HI_CUSTOM_CONFIGS "ReleaseMini;ReleaseNoConsole;ReleaseMiniNoConsole")
if(CMAKE_GENERATOR MATCHES "Visual Studio" OR CMAKE_GENERATOR MATCHES "Ninja Multi-Config")
    set(CMAKE_CONFIGURATION_TYPES
        "Debug;Release;${HI_CUSTOM_CONFIGS}"
        CACHE STRING "Available build configurations" FORCE
    )
endif()

project(higui LANGUAGES CXX)
# ---------------------------------------------------------------------------
# Seed missing per-config flag variables (must be AFTER project())
# to avoid: CMAKE_EXE_LINKER_FLAGS_<CFG> not set
# ---------------------------------------------------------------------------
if(CMAKE_GENERATOR MATCHES "Visual Studio")
    foreach(cfg IN ITEMS ReleaseMini ReleaseNoConsole ReleaseMiniNoConsole)
        string(TOUPPER "${cfg}" CFGU)

        foreach(var IN ITEMS
            CMAKE_C_FLAGS
            CMAKE_CXX_FLAGS
            CMAKE_EXE_LINKER_FLAGS
            CMAKE_SHARED_LINKER_FLAGS
            CMAKE_MODULE_LINKER_FLAGS
        )
            # If not defined, copy from Release as baseline.
            if(NOT DEFINED ${var}_${CFGU})
                set(${var}_${CFGU} "${${var}_RELEASE}" CACHE STRING "" FORCE)
            endif()
        endforeach()
    endforeach()
endif()
# Options
option(LEGACY_CPP14 "Use legacy C++14 compiler (default ON)" ON)
option(BUILD_SHARED_LIBS "Build shared libs" OFF)
# Language standard
if(LEGACY_CPP14)
    set(CMAKE_CXX_STANDARD 14)
else()
    set(CMAKE_CXX_STANDARD 20)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# Config groups
set(FREESTANDING_CONFIGS ReleaseMini ReleaseNoConsole ReleaseMiniNoConsole)
# Helper generator expression: true when freestanding config
set(_GE_FREESTANDING
    $<OR:$<CONFIG:ReleaseMini>,$<CONFIG:ReleaseNoConsole>,$<CONFIG:ReleaseMiniNoConsole>>
)
# Flags (cleanly separated: common vs freestanding-only)
set(msvc_common_compile
    "/permissive-"
    "/diagnostics:column"
    "/nologo"
)
# Freestanding base (applies to all Release* freestanding configs)
set(msvc_freestanding_base
    "/TP"
    "/W3"
    "/FC"
    "/GS-"
    "/GR-"
    "/Zl"
    "/Zc:threadSafeInit-"
    "/Oi-"     # disable intrinsics (essential for freestanding)
    "/EHs-"    # disable C++ exceptions (no unwind tables)
    "/EHc-"    # no extern "C" unwind
    "/Gm-"
    "/Gy"
    "/Zc:inline"
    "/Zc:wchar_t"
    "/Zc:forScope"
    "/sdl-"
    "/errorReport:none"
    "/WX-"
    "/D;NDEBUG"
    "/D_HAS_EXCEPTIONS=0"
)
# Freestanding "full" (ReleaseNoConsole)
set(msvc_freestanding_full_opt
    "/O2"
    "/Ob2"
    "/fp:precise"
    "/Ot"
    "/Oy"
)
# Freestanding "mini" (ReleaseMini, ReleaseMiniNoConsole)
set(msvc_freestanding_mini_opt
    "/O1"
    "/Ob1"
    "/fp:fast"
    "/Os"
    "/Oy"
)
# Debug
set(msvc_debug_compile
    "/Zi"
    "/W3"
    "/GR"
    "/Od"
)
# GCC/Clang
set(unix_common_compile
    "-pipe"
)
set(unix_freestanding_base
    "-ffunction-sections"
    "-fdata-sections"
    "-fno-exceptions"
    "-fno-rtti"
    "-fno-math-errno"
    "-fno-signaling-nans"
    "-fomit-frame-pointer"
)
set(unix_freestanding_full_opt
    "-O3"
    "-flto"
    "-ffast-math"
)
set(unix_freestanding_mini_opt
    "-Os"
    "-ffunction-sections"
    "-fdata-sections"
    "-fno-exceptions"
    "-fno-rtti"
    "-fno-unwind-tables"
    "-fno-asynchronous-unwind-tables"
    "-fno-ident"
    "-fvisibility=hidden"
    "-fvisibility-inlines-hidden"
)
set(unix_debug_compile
    "-O0"
    "-g"
    "-fno-omit-frame-pointer"
)
set(unix_hosted_link_opt
    "-Wl,--gc-sections"
    "-Wl,--hash-style=gnu"
    "-Wl,--build-id=none"   # optional
    "-Wl,-z,relro"          # optional security
    "-Wl,-z,now"            # optional security (slower startup)
)

# -----------------------------------------------------------------------------
# Common include root
# -----------------------------------------------------------------------------
set(PROJECT_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/src/include")

# -----------------------------------------------------------------------------
# Functions
# -----------------------------------------------------------------------------
function(hi_apply_common target)
    target_include_directories(${target} PRIVATE $<BUILD_INTERFACE:${PROJECT_INCLUDE_DIR}>)
    target_compile_definitions(${target} PRIVATE
        $<$<CONFIG:Release>:NDEBUG>
        # IO_HAS_STD when NOT freestanding
        $<$<NOT:${_GE_FREESTANDING}>:IO_HAS_STD=1>
    )
    # compiler-id common flags
    target_compile_options(${target} PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:${msvc_common_compile}>
        $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:${unix_common_compile}>
    )
    # MSVC linker opts for all Release-like configs
    if(MSVC)
        target_link_options(${target} PRIVATE
            $<$<OR:$<CONFIG:Release>,$<CONFIG:ReleaseMini>,$<CONFIG:ReleaseNoConsole>,$<CONFIG:ReleaseMiniNoConsole>>:/OPT:REF>
            $<$<OR:$<CONFIG:Release>,$<CONFIG:ReleaseMini>,$<CONFIG:ReleaseNoConsole>,$<CONFIG:ReleaseMiniNoConsole>>:/OPT:ICF>
        )
    endif()
endfunction()

function(hi_apply_freestanding target)
    # MSVC: apply ONLY for freestanding configs
    target_compile_options(${target} PRIVATE
        $<$<AND:$<CXX_COMPILER_ID:MSVC>,${_GE_FREESTANDING}>:${msvc_freestanding_base}>
        $<$<AND:$<CXX_COMPILER_ID:MSVC>,${_GE_FREESTANDING}>:/EHs- /EHc- /D_HAS_EXCEPTIONS=0>

        $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:ReleaseNoConsole>>:${msvc_freestanding_full_opt}>
        $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:ReleaseMini>>:${msvc_freestanding_mini_opt}>
        $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:ReleaseMiniNoConsole>>:${msvc_freestanding_mini_opt}>
    )

    if(MSVC)
        # 1) DO NOT select CRT runtime for freestanding
        #    (so CMake/MSVC won't inject /DEFAULTLIB:libcpmt/msvcrt)
        set_property(TARGET ${target} PROPERTY MSVC_RUNTIME_LIBRARY
          "$<IF:${_GE_FREESTANDING},MultiThreaded,$<IF:$<CONFIG:Debug>,MultiThreadedDebug,MultiThreadedDLL>>"
        )

        # 2) No default libs at link time for freestanding configs
        target_link_options(${target} PRIVATE
          $<$<AND:${_GE_FREESTANDING},$<CXX_COMPILER_ID:MSVC>>:/NODEFAULTLIB>
        )


        # 3) Keep only what you explicitly link (Win32 API libs etc.)
        target_link_libraries(${target} PRIVATE user32 gdi32 opengl32 kernel32 bcrypt ws2_32)
    endif()

    # GCC/Clang freestanding compile options
    target_compile_options(${target} PRIVATE
        $<$<AND:$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>,${_GE_FREESTANDING}>:${unix_freestanding_base}>

        $<$<AND:$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>,$<CONFIG:ReleaseNoConsole>>:${unix_freestanding_full_opt}>
        $<$<AND:$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>,$<CONFIG:ReleaseMini>>:${unix_freestanding_mini_opt}>
        $<$<AND:$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>,$<CONFIG:ReleaseMiniNoConsole>>:${unix_freestanding_mini_opt}>
    )

    # GCC/Clang no-std
    get_property(_hosted TARGET ${target} PROPERTY HI_HOSTED_LINK)
    # linux: not hosted link
    if(UNIX AND NOT APPLE AND NOT _hosted)
      target_link_options(${target} PRIVATE
        $<$<AND:${_GE_FREESTANDING},$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>>:
          -s -nostdlib -nostartfiles -nodefaultlibs -fno-stack-protector -fno-unwind-tables -fno-asynchronous-unwind-tables
          -Wl,--hash-style=gnu
          -Wl,--build-id=none
          -Wl,--gc-sections
          -Wl,-e,_start
          -Wl,-z,norelro
        >
      )
    endif()
    # linux: hosted link
    if(UNIX AND NOT APPLE AND _hosted)
        target_link_options(${target} PRIVATE
            $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:${unix_hosted_link_opt}>
        )
    endif()
endfunction()

function(hi_apply_debug target)
    target_compile_options(${target} PRIVATE
        $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Debug>>:${msvc_debug_compile}>
        $<$<AND:$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>,$<CONFIG:Debug>>:${unix_debug_compile}>
    )
endfunction()

function(hi_console target)
    if(MSVC)
        target_link_options(${target} PRIVATE
            $<$<CONFIG:ReleaseNoConsole>:/SUBSYSTEM:WINDOWS>
            $<$<CONFIG:ReleaseNoConsole>:/ENTRY:WinMain>
            $<$<CONFIG:ReleaseMiniNoConsole>:/SUBSYSTEM:WINDOWS>
            $<$<CONFIG:ReleaseMiniNoConsole>:/ENTRY:WinMain>
        )
    endif()

    # CONSOLE define only for configs where it set
    target_compile_definitions(${target} PRIVATE
        $<$<OR:$<CONFIG:Debug>,$<CONFIG:Release>,$<CONFIG:ReleaseMini>>:CONSOLE>
    )
endfunction()

function(hi_add_header_only name)
    add_library(${name} INTERFACE)
    target_include_directories(${name} INTERFACE
        $<BUILD_INTERFACE:${PROJECT_INCLUDE_DIR}>
    )
    # to show '.hpp' in IDE:
    target_sources(${name} INTERFACE ${ARGN})
endfunction()

function(hi_add_test_exe name)
    # usage: hi_add_test_exe(target cpp_file [headers...])
    set(options)
    set(oneValueArgs CPP)
    set(multiValueArgs HEADERS LIBS)
    cmake_parse_arguments(T "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    add_executable(${name} ${T_CPP} ${T_HEADERS})
    hi_console(${name})

    if(T_LIBS)
        target_link_libraries(${name} PRIVATE ${T_LIBS})
    endif()

    hi_apply_common(${name})
    hi_apply_freestanding(${name})
    hi_apply_debug(${name})
endfunction()

function(hi_add_gui_exe name)
    # usage: hi_add_gui_exe(target cpp_file [headers...] [libs...])
    set(options)
    set(oneValueArgs CPP)
    set(multiValueArgs HEADERS LIBS)
    cmake_parse_arguments(T "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    add_executable(${name} ${T_CPP} ${T_HEADERS})
    set_property(TARGET ${name} PROPERTY HI_HOSTED_LINK 1)
    target_compile_definitions(${name} PRIVATE
        HI_GUI_APP=1
    )
    if(T_LIBS)
        target_link_libraries(${name} PRIVATE ${T_LIBS})
    endif()
    hi_apply_common(${name})
    hi_apply_freestanding(${name})
    hi_apply_debug(${name})
    hi_console(${name})
    # --- Windows: GUI subsystem for "no console" configs ---
    if(MSVC)
        target_link_options(${name} PRIVATE
            $<$<CONFIG:ReleaseNoConsole>:/SUBSYSTEM:WINDOWS>
            $<$<CONFIG:ReleaseNoConsole>:/ENTRY:WinMain>
            $<$<CONFIG:ReleaseMiniNoConsole>:/SUBSYSTEM:WINDOWS>
            $<$<CONFIG:ReleaseMiniNoConsole>:/ENTRY:WinMain>
        )
    endif()
    # --- Linux: link X11 + GL only when needed (typical for your GUI targets) ---
    if(UNIX AND NOT APPLE)
        # NOTE: for true -nostdlib freestanding you must provide these as explicit libs,
        # and ensure you have syscalls/loader logic. Adjust if you keep freestanding.
        target_link_libraries(${name} PRIVATE
            $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:X11 GL>
        )
    endif()
endfunction()

# -----------------------------------------------------------------------------
# Sources
# -----------------------------------------------------------------------------

set(SOURCES_IO "hi/io.hpp")
set(SOURCES_HI "hi/hi.hpp")
set(SOURCES_AUDIO
    "hi/audiocontext.hpp"
)
set(TEST_SOURCES_IO_HI
    ${SOURCES_IO}
    ${SOURCES_HI}
    "examples/tests/test_socket.hpp" # general convience functions for socket tests

    "examples/tests/catch.hpp" # 3rd party
    "examples/tests/atomic.hpp"
    "examples/tests/containers.hpp"
    "examples/tests/file.hpp"
    "examples/tests/filesystem.hpp"
    "examples/tests/syscalls.hpp"
    "examples/tests/types.hpp"
)
set(SOURCES_CRYPTO
    "hi/crypto/aead_chacha20_poly1305.hpp"
    "hi/crypto/chacha20.hpp"
    "hi/crypto/poly1305.hpp"
    "hi/crypto/sha256.hpp"
    "hi/crypto/sha384.hpp"
    "hi/crypto/x25519.hpp")
set(TEST_SOURCES_CRYPTO
    "examples/tests/test_aead_chacha20_poly1305.hpp"
    "examples/tests/test_chacha20.hpp"
    "examples/tests/test_poly1305.hpp"
    "examples/tests/test_sha256.hpp"
    "examples/tests/test_sha384.hpp"
    "examples/tests/test_x25519.hpp")

# -----------------------------------------------------------------------------
# Header-only interface targets
# -----------------------------------------------------------------------------
hi_add_header_only(io ${SOURCES_IO})
hi_add_header_only(hi ${SOURCES_HI})
hi_add_header_only(crypto ${SOURCES_CRYPTO})
hi_add_header_only(audio ${SOURCES_AUDIO})

# -----------------------------------------------------------------------------
# Freestanding examples
# -----------------------------------------------------------------------------

# client
hi_add_test_exe(client
    CPP "examples/client.cpp"
    HEADERS ${SOURCES_IO}
    LIBS io
)

# server
hi_add_test_exe(server
    CPP "examples/server.cpp"
    HEADERS ${SOURCES_IO}
    LIBS io
)

# -----------------------------------------------------------------------------
# General tests
# -----------------------------------------------------------------------------

# io + hi tests
hi_add_test_exe(test_io_hi
    CPP "examples/hi.cpp"
    HEADERS ${TEST_SOURCES_IO_HI}
    LIBS io hi
)

# client-server tests
hi_add_test_exe(test_server
    CPP "examples/tests/test_server.cpp"
    HEADERS ${TEST_SOURCES_IO_HI}
    LIBS io hi
)
hi_add_test_exe(test_client
    CPP "examples/tests/test_client.cpp"
    HEADERS ${TEST_SOURCES_IO_HI}
    LIBS io hi
)

# crypto
hi_add_test_exe(test_crypto
    CPP "examples/crypto.cpp"
    HEADERS ${TEST_SOURCES_CRYPTO}
    LIBS io crypto
)

# thread
hi_add_test_exe(test_thread
    CPP "examples/tests/thread.cpp"
    HEADERS ${SOURCES_IO}
    LIBS io
)

# audio
hi_add_test_exe(test_audio
    CPP "examples/tests/test_audio.cpp"
    HEADERS ${SOURCES_AUDIO}
    LIBS io audio winmm
)

hi_add_test_exe(test_wav
    CPP "examples/tests/test_wav.cpp"
    HEADERS ${SOURCES_AUDIO} ${SOURCES_IO}
    LIBS io audio winmm
)

hi_add_test_exe(test_fade_in_out
    CPP "examples/tests/test_fade_in_out.cpp"
    HEADERS ${SOURCES_AUDIO} ${SOURCES_IO}
    LIBS io audio winmm
)

hi_add_test_exe(test_audio_mixer
    CPP "examples/tests/test_audio_mixer.cpp"
    HEADERS ${SOURCES_AUDIO} ${SOURCES_IO}
    LIBS io audio winmm
)

hi_add_test_exe(test_audio_scheduler
    CPP "examples/tests/test_audio_scheduler.cpp"
    HEADERS ${SOURCES_AUDIO} ${SOURCES_IO}
    LIBS io audio winmm
)

# thread VTune threading test
hi_add_test_exe(threadpool_vtune
    CPP "examples/threadpool_vtune.cpp"
    HEADERS ${SOURCES_IO}
    LIBS io
)

# -----------------------------------------------------------------------------
# GUI Applications
# -----------------------------------------------------------------------------

hi_add_gui_exe(gl
    CPP "examples/gl.cpp"
    HEADERS ${SOURCES_HI}
    LIBS io hi
)

hi_add_gui_exe(test_gl
    CPP "examples/tests/opengl.cpp"
    HEADERS ${SOURCES_HI}
    LIBS io hi
)

message(STATUS "Configured ${PROJECT_NAME} (C++${CMAKE_CXX_STANDARD})")
message(STATUS "Freestanding configs: ${FREESTANDING_CONFIGS}")
