cmake_minimum_required(VERSION 3.10)

include(FetchContent)
include(cmake/AddShader.cmake)

project(xmbshell)

option(DOWNLOAD_DEPENDENCIES_SHALLOW "Download dependency git repositories shallow" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)
find_package(SDL2 REQUIRED)
find_package(glm REQUIRED)
find_package(spdlog REQUIRED)
find_package(Freetype REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(GIOMM giomm-2.68 REQUIRED)

file(GLOB_RECURSE sources src/*.cpp src/*.h)
file(GLOB_RECURSE opt_sources opt/*.cpp opt/*.h)
file(GLOB_RECURSE shaders shaders/*.vert shaders/*.frag shaders/*.geom)

FetchContent_Declare(
  libspng
  GIT_REPOSITORY https://github.com/randy408/libspng.git
  GIT_SHALLOW    ${DOWNLOAD_DEPENDENCIES_SHALLOW}
  GIT_TAG        v0.7.4
)
FetchContent_Declare(
  VulkanMemoryAllocator-Hpp
  GIT_REPOSITORY https://github.com/YaaZ/VulkanMemoryAllocator-Hpp.git
  GIT_TAG        master
  GIT_SHALLOW    ${DOWNLOAD_DEPENDENCIES_SHALLOW}
  GIT_SUBMODULES_RECURSE FALSE
  PATCH_COMMAND  patch --forward -i ${CMAKE_CURRENT_SOURCE_DIR}/VulkanMemoryAllocator-Hpp.patch || true
)
FetchContent_Declare(
  sdbus-cpp
  GIT_REPOSITORY https://github.com/Kistler-Group/sdbus-cpp.git
  GIT_TAG        v1.5.0
  GIT_SHALLOW    ${DOWNLOAD_DEPENDENCIES_SHALLOW}
)
FetchContent_MakeAvailable(libspng VulkanMemoryAllocator-Hpp sdbus-cpp)

target_compile_options(spng PRIVATE -O3)

add_executable(xmbshell ${sources})

add_library(optimized_components STATIC ${opt_sources})
target_include_directories(optimized_components PRIVATE include/)
target_link_libraries(optimized_components PRIVATE VulkanMemoryAllocator-Hpp)
target_compile_options(optimized_components PRIVATE -O3)

target_include_directories(xmbshell PRIVATE include/)
target_include_directories(xmbshell PRIVATE ${GLM_INCLUDE_DIRS})
target_include_directories(xmbshell PRIVATE ${GIOMM_INCLUDE_DIRS})

target_link_libraries(xmbshell PRIVATE Vulkan::Vulkan)
target_link_libraries(xmbshell PRIVATE VulkanMemoryAllocator-Hpp)
target_link_libraries(xmbshell PRIVATE SDL2::SDL2)
target_link_libraries(xmbshell PRIVATE spdlog::spdlog)
target_link_libraries(xmbshell PRIVATE dl)
target_link_libraries(xmbshell PRIVATE sdbus-c++)
target_link_libraries(xmbshell PRIVATE spng)
target_link_libraries(xmbshell PRIVATE Freetype::Freetype)
target_link_libraries(xmbshell PRIVATE optimized_components)
target_link_libraries(xmbshell PRIVATE ${GIOMM_LIBRARIES})

foreach(shader ${shaders})
	add_shader(xmbshell ${shader})
endforeach()
target_include_directories(xmbshell PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/shaders)
