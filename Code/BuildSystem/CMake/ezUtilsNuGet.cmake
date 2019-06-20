######################################
### NuGet support
######################################

######################################
### ez_requires_nuget()
######################################

function(ez_requires_nuget)

    find_program(NUGET nuget
        HINTS ${CMAKE_BINARY_DIR})
    if(NOT NUGET)
        message(STATUS "Downloading Nuget...")
        file(DOWNLOAD
            https://dist.nuget.org/win-x86-commandline/latest/nuget.exe
            ${CMAKE_BINARY_DIR}/nuget.exe
            SHOW_PROGRESS
        )
        find_program(NUGET nuget
            HINTS ${CMAKE_BINARY_DIR})
    endif()

endfunction()
