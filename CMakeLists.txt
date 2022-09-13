cmake_minimum_required(VERSION 3.10)

project(xpbd_engine)

find_package(assimp CONFIG REQUIRED)

# Include packages from vcpkg
# include_directories(${PACKAGE_PREFIX_DIR}/include)

add_executable(xpbd_engine main.cpp)
target_link_libraries(xpbd_engine PRIVATE assimp::assimp)

# get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
# foreach(dir ${dirs})
#   message(STATUS "dir='${dir}'")
# endforeach()
# message("CMAKE_PREFIX_PATH" ${CMAKE_PREFIX_PATH})
# message("PACKAGE_PREFIX_DIR" ${PACKAGE_PREFIX_DIR})


if("${TAICHI_REPO}" STREQUAL "")
    message(FATAL_ERROR "Set TAICHI_REPO with -DTAICHI_REPO=.. ")
endif()

target_compile_options(xpbd_engine PUBLIC -DTI_WITH_VULKAN=ON -DTI_INCLUDED)

# C-API Dependencies
target_include_directories(xpbd_engine
  PRIVATE
    ${PROJECT_SOURCE_DIR}
    ${TAICHI_REPO}/c_api/include
    ${TAICHI_REPO}/c_api/src
  )

target_link_directories(xpbd_engine
  PRIVATE
    ${PROJECT_SOURCE_DIR}
    ${TAICHI_REPO}/build
  )

target_link_libraries(xpbd_engine PUBLIC taichi_c_api)

# GUI Dependencies
target_include_directories(xpbd_engine 
    PUBLIC 
        ${TAICHI_REPO}
        ${TAICHI_REPO}/taichi/backends/vulkan
        ${TAICHI_REPO}/external/Vulkan-Headers/include/
        ${TAICHI_REPO}/external/SPIRV-Tools/include/
        ${TAICHI_REPO}/external/volk/
        ${TAICHI_REPO}/external/glm/
        ${TAICHI_REPO}/external/imgui/
        ${TAICHI_REPO}/external/glfw/include
        ${TAICHI_REPO}/external/imgui/backends
        ${TAICHI_REPO}/external/eigen/
        ${TAICHI_REPO}/external/spdlog/include/
        ${TAICHI_REPO}/external/VulkanMemoryAllocator/include/
        )
