cmake_minimum_required(VERSION 3.10)

project(xpbd_engine)

find_package(assimp CONFIG REQUIRED)

# Include packages from vcpkg
include_directories(${PACKAGE_PREFIX_DIR}/include)

add_executable(xpbd_engine main.cpp)
target_link_libraries(xpbd_engine PRIVATE assimp::assimp)

# get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
# foreach(dir ${dirs})
#   message(STATUS "dir='${dir}'")
# endforeach()
# message("CMAKE_PREFIX_PATH" ${CMAKE_PREFIX_PATH})
# message("PACKAGE_PREFIX_DIR" ${PACKAGE_PREFIX_DIR})
