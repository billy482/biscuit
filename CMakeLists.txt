cmake_minimum_required(VERSION 3.10)

add_compile_options(-Werror -Wall -Wextra -pedantic -pipe)

find_package(OpenSSL REQUIRED)

if(CMAKE_BUILD_TYPE MATCHES Debug)
	message("Building with debug options")
	add_compile_options(-ggdb3 -O0)
else()
	message("Building with release options")
	add_compile_options(-O2)
endif()

project(biscuit VERSION 0.1)

file(GLOB_RECURSE biscuit_src_files src/*.cpp)
add_executable(biscuit "${biscuit_src_files}")
target_compile_features(biscuit PUBLIC cxx_std_17)
target_include_directories(biscuit PRIVATE "${OPENSSL_INCLUDE_DIR}" "${CMAKE_SOURCE_DIR}/include")
target_link_libraries(biscuit crypto)