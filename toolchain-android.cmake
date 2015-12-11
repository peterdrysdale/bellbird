# Toolchain file for doing android cross compiles.
# Call with 'cmake -DCMAKE_TOOLCHAIN_FILE=toolchain-android.cmake -DENABLE_ANDROID=YES -DENABLE_ALSA=OFF ."

SET(CMAKE_SYSTEM_NAME Android)

SET(CMAKE_SYSTEM_VERSION 1)

SET(CMAKE_C_COMPILER arm-linux-androideabi-gcc)

