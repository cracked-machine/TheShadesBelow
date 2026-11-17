# Copies res/ into the binary dir. Run at build time (see copy_res in CMakeLists.txt) so edits
# to res/ reach bin/ on every build; file(COPY) skips files whose timestamps are unchanged.
file(COPY ${SRC_DIR}/res DESTINATION ${DST_DIR}
     PATTERN "res/shaders/unused" EXCLUDE
     PATTERN "*.aseprite" EXCLUDE )
