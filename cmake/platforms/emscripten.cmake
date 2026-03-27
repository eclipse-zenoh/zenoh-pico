zp_platform_add_definition(ZENOH_EMSCRIPTEN)
zp_platform_glob_sources("${PROJECT_SOURCE_DIR}/src/system/emscripten/*.c")
zp_platform_add_sources("${PROJECT_SOURCE_DIR}/src/link/backend/upper/ws_emscripten.c")
