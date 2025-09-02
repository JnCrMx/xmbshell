cmake_minimum_required(VERSION 3.10)
cmake_policy(SET CMP0097 NEW)

set(CMAKE_C_FLAGS_DEBUGASAN "-g -fsanitize=address -fno-omit-frame-pointer -g" CACHE STRING "Flags used by the C compiler during AddressSanitizer builds." FORCE)
set(CMAKE_CXX_FLAGS_DEBUGASAN "-g -fsanitize=address -fno-omit-frame-pointer -g" CACHE STRING "Flags used by the C++ compiler during AddressSanitizer builds." FORCE)
set(CMAKE_EXE_LINKER_FLAGS_DEBUGASAN "-fsanitize=address" CACHE STRING "Flags used for linking binaries during AddressSanitizer builds." FORCE)
mark_as_advanced(CMAKE_C_FLAGS_DEBUGASAN CMAKE_CXX_FLAGS_DEBUGASAN CMAKE_EXE_LINKER_FLAGS_DEBUGASAN)

set(CMAKE_CXX_FLAGS_PROFILELLVM "-g -O3 -fprofile-instr-generate -fcoverage-mapping" CACHE STRING "Flags used by the C++ compiler during LLVM Profile builds." FORCE)
set(CMAKE_EXE_LINKER_FLAGS_PROFILELLVM "-fprofile-instr-generate -fcoverage-mapping" CACHE STRING "Flags used for linking binaries during LLVM Profile builds." FORCE)
mark_as_advanced(CMAKE_CXX_FLAGS_PROFILELLVM CMAKE_EXE_LINKER_FLAGS_PROFILELLVM)

set(CMAKE_CXX_FLAGS_PROFILE "-g -O3 -pg" CACHE STRING "Flags used by the C++ compiler during Profile builds." FORCE)
set(CMAKE_EXE_LINKER_FLAGS_PROFILE "-pg" CACHE STRING "Flags used for linking binaries during Profile builds." FORCE)
mark_as_advanced(CMAKE_CXX_FLAGS_PROFILE CMAKE_EXE_LINKER_FLAGS_PROFILE)

project(xmbshell VERSION 1.0 LANGUAGES CXX)
if(NOT DEFINED BUILD_VERSION)
  set(BUILD_VERSION ${PROJECT_VERSION})
endif()
message(STATUS "Building version ${BUILD_VERSION}")

option(GENERATE_POT "Generate .pot file" OFF)
option(SEPARATE_DEBUG_INFO "Generate separate debug info files" OFF)

