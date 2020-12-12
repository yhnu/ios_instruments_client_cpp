cmake_minimum_required (VERSION 3.12)

#if use vcpkg
#cmake -DCMAKE_TOOLCHAIN_FILE=I:\vcpkg\scripts\buildsystems\vcpkg.cmake ..
#

#specify project
set(PROJECT "instrument")

project(${PROJECT})

SET(CMAKE_CXX_FLAGS "-std=c++14")
# The following folder will be included
add_definitions(-DPACKAGE_URL="https://libimobiledevice.org")
add_definitions(-DPACKAGE_BUGREPORT="https://github.com/libimobiledevice/libimobiledevice/issues")
add_definitions(-DPACKAGE_VERSION="1.0")

include_directories(
    ${PROJECT_SOURCE_DIR}
    /Users/happyyi/Documents/2020-12/perfcat_build_scripts/built/darwin-x86_64/include
)

message(${PROJECT_SOURCE_DIR})

add_executable(${PROJECT} dxtmsg_test.cpp dxtmsg.cpp)
target_link_directories(${PROJECT} PUBLIC /Users/happyyi/Documents/2020-12/perfcat_build_scripts/built/darwin-x86_64/lib)
target_link_libraries(${PROJECT} libimobiledevice-1.0.a)

