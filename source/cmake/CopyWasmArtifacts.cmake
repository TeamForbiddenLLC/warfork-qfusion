# Copy Emscripten output next to the web launcher, skipping missing files
# (warfork.data is only produced when a preload directory is present).

if (NOT SRC OR NOT DST)
    message(FATAL_ERROR "CopyWasmArtifacts: SRC and DST must be set")
endif()

file(MAKE_DIRECTORY "${DST}")

foreach (name warfork.js warfork.wasm warfork.data warfork.html)
    set(from "${SRC}/${name}")
    if (EXISTS "${from}")
        file(COPY "${from}" DESTINATION "${DST}")
    endif()
endforeach()
