ez_cmake_init()

ez_requires(EZ_3RDPARTY_TRACY_SUPPORT)

find_package(Threads)

ez_create_target(LIBRARY TracyClient NO_EZ_PREFIX NO_UNITY MANUAL_SOURCE_FILES TracyClient.cpp init.cmake)

target_compile_definitions(TracyClient PUBLIC BUILDSYSTEM_ENABLE_TRACY_SUPPORT)

set_property(TARGET TracyClient PROPERTY CXX_STANDARD 11)

target_include_directories(TracyClient SYSTEM PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>)

# NOTE: Linking to Threads for windows seems to be problematic for Editor Tests, so it will be disabled for WIN32.
if(Threads_FOUND AND NOT WIN32)
    target_link_libraries(TracyClient
        PUBLIC
        Threads::Threads
        ${CMAKE_DL_LIBS}
    )
else()
    target_link_libraries(TracyClient
        PUBLIC
        ${CMAKE_DL_LIBS}
    )
endif()

# Public dependency on some libraries required when using MinGw
if(WIN32 AND ${CMAKE_CXX_COMPILER_ID} MATCHES "GNU")
    target_link_libraries(TracyClient PUBLIC ws2_32 dbghelp)
endif()

if(CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
    find_library(EXECINFO_LIBRARY NAMES execinfo REQUIRED)
    target_link_libraries(TracyClient PUBLIC ${EXECINFO_LIBRARY})
endif()

add_library(Tracy::TracyClient ALIAS TracyClient)

macro(set_option option help value)
    option(${option} ${help} ${value})
    mark_as_advanced(${option})

    if(${option})
        message(STATUS "${option}: ON")
        target_compile_definitions(TracyClient PUBLIC ${option})
    else()
        message(STATUS "${option}: OFF")
    endif()
endmacro()


if (EZ_3RDPARTY_TRACY_TRACK_ALLOCATIONS)
  # attempt to prevent Tracy from locking the PDB files
  # doesn't work, though, it still opens and locks the PDB files, which is why it is now disabled by default
  target_compile_definitions(TracyClient PUBLIC TRACY_ENABLE_MEMORY_TRACKING)
  set(TRACY_NO_CALLSTACK OFF CACHE BOOL "Disable all callstack related functionality" FORCE)
  set(TRACY_NO_CALLSTACK_INLINES OFF CACHE BOOL "Disables the inline functions in callstacks" FORCE)
  set(TRACY_NO_SAMPLING OFF CACHE BOOL "Disable call stack sampling" FORCE)
else()
  set(TRACY_NO_CALLSTACK ON CACHE BOOL "Disable all callstack related functionality" FORCE)
  set(TRACY_NO_CALLSTACK_INLINES ON CACHE BOOL "Disables the inline functions in callstacks" FORCE)
  set(TRACY_NO_SAMPLING ON CACHE BOOL "Disable call stack sampling" FORCE)
endif()

# NOTE: Custom Tracy Options.
set_option(TRACY_ENABLE "Enable profiling" ON)
set_option(TRACY_ON_DEMAND "On-demand profiling" ON)
set_option(TRACY_CALLSTACK "Enforce callstack collection for tracy regions" OFF)
set_option(TRACY_NO_CALLSTACK "Disable all callstack related functionality" OFF)
set_option(TRACY_NO_CALLSTACK_INLINES "Disables the inline functions in callstacks" OFF)
set_option(TRACY_ONLY_LOCALHOST "Only listen on the localhost interface" OFF)
set_option(TRACY_NO_BROADCAST "Disable client discovery by broadcast to local network" OFF)
set_option(TRACY_ONLY_IPV4 "Tracy will only accept connections on IPv4 addresses (disable IPv6)" OFF)
set_option(TRACY_NO_CODE_TRANSFER "Disable collection of source code" OFF)
set_option(TRACY_NO_CONTEXT_SWITCH "Disable capture of context switches" OFF)
set_option(TRACY_NO_EXIT "Client executable does not exit until all profile data is sent to server" OFF)
set_option(TRACY_NO_SAMPLING "Disable call stack sampling" OFF)
set_option(TRACY_NO_VERIFY "Disable zone validation for C API" OFF)
set_option(TRACY_NO_VSYNC_CAPTURE "Disable capture of hardware Vsync events" OFF)
set_option(TRACY_NO_FRAME_IMAGE  "Disable the frame image support and its thread" OFF)
set_option(TRACY_NO_SYSTEM_TRACING  "Disable systrace sampling" OFF)
set_option(TRACY_PATCHABLE_NOPSLEDS  "Enable nopsleds for efficient patching by system-level tools (e.g. rr)" OFF)
set_option(TRACY_DELAYED_INIT "Enable delayed initialization of the library (init on first call)" ON)
set_option(TRACY_MANUAL_LIFETIME "Enable the manual lifetime management of the profile" OFF)
set_option(TRACY_FIBERS "Enable fibers support" OFF)
set_option(TRACY_NO_CRASH_HANDLER "Disable crash handling" OFF)
set_option(TRACY_TIMER_FALLBACK "Use lower resolution timers" OFF)
set_option(TRACY_LIBUNWIND_BACKTRACE "Use libunwind backtracing where supported" OFF)
set_option(TRACY_SYMBOL_OFFLINE_RESOLVE "Instead of full runtime symbol resolution, only resolve the image path and offset to enable offline symbol resolution" OFF)
set_option(TRACY_LIBBACKTRACE_ELF_DYNLOAD_SUPPORT "Enable libbacktrace to support dynamically loaded elfs in symbol resolution resolution after the first symbol resolve operation" OFF)

# advanced
set_option(TRACY_VERBOSE "[advanced] Verbose output from the profiler" OFF)
mark_as_advanced(TRACY_VERBOSE)
set_option(TRACY_DEMANGLE "[advanced] Don't use default demangling function - You'll need to provide your own" OFF)
mark_as_advanced(TRACY_DEMANGLE)

if(EZ_COMPILE_ENGINE_AS_DLL)
    target_compile_definitions(TracyClient PRIVATE TRACY_EXPORTS)
    target_compile_definitions(TracyClient PUBLIC TRACY_IMPORTS)
endif()
