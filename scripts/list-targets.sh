#!/bin/bash
# Lists the build targets (scripts/cmake-<triplet>.cmake) whose compiler is installed in this
# container, one per line. Used by the buildarch picker in .vscode/tasks.json so e.g. the mingw
# target is only offered in the debian-trixie devcontainer.
cd "$(dirname "$0")" || exit 1

# list mingw first so it stays the top choice where it's available
for toolchain in cmake-x86_64-w64-mingw32.cmake cmake-x86_64-linux-gnu.cmake; do
    triplet="${toolchain#cmake-}"
    triplet="${triplet%.cmake}"
    # the native toolchain file uses plain g++, cross toolchain files use <triplet>-g++
    case "$triplet" in
        *-linux-gnu) compiler=g++ ;;
        *)           compiler="$triplet-g++" ;;
    esac
    if command -v "$compiler" >/dev/null; then echo "$triplet"; fi
done
