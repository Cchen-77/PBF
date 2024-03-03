cmake_minimum_required(VERSION 3.15)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(PBF)

if(NOT DEFINED ENV{VULKAN_SDK})
    message(FATAL_ERROR "you should have vulkan sdk")
endif()

set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/target")
set(BINARY_ROOT_DIR ${CMAKE_INSTALL_PREFIX})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BINARY_ROOT_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${BINARY_ROOT_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BINARY_ROOT_DIR}/lib)
set(CMAKE_PDB_OUTPUT_DIRECTORY ${BINARY_ROOT_DIR}/bin)


file(GLOB sources ${CMAKE_SOURCE_DIR}/src/*.cpp)
file(GLOB includes ${CMAKE_SOURCE_DIR}/include/*.h)

source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${sources} ${includes})
add_executable(PBF ${sources} ${includes})

target_include_directories(PBF PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_include_directories(PBF PUBLIC $ENV{VULKAN_SDK}/Include)
target_include_directories(PBF PUBLIC ${CMAKE_SOURCE_DIR}/3rdparty/stb)
target_include_directories(PBF PUBLIC ${CMAKE_SOURCE_DIR}/3rdparty/tinyobjloader)

target_link_directories(PBF PUBLIC $ENV{VULKAN_SDK}/Lib)

target_link_libraries(PBF PUBLIC vulkan-1)
target_link_libraries(PBF PUBLIC glfw)


add_custom_command(TARGET PBF POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different ${CMAKE_SOURCE_DIR}/resources $<TARGET_FILE_DIR:PBF>/resources)

add_subdirectory(3rdparty)

