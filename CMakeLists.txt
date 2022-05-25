cmake_minimum_required(VERSION 3.21) # C++ 20

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(DEFINED ENV{VCPKG_DIR} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE  "$ENV{VCPKG_DIR}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

project("ThisProject")

add_compile_definitions(PROJECT_ROOT_LOCATION="${CMAKE_SOURCE_DIR}/")

add_compile_options(/EHa)

message("||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||")
message("||||||||||||||||||||||| THIS IS ${CMAKE_CXX_COMPILER_ID} \tCOMPILER ||||||||||||||||||||||")
message("||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||")

add_subdirectory(Instancing)
# add_subdirectory(temp)