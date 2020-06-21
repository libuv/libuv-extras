cmake_minimum_required(VERSION 3.10)

project (native-lib)

set(LIBUV_VERSION v1.38.0)

include(ExternalProject)

# https://stackoverflow.com/questions/48132569/cmake-externalproject-superbuild-isnt-executed-with-android-ndk
# Forward Android CMake cross compile args to ExternalProject.
set(ANDROID_CMAKE_ARGS
        -DANDROID_ABI=${ANDROID_ABI}
        -DANDROID_PLATFORM=${ANDROID_PLATFORM}
        -DCMAKE_LIBRARY_OUTPUT_DIRECTORY=${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DANDROID_NDK=${ANDROID_NDK}
        -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
        -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
        -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
        -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
        -G${CMAKE_GENERATOR})

ExternalProject_Add(libuv
        GIT_REPOSITORY https://github.com/libuv/libuv.git
        INSTALL_COMMAND ""
        # ninja hack
        BUILD_BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}
        CMAKE_ARGS ${ANDROID_CMAKE_ARGS}
        GIT_TAG ${LIBUV_VERSION})

ExternalProject_Get_property(libuv SOURCE_DIR)
ExternalProject_Get_property(libuv BINARY_DIR)
message(WARNING "message: ${BINARY_DIR}")

add_library( native-lib SHARED native-lib.cpp )

find_library( log-lib log )

include_directories(${SOURCE_DIR}/include)
link_directories(${BINARY_DIR})

add_dependencies(native-lib libuv)

set(libuv_static ${BINARY_DIR}/libuv_a.a)
target_link_libraries(native-lib ${log-lib} ${libuv_static})
