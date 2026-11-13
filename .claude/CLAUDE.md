# TheShadesBelow

## Building

- Verify changes with the mingw builds only: `cmake --build build-x86_64-w64-mingw32-Debug` (or `-Release`).
  Skip the `build-x86_64-linux-gnu-*` build dirs; they aren't used locally.
- Use the mingw build dir for dependency analysis too (e.g. `ninja -C build-x86_64-w64-mingw32-Debug -t deps`).
