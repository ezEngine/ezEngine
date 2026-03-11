set(CMAKE_SYSTEM_NAME Linux)

message("sdk :'$ENV{EZ_STEAMRT_SDK_ROOT}'")
if(NOT IS_DIRECTORY "$ENV{EZ_STEAMRT_SDK_ROOT}")
  message(FATAL_ERROR "EZ_STEAMRT_SDK_ROOT environment variable not set or the directory it points to is not valid. Please define it to point to the steamrt sniper SDK root.")
endif()

# Tell cmake where the sdk root directory is
set(CMAKE_SYSROOT $ENV{EZ_STEAMRT_SDK_ROOT})
set(CMAKE_FIND_ROOT_PATH $ENV{EZ_STEAMRT_SDK_ROOT})

# Configure cmake to only look inside the steamrt sdk, and not the host system.
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Add a output directory platform postfix to differentiate with a regular linux build (editor)
set(EZ_OUTPUT_DIRECTORY_PLATFORM_POSTFIX "Steamrt")