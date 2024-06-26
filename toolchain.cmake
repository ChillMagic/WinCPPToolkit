get_filename_component(TOOLCHAIN_DIR "${CMAKE_TOOLCHAIN_FILE}" DIRECTORY)

set(CMAKE_MAKE_PROGRAM                 "${TOOLCHAIN_DIR}/bin/ninja.exe"           CACHE STRING "")
set(CMAKE_C_COMPILER                   "${TOOLCHAIN_DIR}/bin/clang.exe"           CACHE STRING "")
set(CMAKE_CXX_COMPILER                 "${TOOLCHAIN_DIR}/bin/clang++.exe"         CACHE STRING "")
set(CMAKE_RC_COMPILER                  "${TOOLCHAIN_DIR}/bin/llvm-rc.exe"         CACHE STRING "")
set(CMAKE_AR                           "${TOOLCHAIN_DIR}/bin/llvm-ar.exe"         CACHE STRING "")
set(CMAKE_CXX_COMPILER_CLANG_SCAN_DEPS "${TOOLCHAIN_DIR}/bin/clang-scan-deps.exe" CACHE STRING "")
