# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include)
#LINK_DIRECTORIES(${CMAKE_SOURCE_DIR}/libs)
set(LIB_DIR ${CMAKE_SOURCE_DIR}/libs/${ANDROID_ABI})

add_library(MNN SHARED IMPORTED)
set_target_properties(
        MNN
        PROPERTIES IMPORTED_LOCATION
        ${LIB_DIR}/libMNN.so
)
add_library(MNN_EXP SHARED IMPORTED)
set_target_properties(
        MNN_EXP
        PROPERTIES IMPORTED_LOCATION
        ${LIB_DIR}/libMNN_Express.so
)


add_library(MNN_CL SHARED IMPORTED)
set_target_properties(
        MNN_CL
        PROPERTIES IMPORTED_LOCATION
        ${LIB_DIR}/libMNN_CL.so
)
add_library(MNN_GL SHARED IMPORTED)
set_target_properties(
        MNN_GL
        PROPERTIES IMPORTED_LOCATION
        ${LIB_DIR}/libMNN_GL.so
)
add_library(MNN_VULKAN SHARED IMPORTED)
set_target_properties(
        MNN_VULKAN
        PROPERTIES IMPORTED_LOCATION
        ${LIB_DIR}/libMNN_VULKAN.so
)


add_library(opencv_java4 SHARED IMPORTED)
set_target_properties(
        opencv_java4
        PROPERTIES IMPORTED_LOCATION
        ${LIB_DIR}/libopencv_java4.so
)


#add_library(OpenCL SHARED IMPORTED)
#set_target_properties(
#        OpenCL
#        PROPERTIES IMPORTED_LOCATION
#        ${LIB_DIR}/libOpenCL.so
#)

file(GLOB SOURCE ${CMAKE_SOURCE_DIR}/*.cpp ${CMAKE_SOURCE_DIR}/*.h)
add_library( # Sets the name of the library.
        siamtracker

        # Sets the library as a shared library.
        SHARED

        # Provides a relative path to your source file(s).
        ${SOURCE})

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
        log-lib

        # Specifies the name of the NDK library that
        # you want CMake to locate.
        log)

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.
target_link_libraries( # Specifies the target library.
        siamtracker
        # Links the target library to the log library
        # included in the NDK.
        MNN
        MNN_CL
        MNN_GL
        MNN_VULKAN
        opencv_java4
        ${log-lib}
        )