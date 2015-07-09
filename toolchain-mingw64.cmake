# Toolchain file for doing 32 bit Windows cross-compiles.
# Call with 'cmake -DCMAKE_TOOLCHAIN_FILE=toolchain-mingw64.cmake -DENABLE_ALSA=OFF .'
SET(CMAKE_SYSTEM_NAME Windows)

SET(CMAKE_C_COMPILER i686-w64-mingw32-gcc)
SET(CMAKE_RC_COMPILER i686-w64-mingw32-windres)

SET(CMAKE_FIND_ROOT_PATH /usr/i686-w64-mingw32)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Set preprocessor flag for Windows reversed directory separator character
ADD_DEFINITIONS (-DBELL_WINDOWS)
