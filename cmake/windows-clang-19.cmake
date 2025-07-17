set(ENV{MSYS2_ROOT} "/tmp/quasi-msys2/root")

set(CMAKE_C_COMPILER clang-19)
set(CMAKE_CXX_COMPILER clang++-19)
set(CMAKE_LINKER_TYPE LLD)

include(${CMAKE_CURRENT_LIST_DIR}/windows.cmake)
