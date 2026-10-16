# the name of the target operating system
set(CMAKE_SYSTEM_NAME Linux)

# which compilers to use for C and C++
set(CMAKE_C_COMPILER   gcc)
set(CMAKE_CXX_COMPILER g++)

# # Add Windows system libraries path
# set(CMAKE_SYSTEM_LIBRARY_PATH /usr/x86_64-w64-mingw32/lib)
# set(CMAKE_SYSTEM_INCLUDE_PATH /usr/x86_64-w64-mingw32/include)

# # Ensure dbghelp is available
# set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -ldbghelp")

# Release: statically link libstdc++/libgcc so the binary only depends on the
# target's glibc (not its libstdc++ version). glibc itself must stay dynamic -
# portability comes from building against an old glibc (.devcontainer/almalinux8).
# Debug is dev-only, so keep it dynamic to match the mingw toolchain file.
string(TOLOWER "${CMAKE_BUILD_TYPE}" _build_type_lower)
if(NOT _build_type_lower STREQUAL "debug")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++ -static-libgcc")
endif()
