set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(TARGET_TRIPLE x86_64-pc-windows-gnu)
set(CMAKE_C_COMPILER_TARGET ${TARGET_TRIPLE})
set(CMAKE_CXX_COMPILER_TARGET ${TARGET_TRIPLE})
set(CMAKE_LIBRARY_ARCHITECTURE ${TARGET_TRIPLE})
set(CMAKE_CXX_FLAGS "--start-no-unused-arguments --sysroot=$ENV{MSYS2_ROOT}/ucrt64 -pthread -fuse-ld=lld-19 -stdlib=libstdc++ -femulated-tls -rtlib=libgcc -unwindlib=libgcc -static --end-no-unused-arguments")
set(CMAKE_C_FLAGS "--start-no-unused-arguments --sysroot=$ENV{MSYS2_ROOT}/ucrt64 -pthread -fuse-ld=lld-19 -femulated-tls -rtlib=libgcc -unwindlib=libgcc -static --end-no-unused-arguments")

set(CMAKE_FIND_ROOT_PATH "$ENV{MSYS2_ROOT}/ucrt64/")
set(ENV{PKG_CONFIG_PATH} "$ENV{MSYS2_ROOT}/ucrt64/lib/pkgconfig:$ENV{MSYS2_ROOT}/ucrt64/share/pkgconfig")
set(ENV{PKG_CONFIG_SYSROOT_DIR} "$ENV{MSYS2_ROOT}")
set(ENV{PKG_CONFIG_SYSTEM_LIBRARY_PATH} "$ENV{MSYS2_ROOT}/ucrt64/lib")
#set(ENV{PKG_CONFIG_SYSTEM_INCLUDE_PATH} "$ENV{MSYS2_ROOT}/ucrt64/include")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
