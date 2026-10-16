message(STATUS "TARGET_TRIPLET in toolchain: '${TARGET_TRIPLET}'")
message(STATUS "ARCH in toolchain: '${ARCH}'")

# the name of the target operating system
set(CMAKE_SYSTEM_NAME Windows)

# which compilers to use for C and C++
set(CMAKE_C_COMPILER   ${TARGET_TRIPLET}-gcc)
set(CMAKE_CXX_COMPILER ${TARGET_TRIPLET}-g++)

# adjust the default behavior of the FIND_XXX() commands:
# search programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# search headers and libraries in the target environment
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Add Windows system libraries path
set(CMAKE_SYSTEM_LIBRARY_PATH /usr/${TARGET_TRIPLET}/lib)
set(CMAKE_SYSTEM_INCLUDE_PATH /usr/${TARGET_TRIPLET}/include)

# Ensure dbghelp is available
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=lld -ldbghelp -fno-lto -Wl,-Map=${CMAKE_BINARY_DIR}/TheShadesBelow.map")

# Release: statically link the MinGW runtime (libstdc++, libgcc, winpthread) so the
# exe doesn't need libstdc++-6.dll / libgcc_s_seh-1.dll / libwinpthread-1.dll shipped
# alongside it. Windows system DLLs are still linked via their import libs.
# Debug: keep the runtime DLLs. With -std=c++23+ the header defines
# std::type_info::operator== inline, and at -O0 that emitted copy clashes with the
# out-of-line one in mingw's static libstdc++.a (tinfo.o) -> lld "duplicate symbol"
# (GCC PR 110572). Linking against the DLL avoids the clash.
string(TOLOWER "${CMAKE_BUILD_TYPE}" _build_type_lower)
if(_build_type_lower STREQUAL "debug")
    file(COPY
        "/usr/lib/gcc/x86_64-w64-mingw32/14-win32/libstdc++-6.dll"
        "/usr/lib/gcc/x86_64-w64-mingw32/14-win32/libgcc_s_seh-1.dll"
        "/usr/x86_64-w64-mingw32/lib/libwinpthread-1.dll"
        DESTINATION ${CMAKE_BINARY_DIR}/bin
    )
else()
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++")
endif()