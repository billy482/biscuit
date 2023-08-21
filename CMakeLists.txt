cmake_minimum_required(VERSION 3.10)

project(biscuit VERSION 0.1 LANGUAGES CXX)

if(CMAKE_BUILD_TYPE MATCHES Debug)
	message("Building with debug options")
	add_compile_options(-ggdb3 -O0)
else()
	message("Building with release options")
	add_compile_options(-O2)
endif()

add_compile_options(-Werror -Wall -Wextra -pedantic -pipe)

find_package(OpenSSL REQUIRED)

if(EXISTS /usr/lib/x86_64-linux-gnu/cmake/Qt6)
	list(APPEND CMAKE_PREFIX_PATH /usr/lib/x86_64-linux-gnu/cmake)
	set(Qt6_INCLUDE_DIR "/usr/include/x86_64-linux-gnu/qt6/")
endif()
find_package(Qt6 REQUIRED COMPONENTS Core)

find_package(yaml-cpp REQUIRED)

file(GLOB_RECURSE biscuit_src_files src/*.cpp)
add_executable(biscuit "${biscuit_src_files}")
target_compile_features(biscuit PUBLIC cxx_std_17)
target_include_directories(biscuit PRIVATE "${OPENSSL_INCLUDE_DIR}" "${CMAKE_SOURCE_DIR}/include" "${Qt6_INCLUDE_DIR}")
target_link_libraries(biscuit PRIVATE crypto Qt6::Core yaml-cpp)