set(CMAKE_CXX_SCAN_FOR_MODULES ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(BUILD_SHARED_LIBS OFF)
add_compile_options(-fPIC)
add_compile_options(-Wno-nullability-completeness)

find_package(glm REQUIRED)
find_package(Freetype REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(i18n++ QUIET)
find_package(Gettext REQUIRED)

include(FetchContent)

function(FetchContent_MakeAvailable_ExcludeFromAll)
  if(CMAKE_VERSION VERSION_LESS "3.28.0")
    foreach(_name ${ARGN})
      FetchContent_GetProperties(${_name})
      if(NOT ${_name}_POPULATED)
        FetchContent_Populate(${_name})
        add_subdirectory(${${_name}_SOURCE_DIR} ${${_name}_BINARY_DIR} EXCLUDE_FROM_ALL)
      endif()
    endforeach()
  else()
    FetchContent_MakeAvailable(${ARGN})
  endif()
endfunction()

if(NOT i18n++_FOUND)
  set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
  set(BUILD_PLUGIN ${GENERATE_POT} CACHE BOOL "" FORCE)
  set(BUILD_MERGE ${GENERATE_POT} CACHE BOOL "" FORCE)

  FetchContent_Declare(i18n++
    GIT_REPOSITORY https://github.com/JnCrMx/i18n-cpp.git
    GIT_TAG        trunk
    GIT_SUBMODULES_RECURSE FALSE
    EXCLUDE_FROM_ALL
  )
  FetchContent_MakeAvailable_ExcludeFromAll(i18n++)
endif()

pkg_check_modules(glibmm glibmm-2.68 REQUIRED IMPORTED_TARGET)
pkg_check_modules(giomm giomm-2.68 REQUIRED IMPORTED_TARGET)

set(SDL_BUILDING_LIBRARY ON CACHE BOOL "" FORCE)
find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(SDL2_mixer REQUIRED)

# Linux-only stuff
if(UNIX AND NOT APPLE)
  FetchContent_Declare(sdbus-cpp
    GIT_REPOSITORY https://github.com/Kistler-Group/sdbus-cpp.git
    GIT_TAG        v1.2.0
    EXCLUDE_FROM_ALL
  )
  set(BUILD_DOC OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable_ExcludeFromAll(sdbus-cpp)
  add_library(SDBusCpp::sdbus-c++ ALIAS sdbus-c++)

  FetchContent_Declare(waylandpp
    GIT_REPOSITORY https://github.com/NilsBrause/waylandpp.git
    GIT_TAG        master
    EXCLUDE_FROM_ALL
  )
  FetchContent_MakeAvailable_ExcludeFromAll(waylandpp)
  add_library(waylandpp::wayland-server++ ALIAS wayland-server++)
  add_library(waylandpp::wayland-server-extra++ ALIAS wayland-server-extra++)

  add_library(waylandppModule STATIC)
  target_sources(waylandppModule PUBLIC FILE_SET CXX_MODULES FILES third_party/waylandpp.cppm)
  target_compile_features(waylandppModule PUBLIC cxx_std_23)
  target_link_libraries(waylandppModule PUBLIC waylandpp::wayland-server++ waylandpp::wayland-server-extra++)
  target_compile_options(waylandppModule PRIVATE -pthread)
endif()

FetchContent_Declare(dreamrender
  GIT_REPOSITORY https://git.jcm.re/jcm/dreamrender.git
  GIT_TAG        main
  EXCLUDE_FROM_ALL
)
set(DREAMS_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(DREAMS_BUILD_WITH_POSIX_THREADS ON CACHE BOOL "" FORCE)
set(DREAMS_FIND_PACKAGES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable_ExcludeFromAll(dreamrender)

FetchContent_Declare(argparse
  GIT_REPOSITORY https://github.com/p-ranav/argparse.git
  GIT_TAG        v3.1
  EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable_ExcludeFromAll(argparse)

FetchContent_Declare(avcpp
  GIT_REPOSITORY https://github.com/h4tr3d/avcpp.git
  GIT_TAG        v2.7.1
  GIT_SUBMODULES ""
  EXCLUDE_FROM_ALL
)
set(AV_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(AV_ENABLE_STATIC ON CACHE BOOL "" FORCE)
set(AV_ENABLE_SHARED OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable_ExcludeFromAll(avcpp)

add_library(glibmmModule STATIC)
target_sources(glibmmModule PUBLIC FILE_SET CXX_MODULES FILES third_party/glibmm.cppm)
target_compile_features(glibmmModule PUBLIC cxx_std_20)
target_link_libraries(glibmmModule PUBLIC PkgConfig::glibmm)
target_compile_options(glibmmModule PRIVATE -pthread)

add_library(giommModule STATIC)
target_sources(giommModule PUBLIC FILE_SET CXX_MODULES FILES third_party/giomm.cppm)
target_compile_features(giommModule PUBLIC cxx_std_20)
target_link_libraries(giommModule PUBLIC PkgConfig::giomm)
target_compile_options(giommModule PRIVATE -pthread)

add_library(i18nModule STATIC)
target_sources(i18nModule PUBLIC FILE_SET CXX_MODULES FILES third_party/i18n.cppm)
target_compile_features(i18nModule PUBLIC cxx_std_20)
target_link_libraries(i18nModule PUBLIC i18n::i18n-lib)
target_compile_options(i18nModule PRIVATE -pthread)

add_library(argparseModule STATIC)
target_sources(argparseModule PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${argparse_SOURCE_DIR}/module FILES ${argparse_SOURCE_DIR}/module/argparse.cppm)
target_compile_features(argparseModule PUBLIC cxx_std_23)
target_link_libraries(argparseModule PUBLIC argparse::argparse)
target_compile_options(argparseModule PRIVATE -pthread)

add_library(avcppModule STATIC)
target_sources(avcppModule PUBLIC FILE_SET CXX_MODULES FILES third_party/avcpp.cppm)
target_compile_features(avcppModule PUBLIC cxx_std_23)
target_link_libraries(avcppModule PUBLIC avcpp::avcpp)
target_compile_options(avcppModule PRIVATE -pthread)

set(XMBSHELL_SOURCES
  src/app/xmbshell.cpp
  src/app/components/choice_overlay.cpp
  src/app/components/main_menu.cpp
  src/app/components/message_overlay.cpp
  src/app/components/news_display.cpp
  src/app/components/progress_overlay.cpp
  src/app/layers/blur_layer.cpp
  src/menu/applications_menu.cpp
  src/menu/files_menu.cpp
  src/menu/settings_menu.cpp
  src/menu/users_menu.cpp
  src/dbus.cpp
  src/render/shaders.cpp
  src/config.cpp
  src/main.cpp
  src/utils.cpp
)
set(XMBSHELL_MODULE_SOURCES
  src/app/module.cppm
  src/app/xmbshell.cppm
  src/app/component.cppm
  src/app/components/choice_overlay.cppm
  src/app/components/main_menu.cppm
  src/app/components/message_overlay.cppm
  src/app/components/news_display.cppm
  src/app/components/progress_overlay.cppm
  src/app/layers/blur_layer.cppm
  src/app/wayland/wayland_server.cppm
  src/config.cppm
  src/constants.cppm
  src/dbus.cppm
  src/menu/applications_menu.cppm
  src/menu/base.cppm
  src/menu/files_menu.cppm
  src/menu/settings_menu.cppm
  src/menu/users_menu.cppm
  src/menu/utils.cppm
  src/programs.cppm
  src/programs/base_viewer.cppm
  src/programs/image_viewer.cppm
  src/programs/text_viewer.cppm
  src/programs/video_player.cppm
  src/render/module.cppm
  src/render/shaders.cppm
  src/render/components/wave_renderer.cppm
  src/utils.cppm
)
set(XMBSHELL_SHADERS
  shaders/blur.comp
  shaders/wave.vert
  shaders/wave.frag
  shaders/wayland_surface.vert
  shaders/wayland_surface.frag
  shaders/yuv420p_decode.comp
)

add_executable(xmbshell ${XMBSHELL_SOURCES})
target_sources(xmbshell PUBLIC
  FILE_SET CXX_MODULES
  BASE_DIRS src
  FILES ${XMBSHELL_MODULE_SOURCES}
)

target_include_directories(xmbshell PRIVATE include/)
target_include_directories(xmbshell PRIVATE ${GLM_INCLUDE_DIRS})

target_compile_options(xmbshell PRIVATE --embed-dir=${CMAKE_CURRENT_BINARY_DIR}/shaders)
target_compile_options(xmbshell PRIVATE --embed-dir=${CMAKE_CURRENT_BINARY_DIR})
target_compile_options(xmbshell PRIVATE --embed-dir=${CMAKE_CURRENT_SOURCE_DIR})
if(WIN32)
  target_compile_options(xmbshell PRIVATE --embed-dir=${CMAKE_FIND_ROOT_PATH})
endif()

target_link_libraries(xmbshell PRIVATE dreams::dreamrender)
target_link_libraries(xmbshell PRIVATE dreams::sdl2Module)
target_link_libraries(xmbshell PRIVATE dreams::glmModule)
target_link_libraries(xmbshell PRIVATE dreams::VulkanHppModule)
target_link_libraries(xmbshell PRIVATE dreams::vmaModule)
target_link_libraries(xmbshell PRIVATE dreams::spdlogModule)
target_link_libraries(xmbshell PRIVATE Freetype::Freetype)
target_link_libraries(xmbshell PRIVATE glibmmModule)
target_link_libraries(xmbshell PRIVATE giommModule)
target_link_libraries(xmbshell PRIVATE i18nModule)
target_link_libraries(xmbshell PRIVATE argparseModule)
target_link_libraries(xmbshell PRIVATE avcppModule)

if(GENERATE_POT)
  target_use_i18n(xmbshell NODOMAIN)
else()
  target_link_libraries(xmbshell PRIVATE i18n::i18n-lib)
endif()

if(UNIX AND NOT APPLE)
  target_link_libraries(xmbshell PRIVATE dl)
  target_link_libraries(xmbshell PRIVATE X11)
  target_link_libraries(xmbshell PRIVATE SDBusCpp::sdbus-c++)
  target_link_libraries(xmbshell PRIVATE waylandppModule)

  add_executable(wayland_server_test "test/wayland_server.cpp")
  target_link_libraries(wayland_server_test PRIVATE waylandppModule)
  target_compile_options(wayland_server_test PRIVATE -pthread)
endif()

if(SEPARATE_DEBUG_INFO)
  function(separate_release_and_debug TARGET)
    add_custom_command(
      TARGET ${TARGET} POST_BUILD
      # Separate the Debug Information from the Target file
      COMMAND ${CMAKE_OBJCOPY} ARGS --only-keep-debug $<TARGET_FILE:${TARGET}> $<TARGET_FILE:${TARGET}>.dbg
      # As we have previously separated the debug information from the
      # previous output, remove all debug information from the same
      COMMAND ${CMAKE_OBJCOPY} ARGS --strip-debug $<TARGET_FILE:${TARGET}>
      # Link the Separate Debug Information with the Final Release
      COMMAND ${CMAKE_OBJCOPY} ARGS --add-gnu-debuglink=$<TARGET_FILE:${TARGET}>.dbg $<TARGET_FILE:${TARGET}>
    )
    set_property(
      TARGET ${TARGET}
      APPEND PROPERTY ADDITIONAL_CLEAN_FILES $<TARGET_FILE:${TARGET}>.dbg
    )
  endfunction(separate_release_and_debug)

  separate_release_and_debug(xmbshell)
endif()

if(WIN32)
  set(CMAKE_INSTALL_LOCALEDIR "/locale")
  set(CMAKE_INSTALL_DATADIR "")
  set(CMAKE_INSTALL_LIBDIR "")
  set(CMAKE_INSTALL_BINDIR "")
else()
  include(GNUInstallDirs)
endif(WIN32)

# read po/LINGUAS file
file(STRINGS po/LINGUAS LINGUAS)
foreach(LING ${LINGUAS})
  gettext_process_po_files(${LING} ALL PO_FILES po/${LING}.po)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${LING}.gmo DESTINATION ${CMAKE_INSTALL_LOCALEDIR}/${LING}/LC_MESSAGES/ RENAME xmbshell.mo)
  add_custom_target(xmbshell-link-po-${LING} ALL
    COMMAND
      ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/locales/${LING}/LC_MESSAGES &&
      ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_BINARY_DIR}/${LING}.gmo ${CMAKE_CURRENT_BINARY_DIR}/locales/${LING}/LC_MESSAGES/xmbshell.mo
  )
endforeach()

foreach(shader ${XMBSHELL_SHADERS})
  dreams_add_shader(xmbshell ${shader})
endforeach()
target_include_directories(xmbshell PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/shaders)

file(GLOB ICON_FILES icons/*.png)

install(TARGETS xmbshell
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}/"
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}/"
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/"
)
install(FILES schemas/re.jcm.xmbos.xmbshell.gschema.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/glib-2.0/schemas)
install(FILES ${ICON_FILES} DESTINATION ${CMAKE_INSTALL_DATADIR}/xmbshell/icons)
install(DIRECTORY sounds DESTINATION ${CMAKE_INSTALL_DATADIR}/xmbshell/)

if(UNIX AND NOT APPLE)
  install(FILES xmbshell.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
  install(FILES icons/xmbshell.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps RENAME xmbshell.png)
endif()

set(CPACK_PACKAGE_NAME "xmbshell")
set(CPACK_PACKAGE_VERSION ${BUILD_VERSION})
set(CPACK_PACKAGE_VENDOR "JCM")
set(CPACK_PACKAGE_CONTACT "JCM <jcm@jcm.re>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A shell mimicking the XMB interface of the PlayStation 3")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://git.jcm.re/XMB-OS/xmbshell")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
include(CPack)
