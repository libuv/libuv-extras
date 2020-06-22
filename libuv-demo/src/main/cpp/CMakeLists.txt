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
ExternalProject_Add(libuv_ext
        GIT_REPOSITORY https://github.com/libuv/libuv.git
        INSTALL_COMMAND ""
        CMAKE_ARGS ${ANDROID_CMAKE_ARGS}
        BUILD_BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/libuv_ext-prefix/src/libuv_ext-build/libuv_a${CMAKE_STATIC_LIBRARY_SUFFIX}
        GIT_TAG ${LIBUV_VERSION})

ExternalProject_Get_property(libuv_ext SOURCE_DIR)
ExternalProject_Get_property(libuv_ext BINARY_DIR)
MESSAGE(WARNING "${BUILD_BYPRODUCTS}")

add_library(libuv STATIC IMPORTED GLOBAL)
set_target_properties(libuv PROPERTIES
  IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/libuv_ext-prefix/src/libuv_ext-build/libuv_a${CMAKE_STATIC_LIBRARY_SUFFIX}
)
add_dependencies(libuv libuv_ext)
include_directories(${SOURCE_DIR}/include)

add_library( native-lib SHARED native-lib.cpp )
add_dependencies(native-lib libuv)

find_library( log-lib log )

link_directories(${BINARY_DIR})

target_link_libraries(native-lib ${log-lib} libuv)


