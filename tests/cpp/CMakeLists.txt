cmake_minimum_required(VERSION 3.14)
project(zenohpico_cpp_api_tests LANGUAGES CXX)

find_package(zenohpico REQUIRED)
option(ASAN "Enable AddressSanitizer." OFF)
if(ASAN AND NOT MSVC)
  add_compile_options(-fsanitize=address)
  add_link_options(-fsanitize=address)
endif()

enable_testing()
add_executable(z_cpp_clone_test z_cpp_clone_test.cpp)
target_compile_features(z_cpp_clone_test PRIVATE cxx_std_11)
target_link_libraries(z_cpp_clone_test PRIVATE zenohpico::lib)
if(WIN32)
  add_custom_command(TARGET z_cpp_clone_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:zenohpico::lib> $<TARGET_FILE_DIR:z_cpp_clone_test>)
endif()
add_test(NAME z_cpp_clone_test COMMAND z_cpp_clone_test)
