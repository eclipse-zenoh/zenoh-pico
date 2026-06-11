set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/fuzz")

add_executable(z_fuzz_scouting_message_decode
               ${PROJECT_SOURCE_DIR}/fuzz/fuzz_scouting_message_decode.c)
target_link_libraries(z_fuzz_scouting_message_decode zenohpico::lib)
target_compile_options(z_fuzz_scouting_message_decode PRIVATE -fsanitize=fuzzer,address,undefined
                                                              -fno-omit-frame-pointer)
target_link_options(z_fuzz_scouting_message_decode PRIVATE -fsanitize=fuzzer,address,undefined)
