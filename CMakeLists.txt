cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_FLAGS_DEBUGASAN "-g -fsanitize=address -fno-omit-frame-pointer -g" CACHE STRING "Flags used by the C++ compiler during AddressSanitizer builds." FORCE)
set(CMAKE_EXE_LINKER_FLAGS_DEBUGASAN "-fsanitize=address" CACHE STRING "Flags used for linking binaries during AddressSanitizer builds." FORCE)
mark_as_advanced(CMAKE_CXX_FLAGS_DEBUGASAN CMAKE_EXE_LINKER_FLAGS_DEBUGASAN)

include(FetchContent)
include(cmake/AddShader.cmake)

project(xmbshell VERSION 1.0 LANGUAGES CXX)

option(DOWNLOAD_DEPENDENCIES_SHALLOW "Download dependency git repositories shallow" OFF)
option(GENERATE_POT "Generate .pot file" OFF)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wno-nullability-completeness)

find_package(Vulkan REQUIRED)
find_package(SDL2 REQUIRED)
find_package(SDL2_mixer REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(glm REQUIRED)
find_package(spdlog REQUIRED)
find_package(Freetype REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(i18n++ QUIET)
find_package(Gettext REQUIRED)

if(NOT i18n++_FOUND)
  set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
  set(BUILD_PLUGIN ${GENERATE_POT} CACHE BOOL "" FORCE)
  set(BUILD_MERGE ${GENERATE_POT} CACHE BOOL "" FORCE)

  FetchContent_Declare(
    i18n++
    GIT_REPOSITORY https://github.com/JnCrMx/i18n-cpp.git
    GIT_TAG        trunk
    GIT_SUBMODULES_RECURSE FALSE
  )
  FetchContent_MakeAvailable(i18n++)
endif()

pkg_check_modules(giomm giomm-2.68 REQUIRED IMPORTED_TARGET)

file(GLOB_RECURSE sources src/*.cpp src/*.h)
file(GLOB_RECURSE opt_sources opt/*.cpp opt/*.h)
file(GLOB_RECURSE shaders shaders/*.vert shaders/*.frag shaders/*.geom)

FetchContent_Declare(
  VulkanMemoryAllocator-Hpp
  GIT_REPOSITORY https://github.com/YaaZ/VulkanMemoryAllocator-Hpp.git
  GIT_TAG        73cdd838c425637c874d343ab0ceba5148189cbf
  GIT_SHALLOW    ${DOWNLOAD_DEPENDENCIES_SHALLOW}
  GIT_SUBMODULES_RECURSE FALSE
  PATCH_COMMAND  patch --forward -i ${CMAKE_CURRENT_SOURCE_DIR}/VulkanMemoryAllocator-Hpp.patch || true
)
FetchContent_MakeAvailable(VulkanMemoryAllocator-Hpp)

FetchContent_Declare(
  sdbus-cpp
  GIT_REPOSITORY https://github.com/Kistler-Group/sdbus-cpp.git
  GIT_TAG        v1.2.0
)
FetchContent_MakeAvailable(sdbus-cpp)
add_library(SDBusCpp::sdbus-c++ ALIAS sdbus-c++)

add_executable(xmbshell ${sources})

add_library(optimized_components STATIC ${opt_sources})
target_include_directories(optimized_components PRIVATE include/)
target_link_libraries(optimized_components PRIVATE VulkanMemoryAllocator-Hpp)
target_compile_options(optimized_components PRIVATE -O3)

target_include_directories(xmbshell PRIVATE include/)
target_include_directories(xmbshell PRIVATE ${GLM_INCLUDE_DIRS})

target_link_libraries(xmbshell PRIVATE Vulkan::Vulkan)
target_link_libraries(xmbshell PRIVATE VulkanMemoryAllocator-Hpp)
target_link_libraries(xmbshell PRIVATE SDL2::SDL2)
target_link_libraries(xmbshell PRIVATE SDL2_mixer::SDL2_mixer)
target_link_libraries(xmbshell PRIVATE SDL2_image::SDL2_image)
target_link_libraries(xmbshell PRIVATE spdlog::spdlog)
target_link_libraries(xmbshell PRIVATE dl)
target_link_libraries(xmbshell PRIVATE SDBusCpp::sdbus-c++)
target_link_libraries(xmbshell PRIVATE Freetype::Freetype)
target_link_libraries(xmbshell PRIVATE optimized_components)
target_link_libraries(xmbshell PRIVATE PkgConfig::giomm)
if(GENERATE_POT)
  target_use_i18n(xmbshell NODOMAIN)
else()
  target_link_libraries(xmbshell PRIVATE i18n::i18n-lib)
endif()

include(GNUInstallDirs)

# read po/LINGUAS file
file(STRINGS po/LINGUAS LINGUAS)
foreach(LING ${LINGUAS})
  gettext_process_po_files(${LING} ALL PO_FILES po/${LING}.po)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${LING}.gmo DESTINATION ${CMAKE_INSTALL_LOCALEDIR}/${LING}/LC_MESSAGES/ RENAME xmbshell.mo)
endforeach()

foreach(shader ${shaders})
	add_shader(xmbshell ${shader})
endforeach()
target_include_directories(xmbshell PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/shaders)

install(TARGETS xmbshell)
install(FILES xmbshell.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES icons/xmbshell.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps RENAME xmbshell.png)
install(FILES schemas/re.jcm.xmbos.xmbshell.gschema.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/glib-2.0/schemas)
install(DIRECTORY icons DESTINATION ${CMAKE_INSTALL_DATADIR}/xmbshell/)
install(DIRECTORY sounds DESTINATION ${CMAKE_INSTALL_DATADIR}/xmbshell/)

set(CPACK_PACKAGE_NAME "xmbshell")
set(CPACK_PACKAGE_CONTACT "JCM <jcm@jcm.re>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A shell mimicking the XMB interface of the PlayStation 3")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://git.jcm.re/XMB-OS/xmbshell")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

include(CPack)
